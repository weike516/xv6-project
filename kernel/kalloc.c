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
  char lock_name[8];
} kmem[NCPU];

void
kinit()
{
  // 循环初始化每个 CPU 的 kmem 数据结构
  for(int i = 0; i < NCPU; i++) {
    // 使用 snprintf 将当前 CPU 的编号加入到 lock_name 中
    snprintf(kmem[i].lock_name, sizeof(kmem[i].lock_name), "kmem_%d", i);
    // 调用 initlock 初始化当前 CPU 的 kmem.lock 锁
    initlock(&kmem[i].lock, kmem[i].lock_name);
  }
  
  // 调用 freerange 初始化可用物理内存范围
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)


void
kfree(void *pa)
{
  struct run *r;
  
  // 首先进行一些检查，确保传入的物理地址是合法的
  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // 将释放的内存填充为垃圾值，以便在使用后能够更容易地发现
  memset(pa, 1, PGSIZE);

  // 将物理地址强制类型转换为 struct run 类型，struct run 用于表示空闲内存块
  r = (struct run*)pa;

  // 关闭中断，确保原子操作
  push_off();
  // 获取当前 CPU 的编号
  int cpu_id = cpuid();
  // 获取当前 CPU 的 kmem 锁，保护内存分配数据结构
  acquire(&kmem[cpu_id].lock);
  // 将释放的内存块添加到当前 CPU 的空闲链表中
  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  // 释放当前 CPU 的 kmem 锁
  release(&kmem[cpu_id].lock);
  // 恢复中断
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
    struct run *r;

    // 关闭中断，确保原子操作
    push_off();
    // 获取当前 CPU 的编号
    int cpu_id = cpuid();
    // 获取当前 CPU 的 kmem 锁，保护内存分配数据结构
    acquire(&kmem[cpu_id].lock);
    // 从当前 CPU 的空闲链表中取出一个空闲内存块
    r = kmem[cpu_id].freelist;
    if(r)
       kmem[cpu_id].freelist = r->next;
    // 释放当前 CPU 的 kmem 锁
    release(&kmem[cpu_id].lock);
    // 恢复中断
    pop_off();

    // 如果当前 CPU 没有空闲内存块，从其他 CPU 的空闲链表中偷取一个
    if(!r) {
        for(int i=0; i<NCPU; i++) {
           acquire(&kmem[i].lock);
           r = kmem[i].freelist;
           if(r) {
              kmem[i].freelist = r->next;
              release(&kmem[i].lock);
              break;
           }
           release(&kmem[i].lock);
        }
    }

    // 如果成功获得内存块，将其填充为垃圾值
    if(r)
        memset((char*)r, 5, PGSIZE);

    return (void*)r; // 返回分配的内存块的指针
}

