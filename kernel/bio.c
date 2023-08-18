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

/*
#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET  12
#define NBUFS (NBUCKET * 3)
struct {
  struct spinlock lock;
  struct buf buf[NBUFS];
} bcache;

struct bucket {
  struct spinlock lock;  // 自旋锁，用于保护哈希桶的并发访问
  struct buf head;       // 哈希桶中链表的头节点
} hashtable[NBUCKET];    // 哈希表，包含多个哈希桶，大小为 NBUCKET

// 计算块号的哈希值，用于确定哈希桶的索引
int
hash(uint blockno)
{
  return blockno % NBUCKET; // 使用取余操作计算哈希值
}


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache"); // 初始化缓存锁，用于保护缓存的并发访问
  for(b = bcache.buf; b < bcache.buf+NBUFS; b++) {
    initsleeplock(&b->lock, "buffer"); // 初始化睡眠锁，用于保护缓存中的缓冲区
  }
  b = bcache.buf;
  for(int i = 0; i < NBUCKET; i++) {
     initlock(&hashtable[i].lock, "bcache_bucket"); // 初始化哈希桶的锁
     for(int j=0; j<NBUFS/NBUCKET; j++) {
        b->blockno = i; // 设置缓冲区的块号为当前哈希桶的索引
        b->next = hashtable[i].head.next; // 将当前缓冲区插入哈希桶链表的头部
        hashtable[i].head.next = b; // 更新哈希桶链表的头指针
        b++;
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

  int idx = hash(blockno); // 计算块号的哈希值，以找到对应的哈希桶
  struct bucket* bucket = hashtable + idx; // 获取对应的哈希桶
  acquire(&bucket->lock); // 获取哈希桶的锁，保护对哈希桶的访问

  // Is the block already cached?
  for(b = bucket->head.next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++; // 增加引用计数
      release(&bucket->lock); // 释放哈希桶的锁
      acquiresleep(&b->lock); // 获取缓冲区的睡眠锁
      // printf("Cached %p\n", b);
      return b;
    }
  }

  // Not cached.
  // First try to find in current bucket.
  int min_time = 0x8fffffff;
  struct buf* replace_buf = 0;

  // 寻找未被引用且时间戳最早的缓冲区，用于替换
  for(b = bucket->head.next; b != 0; b = b->next){
    if(b->refcnt == 0 && b->timestamp < min_time) {
      replace_buf = b;
      min_time = b->timestamp;
    }
  }
  if(replace_buf) {
    // printf("Local %d %p\n", idx, replace_buf);
    goto find;
  }

  // Try to find in other bucket.
  acquire(&bcache.lock); // 获取全局缓冲区锁
  refind:
  for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
    if(b->refcnt == 0 && b->timestamp < min_time) {
      replace_buf = b;
      min_time = b->timestamp;
    }
  }
  if (replace_buf) {
    // remove from old bucket
    int ridx = hash(replace_buf->blockno); // 计算替换缓冲区块号的哈希值
    acquire(&hashtable[ridx].lock); // 获取替换缓冲区所在哈希桶的锁
    if(replace_buf->refcnt != 0)  // 在找到并获取锁之间可能被其他线程使用
    {
      release(&hashtable[ridx].lock);
      goto refind;
    }
    struct buf *pre = &hashtable[ridx].head;
    struct buf *p = hashtable[ridx].head.next;
    while (p != replace_buf) {
      pre = pre->next;
      p = p->next;
    }
    pre->next = p->next;
    release(&hashtable[ridx].lock); // 从旧的哈希桶中移除替换缓冲区并释放锁
    // add to current bucket
    replace_buf->next = hashtable[idx].head.next; // 将替换缓冲区添加到当前哈希桶
    hashtable[idx].head.next = replace_buf;
    release(&bcache.lock); // 释放全局缓冲区锁
    // printf("Global %d -> %d %p\n", ridx, idx, replace_buf);
    goto find;
  }
  else {
    panic("bget: no buffers"); // 没有可用的缓冲区，产生 panic
  }

  find:
  replace_buf->dev = dev; // 设置替换缓冲区的设备号
  replace_buf->blockno = blockno; // 设置替换缓冲区的块号
  replace_buf->valid = 0; // 标记替换缓冲区为无效
  replace_buf->refcnt = 1; // 设置替换缓冲区的引用计数为1
  release(&bucket->lock); // 释放哈希桶的锁
  acquiresleep(&replace_buf->lock); // 获取替换缓冲区的睡眠锁
  return replace_buf; // 返回获取的缓冲区
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
      panic("brelse"); // 如果当前线程未持有缓冲区的睡眠锁，则产生 panic

   releasesleep(&b->lock); // 释放缓冲区的睡眠锁，允许其他线程访问缓冲区

   int idx = hash(b->blockno); // 计算缓冲区块号的哈希值，以找到对应的哈希桶

   acquire(&hashtable[idx].lock); // 获取哈希桶的锁，保护对哈希桶的访问
   b->refcnt--; // 减少缓冲区的引用计数
   if (b->refcnt == 0) {
      // no one is waiting for it.
      b->timestamp = ticks; // 如果没有线程正在等待该缓冲区，更新缓冲区的时间戳为当前 ticks 值
  }
  release(&hashtable[idx].lock); // 释放哈希桶的锁，允许其他线程访问哈希桶
}


void
bpin(struct buf *b) {
  int idx = hash( b->blockno); // 计算块号的哈希值，以找到对应的哈希桶
  acquire(&hashtable[idx].lock); // 获取哈希桶的锁，保护对哈希桶的访问
  b->refcnt++; // 增加引用计数
  release(&hashtable[idx].lock); // 释放哈希桶的锁
}

void
bunpin(struct buf *b) {
  int idx = hash(b->blockno); // 计算块号的哈希值，以找到对应的哈希桶
  acquire(&hashtable[idx].lock); // 获取哈希桶的锁，保护对哈希桶的访问
  b->refcnt--; // 减少引用计数
  release(&hashtable[idx].lock); // 释放哈希桶的锁
}
*/
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

#define NBUCKET 13
#undef NBUF
#define NBUF (NBUCKET * 3)

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

struct bucket {
  struct spinlock lock;
  struct buf head;
}hashtable[NBUCKET];

int
hash(uint dev, uint blockno)
{
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }

  b = bcache.buf;
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&hashtable[i].lock, "bcache_bucket");
    for (int j = 0; j < NBUF / NBUCKET; j++) {
      b->blockno = i;
      b->next = hashtable[i].head.next;
      hashtable[i].head.next = b;
      b++;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  // printf("dev: %d blockno: %d Status: ", dev, blockno);
  struct buf *b;

  int idx = hash(dev, blockno);
  struct bucket* bucket = hashtable + idx;
  acquire(&bucket->lock);

  // Is the block already cached?
  for(b = bucket->head.next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      // printf("Cached %p\n", b);
      return b;
    }
  }

  // Not cached.
  // First try to find in current bucket.
  int min_time = 0x8fffffff;
  struct buf* replace_buf = 0;

  for(b = bucket->head.next; b != 0; b = b->next){
    if(b->refcnt == 0 && b->timestamp < min_time) {
      replace_buf = b;
      min_time = b->timestamp;
    }
  }
  if(replace_buf) {
    // printf("Local %d %p\n", idx, replace_buf);
    goto find;
  }

  // Try to find in other bucket.
  acquire(&bcache.lock);
  refind:
  for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
    if(b->refcnt == 0 && b->timestamp < min_time) {
      replace_buf = b;
      min_time = b->timestamp;
    }
  }
  if (replace_buf) {
    // remove from old bucket
    int ridx = hash(replace_buf->dev, replace_buf->blockno);
    acquire(&hashtable[ridx].lock);
    if(replace_buf->refcnt != 0)  // be used in another bucket's local find between finded and acquire
    {
      release(&hashtable[ridx].lock);
      goto refind;
    }
    struct buf *pre = &hashtable[ridx].head;
    struct buf *p = hashtable[ridx].head.next;
    while (p != replace_buf) {
      pre = pre->next;
      p = p->next;
    }
    pre->next = p->next;
    release(&hashtable[ridx].lock);
    // add to current bucket
    replace_buf->next = hashtable[idx].head.next;
    hashtable[idx].head.next = replace_buf;
    release(&bcache.lock);
    // printf("Global %d -> %d %p\n", ridx, idx, replace_buf);
    goto find;
  }
  else {
    panic("bget: no buffers");
  }

  find:
  replace_buf->dev = dev;
  replace_buf->blockno = blockno;
  replace_buf->valid = 0;
  replace_buf->refcnt = 1;
  release(&bucket->lock);
  acquiresleep(&replace_buf->lock);
  return replace_buf;
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

  int idx = hash(b->dev, b->blockno);

  acquire(&hashtable[idx].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->timestamp = ticks;
  }
  
  release(&hashtable[idx].lock);
}

void
bpin(struct buf *b) {
  int idx = hash(b->dev, b->blockno);
  acquire(&hashtable[idx].lock);
  b->refcnt++;
  release(&hashtable[idx].lock);
}

void
bunpin(struct buf *b) {
  int idx = hash(b->dev, b->blockno);
  acquire(&hashtable[idx].lock);
  b->refcnt--;
  release(&hashtable[idx].lock);
}


