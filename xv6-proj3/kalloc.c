// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

struct {
  int num_free_pages;
  uint refcount[PHYSTOP >> PGSHIFT];
} pmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  
  // Initialize reference counters and free-page counter
  memset(pmem.refcount, 0, sizeof(pmem.refcount));
  pmem.num_free_pages = 0;

  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  // Add remaining physical pages to the freelist
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;
  uint pa = V2P(v); // Get the physical address

  if((uint)v % PGSIZE || v < end || pa >= PHYSTOP)
    panic("kfree");

  if(kmem.use_lock)
    acquire(&kmem.lock);

  // Decrease reference count if the page is in use
  if(pmem.refcount[pa >> PGSHIFT] > 0)
    pmem.refcount[pa >> PGSHIFT]--;

  // Only free the page when no references remain
  if(pmem.refcount[pa >> PGSHIFT] == 0){ // When reference count = 0
    memset(v, 1, PGSIZE);

    r = (struct run*)v;
    r->next = kmem.freelist;
    kmem.freelist = r;

    pmem.num_free_pages++;
  }

  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;

  pmem.num_free_pages--;

  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

int
freemem(void)
{
  return pmem.num_free_pages; 
}

uint
get_refcount(uint pa)
{
  return pmem.refcount[pa >> PGSHIFT]; // Return the reference count of a physical page
}


void
inc_refcount(uint pa)
{
  pmem.refcount[pa >> PGSHIFT]++; // Increases the count when another reference is added
}

void  
dec_refcount(uint pa)
{
  pmem.refcount[pa >> PGSHIFT]--; // 	Decreases the count when a reference is removed
}

