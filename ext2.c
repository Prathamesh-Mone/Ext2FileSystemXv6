#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "ext2_types.h"
#include "ext2_fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define EXT2_IPB (BSIZE/(ext2sb.s_inode_size))
#define BLOCK_SIZE ((1024 << ext2sb.s_log_block_size))
#define EXT2_BPB (BSIZE*8)

//Links for references of macros below.
//https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2uapi_2linux_2stat_8h_source.html
//https://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
#define S_IFMT  00170000
#define S_IFREG  0100000
#define S_IFDIR  0040000
#define S_IFBLK  0060000
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)

struct ext2_super_block ext2sb;
struct ext2_group_desc gd;

struct {
  struct spinlock lock;
  struct inode inode[NINODE];
} icache;

struct inode* iget(uint dev, uint inum);

//Equivalent function of iblock macro.
int iblock_mac(int inode_num, struct ext2_super_block ext2sb){
  int inode_table_index, inode_offset, iblock;
  struct buf *bp;
  bp = bread(2, 1);
  memmove(&gd, bp->data, sizeof(gd));
  brelse(bp);
  inode_table_index = (inode_num - 1);
  inode_offset = (inode_table_index * ext2sb.s_inode_size) + (gd.bg_inode_table * BSIZE);
  iblock = (int)(inode_offset/BSIZE);
  return iblock;
}

//Function to read ext2 superblock
void ext2_readsb(int dev, struct ext2_super_block *ext2sb)
{
  struct buf *bp;
  bp = bread(dev, 0);
  memmove(ext2sb, bp->data+1024, sizeof(*ext2sb));
  brelse(bp);
}

//Taken same function as in fs.c
//Instead of log write, directly do bwrite.
static void
bzero(int dev, int bno)
{
  struct buf *bp;

  bp = bread(dev, bno);
  memset(bp->data, 0, BSIZE);
  bwrite(bp);
  brelse(bp);
}

// Allocate a zeroed disk block.
// Read the block bitmap.
// bi th bit set one in block bitmap after allocation that block.
// As of now my disk has only one group descriptor, so code is written accordingly of balloc.
// No looping over group descriptors is considered.
uint
ext2_balloc(uint dev){
  int bi, m;
  struct buf *bp;

  bp = 0;
  bp = bread(dev, 1);
  memmove(&gd, bp->data, sizeof(gd));
  brelse(bp);
  bp = bread(dev, gd.bg_block_bitmap);
  for(bi = 0; bi < EXT2_BPB; bi++){
    m = 1 << (bi % 8);
    if((bp->data[bi/8] & m) == 0){
      bp->data[bi/8] |= m;
      bwrite(bp);
      brelse(bp);
      bzero(dev, bi);
      return bi;
    }
  }
  brelse(bp);
  panic("balloc: out of blocks");
}

// Free a disk block.
// Got the block group descriptor and set that bit to zero in block bitmap,
// at which block is freed by bfree function. 
void
ext2_bfree(int dev, uint b)
{
  struct buf *bp;
  int bi, m;

  bp = bread(dev, 1);
  memmove(&gd, bp->data, sizeof(gd));
  brelse(bp);
  bp = bread(dev, gd.bg_block_bitmap);
  bi = b % EXT2_BPB;
  m = 1 << (bi % 8);
  if((bp->data[bi/8] & m) == 0)
    panic("freeing free block");
  bp->data[bi/8] &= ~m;
  bwrite(bp);
  brelse(bp);
}

void ext2_iinit(int dev){
  ext2_readsb(dev, &ext2sb);
  cprintf("sb: magic %x icount = %d bcount = %d\n log block size  %d inodes per group  %d first inode %d \
    inode size %d\n", ext2sb.s_magic, ext2sb.s_inodes_count, ext2sb.s_blocks_count, \
    ext2sb.s_log_block_size, ext2sb.s_inodes_per_group, ext2sb.s_first_ino, ext2sb.s_inode_size);
}

//Allocate an inode.
//First read inode bitmap and find free inode.
//Then capture that inode using inode table.
//Call iget to partially initialize it.
struct inode*
ext2_ialloc(uint dev, short type)
{
  int inum, bi, m;
  struct buf *bp;
  struct ext2_inode *dip;
  int inode_table_index;

  bp = bread(dev, 1);
  memmove(&gd, bp->data, sizeof(gd));
  brelse(bp);
  bp = bread(dev, gd.bg_inode_bitmap);
  for(bi = 0; bi < EXT2_BPB; bi++){
    m = 1 << (bi % 8);
    if((bp->data[bi/8] & m) == 0){
      bp->data[bi/8] |= m;
      bwrite(bp);
      brelse(bp);
      inum = bi+1;
      inode_table_index = (inum - 1)%(BSIZE/ext2sb.s_inode_size);
      bp = bread(dev, iblock_mac(inum, ext2sb));
      dip = (struct ext2_inode*)(bp->data + inode_table_index*(ext2sb.s_inode_size));
      if(type == T_FILE){
        dip->i_mode = 33188;
      }
      else if(type == T_DIR){
	dip->i_mode = 16877;
      }
      bwrite(bp);
      brelse(bp);
      return iget(dev, inum);
    }
  }
  panic("ialloc: no inodes");
}

// Copy a modified in-memory inode to disk.
void
ext2_iupdate(struct inode *ip)
{
  struct buf *bp;
  struct ext2_inode *dip;
  int inode_table_index;

  inode_table_index = (ip->inum - 1)%(BSIZE/ext2sb.s_inode_size);
  bp = bread(ip->dev, iblock_mac(ip->inum, ext2sb));
  dip = (struct ext2_inode*)(bp->data + inode_table_index*(ext2sb.s_inode_size));
  dip->i_links_count = ip->nlink;
  dip->i_size = ip->size;
  memmove(dip->i_block, ip->addrs, sizeof(ip->addrs));
  bwrite(bp);
  brelse(bp);
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode*
ext2_idup(struct inode *ip)
{
  acquire(&icache.lock);
  ip->ref++;
  release(&icache.lock);
  return ip;
}

//Initialize in memory inode from on disk inode.
void ext2_ilock(struct inode *ip){
  struct buf *bp;
  struct ext2_inode *dip;
  int inode_table_index = (ip->inum - 1)%(BSIZE/ext2sb.s_inode_size);
  if(ip == 0 || ip->ref < 1)
    panic("ilock");

  acquiresleep(&ip->lock);

  if(ip->valid == 0){
    bp = bread(ip->dev, iblock_mac(ip->inum, ext2sb));
    dip = (struct ext2_inode*)(bp->data + inode_table_index*(ext2sb.s_inode_size));
    if(S_ISREG(dip->i_mode)){
      ip->type = T_FILE;
    }
    else if(S_ISDIR(dip->i_mode)){
      ip->type = T_DIR;
    }
    ip->nlink = dip->i_links_count;
    ip->size = dip->i_size;
    memmove(ip->addrs, dip->i_block, sizeof(ip->addrs));
    brelse(bp);
    ip->valid = 1; //on disk inode
    if(ip->type == 0)
      panic("ilock: no type ext2");
  }
}

void
ext2_iunlock(struct inode *ip)
{
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
    panic("iunlock");

  releasesleep(&ip->lock);
}

void ext2_itrunc(struct inode *ip);

void
ext2_iput(struct inode *ip)
{
  acquiresleep(&ip->lock);
  if(ip->valid && ip->nlink == 0){
    acquire(&icache.lock);
    int r = ip->ref;
    release(&icache.lock);
    if(r == 1){
      // inode has no links and no other references: truncate and free.
      ext2_itrunc(ip);
      ip->type = 0;
      ext2_iupdate(ip);
      ip->valid = 0;
    }
  }
  releasesleep(&ip->lock);

  acquire(&icache.lock);
  ip->ref--;
  release(&icache.lock);
}

// Common idiom: unlock, then put.
void
ext2_iunlockput(struct inode *ip)
{
  ext2_iunlock(ip);
  ext2_iput(ip);
}

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
uint
ext2_bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = ext2_balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;

  if(bn < NINDIRECT){
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = ext2_balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = ext2_balloc(ip->dev);
      bwrite(bp);
    }
    brelse(bp);
    return addr;
  }

  panic("bmap: out of range");
}

void
ext2_itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  uint *a;

  for(i = 0; i < NDIRECT; i++){
    if(ip->addrs[i]){
      ext2_bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    for(j = 0; j < NINDIRECT; j++){
      if(a[j])
        ext2_bfree(ip->dev, a[j]);
    }
    brelse(bp);
    ext2_bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  ext2_iupdate(ip);
}

//Read data from inode.
int
ext2_readi(struct inode *ip, char *dst, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(off > ip->size || off + n < off)	//if off is exceeding bound or n < 0.
    return -1;
  if(off + n > ip->size)		//if off + n exceeds size, read possible number of bytes.
    n = ip->size - off;

  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
    bp = bread(ip->dev, ext2_bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    memmove(dst, bp->data + off%BSIZE, m);
    brelse(bp);
  }
  return n;
}

// Write data to inode.
int
ext2_writei(struct inode *ip, char *src, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > MAXFILE*BSIZE)
    return -1;

  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
    bp = bread(ip->dev, ext2_bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    memmove(bp->data + off%BSIZE, src, m);
    bwrite(bp);
    brelse(bp);
  }

  if(n > 0 && off > ip->size){
    ip->size = off;
    ext2_iupdate(ip);
  }
  return n;
}

// Look for a directory entry in a directory.
//Code is similar as used by me in ext2 assignment.
struct inode*
ext2_dirlookup(struct inode *dp, char *name, uint *poff)
{
  int size;
  uint inum;
  struct ext2_dir_entry_2 *entry;
  char myblock[BSIZE];

  if(dp->type != T_DIR)
    panic("dirlookup not DIR");

  size = 0;
  ext2_readi(dp, myblock, 0, BSIZE);
  entry = (struct ext2_dir_entry_2*)myblock;
  while(size < dp->size){
    inum = entry->inode;
    if(strncmp(entry->name, name, entry->name_len) == 0){
      if(poff)
	 *poff = size;
      return iget(2, inum);
    }
    size = size + entry->rec_len;
    entry = (void*)entry + entry->rec_len;
  }

  return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
// Dirlink is partially working and hence "Create" function is not working
// After doing "cat" on the newly created file, it works, but the changes dont reflect on the disk.
// In short it gets reflected only in buffer.
int
ext2_dirlink(struct inode *dp, char *name, uint inum)
{
  int size,k,l;
  struct ext2_dir_entry_2 *entry, *entry1;
  struct inode *ip;
  char myblock[BSIZE];
  int count = 0;

  // Check that name is not present.
  if((ip = ext2_dirlookup(dp, name, 0)) != 0){
    ext2_iput(ip);
    return -1;
  }

  cprintf("Inode number in dirlink %d\n", inum);
  size = 0;
  ext2_readi(dp, myblock, 0, BSIZE);
  entry = (struct ext2_dir_entry_2*)myblock;
  while(size < dp->size){
    count = count + entry->rec_len;
    cprintf("Rec Length is %d\n", entry->rec_len);
    if(count >= BSIZE){
      break;
    }
    size = size + entry->rec_len;
    entry = (void*)entry + entry->rec_len;
  }
  l = entry->rec_len;
  k = entry->name_len;
  while(k%4){
    k++;
  }
  entry->rec_len = k + 8;
  entry = (void *)entry + entry->rec_len;
  entry->inode = inum;
  entry->rec_len = l - (k + 8);
  strncpy(entry->name, name, strlen(name));
  entry->name_len = strlen(name);
  entry->file_type = 1;
  entry1 = (struct ext2_dir_entry_2*)myblock;
  size = 0;
  int count1 = 0;
  while(size < dp->size){
    cprintf("name is %s\n", entry1->name);
    cprintf("Inode number is %d\n", entry1->inode);
    cprintf("Rec length is %d\n", entry1->rec_len);
    count1 = count1 + entry1->rec_len;
    if(count1 >= BSIZE){
      break;
    }
    size = size + entry1->rec_len;
    entry1 = (void*)entry1 + entry1->rec_len;
  }
  if(writei(dp, (char *)myblock, 0, 2048) != 2048){
    panic("dirlink");
  }
  return 0;
}

