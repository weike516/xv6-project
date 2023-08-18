#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// 定义 sys_trace 系统调用函数
uint64 
sys_trace(void)
{
  uint mask; // 存储传递给系统调用的跟踪掩码
  if(argint(0, (int*)&mask) < 0) // 从系统调用参数中获取跟踪掩码
    return -1; // 参数获取失败，返回错误码
  struct proc *c = myproc(); // 获取当前进程的指针
  c->trace_mask |= mask; // 将传递的跟踪掩码应用于进程的跟踪掩码
  return 0; // 返回成功标志
}

uint64 sys_sysinfo(void)
{
  uint64 info; // 用户空间指针
  struct sysinfo kinfo; // 用于存储内核中的系统信息
  struct proc *p = myproc(); // 获取当前进程的进程控制块（PCB）
  
  if(argaddr(0, &info) < 0){
    return -1; // 获取用户传递的指针参数失败
  }
  
  kinfo.freemem = freemem(); // 获取空闲内存大小
  kinfo.nproc = nproc(); // 获取活动进程数量
  
  // 将内核中的系统信息复制到用户空间
  if(copyout(p->pagetable, info, (char*)&kinfo, sizeof(kinfo)) < 0){
    return -1; // 复制失败
  }
  
  return 0; // 返回成功标志
}

