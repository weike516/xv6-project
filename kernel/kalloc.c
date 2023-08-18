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
  struct spinlock reflock;
  uint *ref_count;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.reflock,"kmemref");
  // end:内核之后的第一个可以内存单元地址，它在kernel.ld中定义
  uint64 rc_pages = ((PHYSTOP - (uint64)end) >> 12) +1; // 物理页数
  // 计算存放页面引用计数器占用的页数
  rc_pages = ((rc_pages * sizeof(uint)) >> 12) + 1;
  // 从end开始存放页引用计数器，需要rc_pages页
  kmem.ref_count = (uint*)end;
  // 存放计数器的存储空间大小为：
  uint64 rc_offset = rc_pages << 12;  
  freerange(end + rc_offset, (void*)PHYSTOP);
}

// 将地址转换为物理页号
inline int
kgetrefindex(void *pa)
{
   return ((char*)pa - (char*)PGROUNDUP((uint64)end)) >> 12;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    // 初始化kmem.ref_count
     kmem.ref_count[kgetrefindex((void *)p)] = 1;
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

    // 检查要释放的地址是否合法
    if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree"); // 若不合法，触发 panic

    acquire(&kmem.lock); // 获取内存分配锁
    if (--kmem.ref_count[kgetrefindex(pa)]) { // 减少引用计数，如果仍有引用
        release(&kmem.lock); // 释放内存分配锁
        return; // 直接返回
    }
    release(&kmem.lock); // 释放内存分配锁

    // 填充内存块以捕获悬空引用
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa; // 将指针转换为 struct run 指针

    acquire(&kmem.lock); // 获取内存分配锁
    r->next = kmem.freelist; // 将释放的内存块连接到空闲链表
    kmem.freelist = r; // 更新空闲链表头
    release(&kmem.lock); // 释放内存分配锁
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
    kmem.ref_count[kgetrefindex((void*)r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// 获取内存块的引用计数
int kgetref(void *pa) {
    return kmem.ref_count[kgetrefindex(pa)];
}

// 增加内存块的引用计数
void kaddref(void *pa) {
    kmem.ref_count[kgetrefindex(pa)]++;
}

// 获取引用计数锁
inline void acquire_refcnt() {
    acquire(&kmem.reflock);
}

// 释放引用计数锁
inline void release_refcnt() {
    release(&kmem.reflock);
}



