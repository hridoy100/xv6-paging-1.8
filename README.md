# Introduced xv6-paging-1.8

In this project, changes were made to add support for **page faults** in the xv6 operating system, a feature that wasn’t available in earlier versions like **xv6 version 1.6**. Using version 1.8 as the base, this implementation introduces specific **system calls** designed to handle paging events more effectively. These enhancements allow xv6 to manage memory more dynamically, simulating modern OS capabilities by allocating memory on-demand and optimizing resource usage. The introduced system calls provide finer control over memory handling, improving the operating system’s robustness and functionality.

**doesn't run in ubuntu 18.04**
reference : https://github.com/asafch/xv6-paging
It was implemented for xv6 1.6 version. I've converted this according to xv6 1.8 version.
