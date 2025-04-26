// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKET 13
#define NBUFFER ((NBUF - 1) / BUCKET + 1)

struct {
  struct spinlock lock;
  struct buf buf[NBUFFER];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache[BUCKET];

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < BUCKET; i++){
    initlock(&bcache[i].lock, "bcache");

    // Create linked list of buffers
    // bcache[i].head.prev = &bcache[i].head;
    // bcache[i].head.next = &bcache[i].head;
    for(b = bcache[i].buf; b < bcache[i].buf+NBUFFER; b++){
      // b->next = bcache[i].head.next;
      // b->prev = &bcache[i].head;
      initsleeplock(&b->lock, "buffer");
      b->timestamp = 0;
      // bcache[i].head.next->prev = b;
      // bcache[i].head.next = b;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket = blockno % BUCKET;
  acquire(&bcache[bucket].lock);

  // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }


  for(int i = 0; i < NBUFFER; i++){
    b = &bcache[bucket].buf[i];
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->timestamp = ticks;
      release(&bcache[bucket].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  struct buf *tmp = 0;
  uint least = 0xffffffff;
  int id = 0;

  release(&bcache[bucket].lock);
  for(int i = 0; i < BUCKET; i++, bucket++){
    bucket %= BUCKET;
    acquire(&bcache[bucket].lock);
    for(int j = 0; j < NBUFFER; j++){
      b = &bcache[bucket].buf[j];
      if(b->refcnt == 0){
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->timestamp = ticks;
        release(&bcache[bucket].lock);
        acquiresleep(&b->lock);
        return b;
      }
      if(b->timestamp < least){
        least = b->timestamp;
        tmp = b;
        id = bucket;
      }
    }
    release(&bcache[bucket].lock);
  }

  if(tmp){
    acquire(&bcache[bucket].lock);
    tmp->dev = dev;
    tmp->blockno = blockno;
    tmp->valid = 0;
    tmp->refcnt = 1;
    tmp->timestamp = ticks;
    release(&bcache[id].lock);
    acquiresleep(&tmp->lock);
    return tmp;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");


  acquire(&bcache[b->blockno % BUCKET].lock);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  release(&bcache[b->blockno % BUCKET].lock);
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache[b->blockno % BUCKET].lock);
  b->refcnt++;
  release(&bcache[b->blockno % BUCKET].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache[b->blockno % BUCKET].lock);
  b->refcnt--;
  release(&bcache[b->blockno % BUCKET].lock);
}


