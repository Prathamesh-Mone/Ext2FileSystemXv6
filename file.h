struct inode_operations{
        //void (*readsb)(int, struct ext2_super_block*);
        struct inode* (*dirlookup)(struct inode*, char*,uint*);
        int (*dirlink)(struct inode*, char*, uint);
        struct inode* (*ialloc)(uint, short);
        struct inode* (*idup)(struct inode*);
        void (*iinit)(int);
        void (*ilock) (struct inode*);
        void (*itrunc) (struct inode*);
        void (*iput) (struct inode*);
        void (*iunlock) (struct inode*);
        void (*iunlockput) (struct inode*);
        void (*iupdate) (struct inode*);
        int (*readi) (struct inode*, char*, uint, uint);
        //void (*stati) (struct inode*, struct stat*);
        int (*writei) (struct inode*, char*, uint, uint);
        uint (*balloc) (uint);
        uint (*bmap) (struct inode*, uint);
        void (*bfree) (int, uint);
};

//static int isdirempty(struct inode*);

struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};


// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  struct inode_operations *iops;
  uint size;
  uint addrs[NDIRECT+1];
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
