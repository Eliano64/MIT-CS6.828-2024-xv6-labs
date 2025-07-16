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

#define NHASH 13
#define gethash(i) ((i)%NHASH)

struct bucket{
  struct spinlock lock;

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
};

struct {
  struct bucket table[NHASH];
  struct buf buf[NBUF];
} bcache;



void
binit(void)
{
  struct buf *b;

  for(int i=0;i<NHASH;++i){
    char name[9];
    snprintf(name, 9,"bcache%d", i);
    initlock(&bcache.table[i].lock, name);
  }

  // Create linked list of buffers
  for(int i=0;i<NHASH;++i){
    bcache.table[i].head.prev = &bcache.table[i].head;
    bcache.table[i].head.next = &bcache.table[i].head;
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int i = gethash((b->blockno));
    //printf("i: %d refcnt: %d\n",i,b->refcnt);
    b->next = bcache.table[i].head.next;
    b->prev = &bcache.table[i].head;
    initsleeplock(&b->lock, "buffer");
    bcache.table[i].head.next->prev = b;
    bcache.table[i].head.next = b;
  }
  
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.table[gethash(blockno)].lock);
  // printf("blockno: %d\n",blockno);
  // Is the block already cached?
  for(b = bcache.table[gethash(blockno)].head.next; b != &bcache.table[gethash(blockno)].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.table[gethash(blockno)].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int key = gethash(blockno);
  int cnt = 0;
loop:
  ++cnt;
  for(b = bcache.table[key].head.prev; b != &bcache.table[key].head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      if(key!=gethash(blockno)){
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.table[key].lock);
        acquire(&bcache.table[gethash(blockno)].lock);
        b->next = bcache.table[gethash(blockno)].head.next;
        b->prev = &bcache.table[gethash(blockno)].head;
        bcache.table[gethash(blockno)].head.next->prev = b;
        bcache.table[gethash(blockno)].head.next = b;
        release(&bcache.table[gethash(blockno)].lock);
      }
      else{
        release(&bcache.table[key].lock);
      }
      acquiresleep(&b->lock);
      return b;
    }
  }
  // if this bucket is empty, steal one node from another one.
  if(cnt<=NHASH){
    release(&bcache.table[key].lock);
    key = gethash(key+1);
    acquire(&bcache.table[key].lock);
    goto loop;
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

  releasesleep(&b->lock);

  acquire(&bcache.table[gethash(b->blockno)].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.table[gethash(b->blockno)].head.next;
    b->prev = &bcache.table[gethash(b->blockno)].head;
    bcache.table[gethash(b->blockno)].head.next->prev = b;
    bcache.table[gethash(b->blockno)].head.next = b;
  }
  
  release(&bcache.table[gethash(b->blockno)].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.table[gethash(b->blockno)].lock);
  b->refcnt++;
  release(&bcache.table[gethash(b->blockno)].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.table[gethash(b->blockno)].lock);
  b->refcnt--;
  release(&bcache.table[gethash(b->blockno)].lock);
}


