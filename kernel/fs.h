// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 11               // 直接块的数量
#define NINDIRECT (BSIZE / sizeof(uint))   // 一级间接块中的块地址数量
#define NDINDIRECT (NINDIRECT * NINDIRECT) // 二级间接块中的一级间接块地址数量
#define MAXFILE (NDIRECT + NINDIRECT + NDINDIRECT) // 文件中最大的数据块数量，包括直接块、一级间接块和二级间接块

// On-disk inode structure
struct dinode {
  short type;           // 文件类型
  short major;          // 主设备号（仅适用于 T_DEVICE 类型）
  short minor;          // 次设备号（仅适用于 T_DEVICE 类型）
  short nlink;          // 文件系统中指向该 inode 的链接数
  uint size;            // 文件大小（字节）
  uint addrs[NDIRECT+2];   // 数据块地址数组，包括直接块、一级间接块和二级间接块
};


// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

