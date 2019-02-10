#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();
  int pagesInPhyMem = 0;
  int pagesInSwapFile = 0;
  int totalPageFaultCount = 0;
  int totalPagedOutCount = 0;
  struct freepg *head = 0;
  struct freepg *tail = 0;

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // backup and reset proc fields
  //TODO delete   
  cprintf("EXEC: NONE undefined (proc = %s)- backing up page info \n", curproc->name);
  pagesInPhyMem = curproc->pagesInPhyMem;
  pagesInSwapFile = curproc->pagesInSwapFile;
  totalPageFaultCount = curproc->totalPageFaultCount;
  totalPagedOutCount = curproc->totalPagedOutCount;
  struct freepg freepages[MAX_PSYC_PAGES];
  struct pgdesc swappedpages[MAX_PSYC_PAGES];
  for (i = 0; i < MAX_PSYC_PAGES; i++) {
    freepages[i].virtualAddress = curproc->freepages[i].virtualAddress;
    curproc->freepages[i].virtualAddress = (char*)0xffffffff;
    freepages[i].next = curproc->freepages[i].next;
    curproc->freepages[i].next = 0;
    freepages[i].prev = curproc->freepages[i].prev;
    curproc->freepages[i].prev = 0;
    freepages[i].age = curproc->freepages[i].age;
    curproc->freepages[i].age = 0;
    swappedpages[i].age = curproc->swappedpages[i].age;
    curproc->swappedpages[i].age = 0;
    swappedpages[i].virtualAddress = curproc->swappedpages[i].virtualAddress;
    curproc->swappedpages[i].virtualAddress = (char*)0xffffffff;
    swappedpages[i].swaploc = curproc->swappedpages[i].swaploc;
    curproc->swappedpages[i].swaploc = 0;
  }
  head = curproc->head;
  tail = curproc->tail;
  curproc->pagesInPhyMem = 0;
  curproc->pagesInSwapFile = 0;
  curproc->totalPageFaultCount = 0;
  curproc->totalPagedOutCount = 0;
  curproc->head = 0;
  curproc->tail = 0;



  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  // a swap file has been created in fork(), but its content was of the
  // parent process, and is no longer relevant.
  removeSwapFile(curproc);
  createSwapFile(curproc);
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  curproc->pagesInPhyMem = pagesInPhyMem;
  curproc->pagesInSwapFile = pagesInSwapFile;
  curproc->totalPageFaultCount = totalPageFaultCount;
  curproc->totalPagedOutCount = totalPagedOutCount;
  curproc->head = head;
  curproc->tail = tail;
  for (i = 0; i < MAX_PSYC_PAGES; i++) {
    curproc->freepages[i].virtualAddress = freepages[i].virtualAddress;
    curproc->freepages[i].next = freepages[i].next;
    curproc->freepages[i].prev = freepages[i].prev;
    curproc->freepages[i].age = freepages[i].age;
    curproc->swappedpages[i].age = swappedpages[i].age;
    curproc->swappedpages[i].virtualAddress = swappedpages[i].virtualAddress;
    curproc->swappedpages[i].swaploc = swappedpages[i].swaploc;
  }
  return -1;
}
