#ifndef _IFS_H_
#define _IFS_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"

#define IFS_MAGIC
#define IFS_DEFAULT_PERM 0777 /* 全权限打开 */

/******************************************************************************
 * SECTION: macro debug
 *******************************************************************************/
#define IFS_DBG(fmt, ...)                       \
    do {                                        \
        printf("IFS_DBG: " fmt, ##__VA_ARGS__); \
    } while (0)
/******************************************************************************
 * SECTION: ifs.c
 *******************************************************************************/
void* ifs_init(struct fuse_conn_info*);
void  ifs_destroy(void*);
int   ifs_mkdir(const char*, mode_t);
int   ifs_getattr(const char*, struct stat*);
int   ifs_readdir(const char*, void*, fuse_fill_dir_t, off_t,
      struct fuse_file_info*);
int   ifs_mknod(const char*, mode_t, dev_t);
int   ifs_write(const char*, const char*, size_t, off_t,
      struct fuse_file_info*);
int   ifs_read(const char*, char*, size_t, off_t,
      struct fuse_file_info*);
int   ifs_access(const char*, int);
int   ifs_unlink(const char*);
int   ifs_rmdir(const char*);
int   ifs_rename(const char*, const char*);
int   ifs_utimens(const char*, const struct timespec tv[2]);
int   ifs_truncate(const char*, off_t);

int   ifs_open(const char*, struct fuse_file_info*);
int   ifs_opendir(const char*, struct fuse_file_info*);

/******************************************************************************
 * SECTION: ifs_utils.c
 *******************************************************************************/
int                ifs_mount(struct custom_options);
int                ifs_driver_read(int, uint8_t*, int);
int                ifs_driver_write(int, uint8_t*, int);
int                ifs_sync_inode(struct ifs_inode*);
struct ifs_inode*  ifs_read_inode(struct ifs_dentry*, int);
int                ifs_umount();
struct ifs_dentry* ifs_lookup(const char*, boolean*, boolean*);
struct ifs_inode*  ifs_alloc_inode(struct ifs_dentry*);
int                ifs_alloc_dentry(struct ifs_inode*, struct ifs_dentry*);
char*              ifs_get_fname(const char*);
struct ifs_dentry* ifs_get_dentry(struct ifs_inode*, int);

/******************************************************************************
 * SECTION: ifs_debug.c
 *******************************************************************************/
void ifs_dump_map();
#endif /* _ifs_H_ */