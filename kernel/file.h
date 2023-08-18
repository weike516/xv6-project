struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// in-memory copy of an inode
// In-memory inode structure
struct inode {
  uint dev;           // 设备号
  uint inum;          // inode 编号
  int ref;            // 引用计数
  struct sleeplock lock; // 保护以下所有内容的睡眠锁
  int valid;          // inode 是否已从磁盘读取？

  short type;         // 磁盘 inode 的副本
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+2];   // 数据块地址数组，包括直接块、一级间接块和二级间接块
};


// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
