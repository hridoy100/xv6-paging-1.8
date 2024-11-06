# Introduced xv6-paging-1.8

This project introduces support for **page faults** in the **xv6** operating system, a feature missing in earlier versions like **1.6**. Based on **xv6 version 1.8**, it incorporates new **system calls** to efficiently handle paging events. These changes enable dynamic memory management, similar to modern OS behavior, optimizing memory allocation and resource usage. The system calls offer finer control over memory handling, enhancing the robustness and functionality of xv6, and bringing it closer to real-world OS capabilities.

# Page Fault Resolution Algorithms 🖥️💾

This project implements various page fault resolution algorithms to manage memory in an operating system:

- **FIFO (First-In-First-Out)** 🔄: Replaces the oldest page.
- **LRU (Least Recently Used)** ⏳: Replaces the page that hasn’t been used for the longest.
- **LFU (Least Frequently Used)** 📉: Replaces the least accessed page.
- **Optimal Page Replacement** 🎯: Replaces the page not needed for the longest time (theoretical).
- **Clock (Second Chance)** 🕰️: Uses a circular list with reference bits for replacement.

Each algorithm has unique performance characteristics depending on memory access patterns.


**Isn't compatible/executable with ubuntu 18.04**
reference : https://github.com/asafch/xv6-paging

It was implemented for xv6 1.6 version. I've converted this according to xv6 1.8 version.
