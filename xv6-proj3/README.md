# OS Project 3

## Copy-on-Write (CoW) in xv6

This project implements Copy-on-Write (CoW) memory management in xv6.

### Objectives
- Implement physical page reference counting.
- Modify `copyuvm()` to share physical pages between parent and child processes.
- Implement a page fault handler to perform Copy-on-Write.
- Verify the implementation using the provided test programs.
