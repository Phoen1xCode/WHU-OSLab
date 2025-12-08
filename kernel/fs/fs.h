// On-disk file system format.
// Both the kernel and user programs use this header file.
#include "../include/types.h"

#ifndef FS_H
#define FS_H

#define ROOTINO 1 // root i-number

// Disk layout:
// [ boot block | super block | log | inode blocks | free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;      // 魔数：0x10203040，用于验证文件系统
  uint size;       // 文件系统总块数
  uint nblocks;    // 数据块数量
  uint ninodes;    // inode 数量
  uint nlog;       // 日志块数量
  uint logstart;   // 第一个日志块的块号
  uint inodestart; // 第一个 inode 块的块号
  uint bmapstart;  // 第一个位图块的块号
};

#define FSMAGIC 0x10203040
#define BSIZE 1024                       // 块大小：1KB
#define NDIRECT 12                       // 直接块数量 12
#define NINDIRECT (BSIZE / sizeof(uint)) // 间接块大小 256
#define MAXFILE (NDIRECT + NINDIRECT)    // 最大文件大小：(12 + 256) * 1KB

// File types
#define T_DIR 1    // Directory
#define T_FILE 2   // File
#define T_DEVICE 3 // Device

// On-disk inode structure 磁盘 Inode(dinode)
struct dinode {
  short type;  // 文件类型 0=空闲, T_DIR=目录, T_FILE=文件, T_DEVICE=设备
  short major; // 主设备号（仅设备文件）
  short minor; // 次设备号（仅设备文件）
  short nlink; // 硬链接计数
  uint size;   // 文件大小（字节）
  uint addrs[NDIRECT + 1]; // 数据块地址数组
  /*
    addrs[0..11]  → 直接块 (12 个)
    addrs[12]     → 间接块指针 → 指向包含 256 个块地址的块
  */
};

// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb) ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB (BSIZE * 8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b) / BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

// The name field may have DIRSIZ characters and not end in a NUL character.
// 目录项
struct dirent {
  ushort inum;                                  // inode 编号 0表示空闲
  char name[DIRSIZ] __attribute__((nonstring)); // 文件名 最多DIRSIZ个字符
};

#endif // FS_H