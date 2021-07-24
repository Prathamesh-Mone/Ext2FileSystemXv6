static void itrunc(struct inode*);
static uint balloc(uint);
static uint bmap(struct inode*, uint);
static void bfree(int, uint);

//Prototype declarations for ext2.c file.

//void 	ext2_readsb(int, struct ext2_super_block*);
struct inode* ext2_dirlookup(struct inode*, char*,uint*);
int 	ext2_dirlink(struct inode*, char*, uint);
struct inode* ext2_ialloc(uint, short);
struct inode* ext2_idup(struct inode*);
void 	ext2_iinit(int);
void 	ext2_ilock(struct inode*);
void 	ext2_itrunc(struct inode*);
void 	ext2_iput(struct inode*);
void 	ext2_iunlock(struct inode*);
void 	ext2_iunlockput(struct inode*);
void 	ext2_iupdate(struct inode*);
int 	ext2_readi(struct inode*, char*, uint, uint);
//void 	ext2_stati(struct inode*, struct stat*);
int 	ext2_writei(struct inode*, char*, uint, uint);
uint 	ext2_balloc(uint);
uint 	ext2_bmap(struct inode*, uint);
void 	ext2_bfree(int, uint);

