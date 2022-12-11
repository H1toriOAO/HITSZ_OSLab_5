#ifndef _TYPES_H_
#define _TYPES_H_

/******************************************************************************
 * SECTION: Type def
 *******************************************************************************/
#include <stdint.h>
typedef int      boolean;
typedef uint16_t flag16;

typedef enum ifs_file_type {
    IFS_REG_FILE,
    IFS_DIR,
    IFS_SYM_LINK
} IFS_FILE_TYPE;

/******************************************************************************
 * SECTION: Macro
 *******************************************************************************/
#define MAX_NAME_LEN          128
#define TRUE                  1
#define FALSE                 0
#define UINT32_BITS           32
#define UINT8_BITS            8

#define IFS_MAGIC_NUM         0x00114514
#define IFS_SUPER_OFS         0
#define IFS_ROOT_INO          0

#define IFS_SUPER_BLKS        1
#define IFS_MAP_INODE_BLKS    1
#define IFS_MAP_DATA_BLKS     1
#define IFS_INODE_BLKS        512
#define IFS_DATA_BLKS         2048

#define IFS_ERROR_NONE        0
#define IFS_ERROR_ACCESS      EACCES
#define IFS_ERROR_SEEK        ESPIPE
#define IFS_ERROR_ISDIR       EISDIR
#define IFS_ERROR_NOSPACE     ENOSPC
#define IFS_ERROR_EXISTS      EEXIST
#define IFS_ERROR_NOTFOUND    ENOENT
#define IFS_ERROR_UNSUPPORTED ENXIO
#define IFS_ERROR_IO          EIO    /* Error Input/Output */
#define IFS_ERROR_INVAL       EINVAL /* Invalid Args */

#define IFS_MAX_FILE_NAME     128
#define IFS_INODE_PER_FILE    1
#define IFS_DATA_PER_FILE     6
#define IFS_DEFAULT_PERM      0777

#define IFS_IOC_MAGIC         'S'
#define IFS_IOC_SEEK          _IO(IFS_IOC_MAGIC, 0)

#define IFS_FLAG_BUF_DIRTY    0x1
#define IFS_FLAG_BUF_OCCUPY   0x2

/******************************************************************************
 * SECTION: Macro Function
 *******************************************************************************/
#define IFS_IO_SZ()   (super.sz_io)
#define IFS_BLK_SZ()  (super.sz_blk)
#define IFS_DISK_SZ() (super.sz_disk)
#define IFS_DRIVER()  (super.driver_fd)

#define IFS_ROUND_DOWN(value, round) \
    (value % round == 0 ? value : (value / round) * round)
#define IFS_ROUND_UP(value, round) \
    (value % round == 0 ? value : (value / round + 1) * round)

#define IFS_BLKS_SZ(blks) (blks * IFS_BLK_SZ())
#define IFS_ASSIGN_FNAME(pifs_dentry, _fname) \
    memcpy(pifs_dentry->name, _fname, strlen(_fname))
#define IFS_INO_OFS(ino)   (super.inode_offset + ino * IFS_BLK_SZ())
#define IFS_DATA_OFS(ino)  (super.data_offset + ino * IFS_BLK_SZ())

#define IFS_IS_DIR(pinode) (pinode->dentry->ftype == IFS_DIR)
#define IFS_IS_REG(pinode) (pinode->dentry->ftype == IFS_REG_FILE)

/******************************************************************************
 * SECTION: FS Specific Structure - In memory structure
 *******************************************************************************/
struct ifs_dentry;
struct ifs_inode;
struct ifs_super;

struct custom_options {
    const char* device;
};

struct ifs_super {
    int                driver_fd;

    int                sz_io;
    int                sz_blk;
    int                sz_disk;
    int                sz_usage;

    int                max_ino;
    int                max_data;

    uint8_t*           map_inode;
    int                map_inode_blks;
    int                map_inode_offset;

    uint8_t*           map_data;
    int                map_data_blks;
    int                map_data_offset;

    int                inode_offset;
    int                data_offset;

    boolean            is_mounted;

    struct ifs_dentry* root_dentry;
};

struct ifs_inode {
    uint32_t           ino;
    int                size;                             // 文件占用空间
    int                dir_cnt;                          // 目录项的数量
    struct ifs_dentry* dentry;                           // 指向当前inode的父dentry
    struct ifs_dentry* dentries;                         //
    uint8_t*           block_pointer[IFS_DATA_PER_FILE]; //
    int                bno[IFS_DATA_PER_FILE];           //
    int                block_alloc;                      // 当前已分配的数据块数
};

struct ifs_dentry {
    char               name[MAX_NAME_LEN]; // 文件名
    uint32_t           ino;                //
    struct ifs_dentry* parent;             // 父亲inode的dentry
    struct ifs_dentry* brother;            // 下一个兄弟inode的dentry
    struct ifs_inode*  inode;              // 指向inode
    IFS_FILE_TYPE      ftype;
};

static inline struct ifs_dentry* new_dentry(char* fname, IFS_FILE_TYPE ftype)
{
    struct ifs_dentry* dentry = (struct ifs_dentry*)malloc(sizeof(struct ifs_dentry));
    memset(dentry, 0, sizeof(struct ifs_dentry));
    IFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;

    return dentry;
}

/******************************************************************************
 * SECTION: FS Specific Structure - Disk structure
 *******************************************************************************/

struct ifs_super_d {
    uint32_t magic_num;
    int      sz_usage;

    int      map_inode_blks;   // inode位图占用的块数
    int      map_inode_offset; // inode位图在磁盘上的偏移

    int      map_data_blks;   // data位图占用的块数
    int      map_data_offset; // data位图在磁盘上的偏移

    int      inode_offset; // 索引结点的偏移
    int      data_offset;  // 数据块的偏移
};

struct ifs_inode_d {
    uint32_t      ino;
    int           size;
    int           dir_cnt;
    IFS_FILE_TYPE ftype;
    int           bno[IFS_DATA_PER_FILE];
};

struct ifs_dentry_d {
    char          name[MAX_NAME_LEN];
    IFS_FILE_TYPE ftype;
    uint32_t      ino;
};
#endif /* _TYPES_H_ */