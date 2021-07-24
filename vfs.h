struct inode_operations{
        //void (*readsb)(int, struct ext2_super_block*);
        void (*namecmp) (const char*, const char*);
        struct inode* (*namei) (char*);
        struct inode* (*nameiparent) (char*, char*);
        struct inode* (*dirlookup)(struct inode*, char*,uint*);
        int (*dirlink)(struct inode*, char*, uint);
        //(*unlink) (struct inode*, uint);
        static int (*isdirempty) (struct inode*);
        struct inode* (*ialloc)(unit, short);
        struct inode* (*idup)(struct inode*);
        void (*iinit)(int);
        void (*ilock) (struct inode*);
        static void (*itrunc) (struct inode*);
        void (*iput) (struct inode*);
        void (*iunlock) (struct inode*);
        void (*iunlockput) (struct inode*);
        void (*iupdate) (struct inode*);
        int (*readi) (struct inode*, char*, uint, uint);
        void (*stati) (struct inode*, struct stat*);
        int (*writei) (struct inode*, char*, uint, uint);
        static uint (*balloc) (uint);
        static uint (*bmap) (struct inode*, uint);
        static void (*bfree) (int, uint);
};

/*struct inode_operations xv6_iops = {
        //.readsb = readsb,
        //.namecmp = namecmp,
        .namei = namei,
        .nameiparent = nameiparent,
        .dirlookup = dirlookup,
        .dirlink = dirlink,
        //unlink
        .isdirempty = isdirempty,
        .ialloc = ialloc,
        .idup = idup,
        .iinit = iinit,
        .ilock = ilock,
        .itrunc = itrunc,
        .iput = iput,
        .iunlock = iunlock,
        .iunlockput = iunlockput,
        .iupdate = iupdate,
        .readi = readi,
        .stati = stati,
        .writei = writei,
        .balloc = balloc,
        .bmap = bmap,
        .bfree = bfree
};*/


