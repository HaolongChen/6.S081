// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int count[PA2PID(PHYSTOP) + 1];
} reference;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&reference.lock, "reference");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    reference.count[PA2PID((uint64)p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&reference.lock);
  if(reference.count[PA2PID((uint64)pa)] <= 0){
    release(&reference.lock);
    return;
  }

  if(--reference.count[PA2PID((uint64)pa)] <= 0){
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);
    reference.count[PA2PID((uint64)pa)] = 0;

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&reference.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    
    // Properly update reference count with lock
    acquire(&reference.lock);
    reference.count[PA2PID((uint64)r)] = 1;
    release(&reference.lock);
  }
  return (void*)r;
}

void
refcnt_inc(void* pa)
{
  // Validate the pointer first
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return;
    
  acquire(&reference.lock);
  reference.count[PA2PID((uint64)pa)]++;
  release(&reference.lock);
}

int get_refcnt(void* pa)
{
  int result;
  acquire(&reference.lock);
  result = reference.count[PA2PID((uint64)pa)];
  release(&reference.lock);
  return result;
}