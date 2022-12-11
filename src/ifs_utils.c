#include "../include/ifs.h"
#include "types.h"
#include <stdint.h>

/*GLOBAL*/
extern struct ifs_super      super;
extern struct custom_options ifs_options;

/*CODE*/
char* ifs_get_fname(const char* path)
{
    char  ch = '/';
    char* q  = strrchr(path, ch) + 1;

    return q;
}

int ifs_calc_lvl(const char* path)
{
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = path;
    int   lvl = 0;

    if (strcmp(path, "/") == 0) {
        return lvl;
    }

    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}

struct ifs_dentry* ifs_get_dentry(struct ifs_inode* inode, int dir)
{
    struct ifs_dentry* dentry_cursor = inode->dentries;
    int                cnt           = 0;
    while (dentry_cursor) {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}

int ifs_driver_read(int offset, uint8_t* out_content, int size)
{
    int      offset_aligned = IFS_ROUND_DOWN(offset, IFS_BLK_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = IFS_ROUND_UP((size + bias), IFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(SFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(IFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0) {
        // read(SFS_DRIVER(), cur, SFS_IO_SZ());
        ddriver_read(IFS_DRIVER(), cur, IFS_IO_SZ());
        cur += IFS_IO_SZ();
        size_aligned -= IFS_IO_SZ();
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return IFS_ERROR_NONE;
}

int ifs_driver_write(int offset, uint8_t* in_content, int size)
{
    int      offset_aligned = IFS_ROUND_DOWN(offset, IFS_BLK_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = IFS_ROUND_UP((size + bias), IFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    ifs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);

    // lseek(SFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(IFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0) {
        // write(SFS_DRIVER(), cur, SFS_IO_SZ());
        ddriver_write(IFS_DRIVER(), cur, IFS_IO_SZ());
        cur += IFS_IO_SZ();
        size_aligned -= IFS_IO_SZ();
    }

    free(temp_content);
    return IFS_ERROR_NONE;
}

int ifs_alloc_data()
{
    int     byte_cursor       = 0;
    int     bit_cursor        = 0;
    int     data_cursor       = 0;
    boolean is_find_free_data = 0;

    for (byte_cursor = 0; byte_cursor < IFS_BLKS_SZ(super.map_data_blks); byte_cursor++) {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if ((super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {
                super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_data = TRUE;

                break;
            }
            data_cursor++;
        }
        if (is_find_free_data) {
            break;
        }
    }
    if (!is_find_free_data || data_cursor >= IFS_BLKS_SZ(super.map_data_blks)) {
        return -IFS_ERROR_NOSPACE;
    }
    return data_cursor;
}

int ifs_alloc_dentry(struct ifs_inode* inode, struct ifs_dentry* dentry)
{
    if (inode->dentries == NULL) {
        inode->dentries = dentry;
    } else {
        dentry->brother = inode->dentries;
        inode->dentries = dentry;
    }
    inode->dir_cnt++;
    inode->size += sizeof(struct ifs_dentry_d);

    if (IFS_ROUND_UP((inode->dir_cnt) * sizeof(struct ifs_dentry_d), IFS_BLK_SZ()) > IFS_BLKS_SZ(inode->block_alloc)) {
        inode->block_pointer[inode->block_alloc] = ifs_alloc_data();
        inode->block_alloc++;
    }

    return inode->dir_cnt;
}

int ifs_drop_dentry(struct ifs_inode* inode, struct ifs_dentry* dentry)
{
    boolean            is_find       = FALSE;
    struct ifs_dentry* dentry_cursor = inode->dentries;

    if (dentry_cursor == dentry) {
        inode->dentries = dentry->brother;
        is_find         = TRUE;
    } else {
        while (dentry_cursor) {
            if (dentry_cursor->brother == dentry) {
                dentry_cursor->brother = dentry->brother;
                is_find                = TRUE;

                break;
            }
            dentry_cursor = dentry_cursor->brother;
        }
    }
    if (!is_find) {
        return -IFS_ERROR_NOTFOUND;
    }
    inode->dir_cnt--;

    return inode->dir_cnt;
}

struct ifs_inode* ifs_alloc_inode(struct ifs_dentry* dentry)
{
    // 返回值
    struct ifs_inode* ret;

    // 遍历用指针
    int byte_cursor = 0;
    int bit_cursor  = 0;
    int ino_cursor  = 0;
    int bno_cursor  = 0;
    int blk_cnt     = 0;

    // FLAGS
    boolean is_find_free_entry = FALSE;
    boolean is_find_free_data  = FALSE;

    // 遍历 Inode Map,寻找空闲的位置分配
    for (byte_cursor = 0; byte_cursor < IFS_BLKS_SZ(super.map_inode_blks); byte_cursor++) {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if ((super.map_inode[byte_cursor] & (1 << bit_cursor)) == 0) {
                super.map_inode[byte_cursor] |= (1 << bit_cursor);
                is_find_free_entry = TRUE;

                break;
            }
            ino_cursor++;
        }

        if (is_find_free_entry) {
            break;
        }
    }

    // 如果没有找到空闲的位置，或占用了最后一个空闲的位置，则说明Inode Map当前无空余
    if (!is_find_free_entry || ino_cursor == super.max_ino) {
        return (struct ifs_inode*)-IFS_ERROR_NOSPACE;
    }

    // 为新建的Inode分配空间
    ret       = (struct ifs_inode*)malloc(sizeof(struct ifs_inode));
    ret->ino  = ino_cursor;
    ret->size = 0;

    // 将新建的Inode分配给当前Dentry
    dentry->inode = ret;
    dentry->ino   = ret->ino;

    // 初始化新建Inode
    ret->dentry      = dentry;
    ret->dir_cnt     = 0;
    ret->dentries    = NULL;
    ret->block_alloc = 0;
    for (blk_cnt = 0; blk_cnt < IFS_DATA_PER_FILE; blk_cnt++) {
        ret->block_pointer[blk_cnt] = (uint8_t*)(-1);
    }

    // 如果新建的Inode为REG_FILE，则为其分配内存空间，并初始化block_pointer数组
    if (IFS_IS_REG(ret)) {
        for (int p_count = 0; p_count < IFS_DATA_PER_FILE; p_count++) {
            ret->block_pointer[p_count] = (uint8_t*)malloc(IFS_BLK_SZ());
        }
    }

    return ret;
}

int ifs_sync_inode(struct ifs_inode* inode)
{
    struct ifs_inode_d  inode_d;
    struct ifs_dentry*  dentry_cursor;
    struct ifs_dentry_d dentry_d;
    int                 ino = inode->ino;
    inode_d.ino             = ino;
    inode_d.size            = inode->size;
    inode_d.ftype           = inode->dentry->ftype;
    inode_d.dir_cnt         = inode->dir_cnt;

    int blk_cnt;
    for (blk_cnt = 0; blk_cnt < IFS_DATA_PER_FILE; blk_cnt++) {
        inode_d.bno[blk_cnt] = inode->bno[blk_cnt];
    }

    // 将inode_d内容通过ddriver写入
    int offset;
    if (ifs_driver_write(IFS_INO_OFS(ino), (uint8_t*)&inode_d, sizeof(struct ifs_inode_d)) != IFS_ERROR_NONE) {
        IFS_DBG("[%s] io error\n", __func__);

        return -IFS_ERROR_IO;
    }

    /* Cycle 1: 写 INODE */
    /* Cycle 2: 写 数据 */
    if (IFS_IS_DIR(inode)) { // 如果当前Inode为目录项
        // 初始化计数器
        blk_cnt       = 0;
        dentry_cursor = inode->dentries;

        while (dentry_cursor != NULL && blk_cnt < inode->block_alloc) {
            offset = IFS_DATA_OFS(inode->bno[blk_cnt]);

            while (dentry_cursor != NULL && blk_cnt < inode->block_alloc) {
                memcpy(dentry_d.name, dentry_cursor->name, IFS_MAX_FILE_NAME);
                dentry_d.ftype = dentry_cursor->ftype;
                dentry_d.ino   = dentry_cursor->ino;

                if (ifs_driver_write(offset, (uint8_t*)&dentry_d, sizeof(struct ifs_dentry_d)) != IFS_ERROR_NONE) {
                    IFS_DBG("[%s] io error\n", __func__);

                    return -IFS_ERROR_IO;
                }

                // 递归调用，将下级目录也写入磁盘
                if (dentry_cursor->inode != NULL) {
                    ifs_sync_inode(dentry_cursor->inode);
                }

                dentry_cursor = dentry_cursor->brother;
                offset += sizeof(struct ifs_dentry_d);

                if (offset >= IFS_DATA_OFS(inode->block_alloc)) {
                    break;
                }
            }

            blk_cnt++;
        }
    } else if (IFS_IS_REG(inode)) { // 如果当前Inode为REG_FILE
        for (blk_cnt = 0; blk_cnt < inode->block_alloc; blk_cnt++) {
            if (ifs_driver_write(IFS_DATA_OFS(ino), inode->block_pointer[blk_cnt], IFS_BLK_SZ()) != IFS_ERROR_NONE) {
                IFS_DBG("[%s] io error\n", __func__);
                return -IFS_ERROR_IO;
            }
        }
    }

    return IFS_ERROR_NONE;
}

struct ifs_inode* ifs_read_inode(struct ifs_dentry* dentry, int ino)
{
    struct ifs_inode*   inode = (struct ifs_inode*)malloc(sizeof(struct ifs_inode));
    struct ifs_inode_d  inode_d;
    struct ifs_dentry*  sub_dentry;
    struct ifs_dentry_d dentry_d;
    int                 blk_cnt = 0, offset, offset_limit;
    int                 dir_cnt = 0, i;

    // 通过ddriver读取内容至inode_d中
    if (ifs_driver_read(IFS_INO_OFS(ino), (uint8_t*)&inode_d, sizeof(struct ifs_inode_d)) != IFS_ERROR_NONE) {
        IFS_DBG("[%s] io error\n", __func__);
        return NULL;
    }

    // inode初始化
    inode->dir_cnt  = 0;
    inode->ino      = inode_d.ino;
    inode->size     = inode_d.size;
    inode->dentry   = dentry;
    inode->dentries = NULL;
    for (blk_cnt = 0; blk_cnt < IFS_DATA_PER_FILE; blk_cnt++) {
        inode->bno[blk_cnt] = inode_d.bno[blk_cnt];
    }

    // 如果inode为目录项
    if (IFS_IS_DIR(inode)) {
        dir_cnt = inode_d.dir_cnt;
        blk_cnt = 0;

        while (dir_cnt != 0) {
            offset       = IFS_DATA_OFS(inode->bno[blk_cnt]);
            offset_limit = IFS_DATA_OFS(inode->bno[blk_cnt] + 1);

            // 遍历当前目录项的每一条子目录
            while (offset + sizeof(struct ifs_dentry_d) < offset_limit) {
                if (ifs_driver_read(offset, (uint8_t*)&dentry_d, sizeof(struct ifs_dentry_d))
                    != IFS_ERROR_NONE) {
                    IFS_DBG("[%s] io error\n", __func__);

                    return NULL;
                }
                sub_dentry         = new_dentry(dentry_d.name, dentry_d.ftype);
                sub_dentry->parent = inode->dentry;
                sub_dentry->ino    = dentry_d.ino;
                ifs_alloc_dentry(inode, sub_dentry);

                offset += sizeof(struct ifs_dentry_d);

                if (--dir_cnt == 0) {
                    break;
                }
            }

            blk_cnt++;
        }
    } else if (IFS_IS_REG(inode)) {
        for (blk_cnt = 0; blk_cnt < inode->block_alloc; blk_cnt++) {
            inode->block_pointer[blk_cnt] = (uint8_t*)malloc(IFS_BLK_SZ());
            if (ifs_driver_read(IFS_DATA_OFS(inode->bno[blk_cnt]), inode->block_pointer[blk_cnt],
                    IFS_BLKS_SZ(IFS_DATA_PER_FILE))
                != IFS_ERROR_NONE) {
                IFS_DBG("[%s] io error\n", __func__);

                return NULL;
            }
        }
    }

    return inode;
}

struct ifs_dentry* ifs_lookup(const char* path, boolean* is_find, boolean* is_root)
{
    struct ifs_dentry* dentry_cursor = super.root_dentry;
    struct ifs_dentry* dentry_ret    = NULL;
    struct ifs_inode*  inode;
    int                total_lvl = ifs_calc_lvl(path);
    int                lvl       = 0;
    boolean            is_hit;
    char*              fname    = NULL;
    char*              path_cpy = (char*)malloc(sizeof(path));
    *is_root                    = FALSE;
    strcpy(path_cpy, path);

    if (total_lvl == 0) { /* 根目录 */
        *is_find   = TRUE;
        *is_root   = TRUE;
        dentry_ret = super.root_dentry;
    }
    fname = strtok(path_cpy, "/");
    while (fname) {
        lvl++;
        if (dentry_cursor->inode == NULL) { /* Cache机制 */
            ifs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;

        // 如果还没有到达total_lvl时inode类型就为REG_FILE，说明目录非法
        if (IFS_IS_REG(inode) && lvl < total_lvl) {
            IFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (IFS_IS_DIR(inode)) {
            dentry_cursor = inode->dentries;
            is_hit        = FALSE;

            // 遍历brother，寻找目标
            while (dentry_cursor) {
                if (memcmp(dentry_cursor->name, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }

            // 如果没找到，说明当前层级无目标，非法
            if (!is_hit) {
                *is_find = FALSE;
                IFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find   = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/");
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = ifs_read_inode(dentry_ret, dentry_ret->ino);
    }

    return dentry_ret;
}

int ifs_mount(struct custom_options options)
{
    int                ret = IFS_ERROR_NONE;
    int                driver_fd;
    struct ifs_super_d ifs_super_d;
    struct ifs_dentry* root_dentry;
    struct ifs_inode*  root_inode;

    int                inode_num;
    int                map_inode_blks;
    int                map_data_blks;

    int                super_blks;
    boolean            is_init = FALSE;

    super.is_mounted           = FALSE;

    driver_fd                  = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    super.driver_fd = driver_fd;
    ddriver_ioctl(IFS_DRIVER(), IOC_REQ_DEVICE_SIZE, &super.sz_disk);
    ddriver_ioctl(IFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &super.sz_io);
    super.sz_blk = super.sz_io * 2;

    // 创建根目录
    root_dentry = new_dentry("/", IFS_DIR);

    if (ifs_driver_read(IFS_SUPER_OFS, (uint8_t*)(&ifs_super_d), sizeof(struct ifs_super_d)) != IFS_ERROR_NONE) {
        return -IFS_ERROR_IO;
    }

    /* 读取super */
    if (ifs_super_d.magic_num != IFS_MAGIC_NUM) { /* 幻数无 */
                                                  /* 指定各部分大小 */
        super_blks     = IFS_SUPER_BLKS;
        map_inode_blks = IFS_MAP_INODE_BLKS;
        map_data_blks  = IFS_MAP_DATA_BLKS;
        inode_num      = IFS_INODE_BLKS;
        data_num       = IFS_DATA_BLKS;

        /* 布局layout */
        super.max_ino                = inode_num;
        ifs_super_d.map_inode_offset = IFS_SUPER_OFS + IFS_BLKS_SZ(super_blks);
        ifs_super_d.map_data_offset  = ifs_super_d.map_inode_offset + IFS_BLKS_SZ(map_inode_blks);

        ifs_super_d.inode_offset     = ifs_super_d.map_data_offset + IFS_BLKS_SZ(map_data_blks);
        ifs_super_d.data_offset      = ifs_super_d.inode_offset + IFS_BLKS_SZ(inode_num);

        ifs_super_d.map_inode_blks   = map_inode_blks;
        ifs_super_d.map_data_blks    = map_data_blks;

        ifs_super_d.sz_usage         = 0;

        is_init                      = TRUE;
    }
    super.sz_usage         = ifs_super_d.sz_usage; /* 建立 in-memory 结构 */

    super.map_inode        = (uint8_t*)malloc(IFS_BLKS_SZ(ifs_super_d.map_inode_blks));
    super.map_inode_blks   = ifs_super_d.map_inode_blks;
    super.map_inode_offset = ifs_super_d.map_inode_offset;

    super.map_data         = (uint8_t*)malloc(IFS_BLKS_SZ(ifs_super_d.map_data_blks));
    super.map_data_blks    = ifs_super_d.map_data_blks;
    super.map_data_offset  = ifs_super_d.map_data_offset;

    super.inode_offset     = ifs_super_d.inode_offset;
    super.data_offset      = ifs_super_d.data_offset;

    if (ifs_driver_read(ifs_super_d.map_inode_offset, (uint8_t*)(super.map_inode),
            IFS_BLKS_SZ(ifs_super_d.map_inode_blks))
        != IFS_ERROR_NONE) {
        return -IFS_ERROR_IO;
    }

    if (is_init) { /* 分配根节点 */
        root_inode = ifs_alloc_inode(root_dentry);
        ifs_sync_inode(root_inode);
    }

    root_inode         = ifs_read_inode(root_dentry, IFS_ROOT_INO);
    root_dentry->inode = root_inode;
    super.root_dentry  = root_dentry;
    super.is_mounted   = TRUE;

    // ifs_dump_map();
    return ret;
}

int ifs_umount()
{
    struct ifs_super_d ifs_super_d;

    // 如果没有被装载，则无需卸载
    if (!super.is_mounted) {
        return IFS_ERROR_NONE;
    }

    ifs_sync_inode(super.root_dentry->inode); /* 从根节点向下刷写节点 */

    // 初始化ifs_super_d
    ifs_super_d.magic_num        = IFS_MAGIC_NUM;
    ifs_super_d.sz_usage         = super.sz_usage;

    ifs_super_d.map_inode_blks   = super.map_inode_blks;
    ifs_super_d.map_inode_offset = super.map_inode_offset;
    ifs_super_d.map_data_blks    = super.map_data_blks;
    ifs_super_d.map_data_offset  = super.map_data_offset;

    ifs_super_d.inode_offset     = super.inode_offset;
    ifs_super_d.data_offset      = super.data_offset;

    // 通过ddriver写入
    if (ifs_driver_write(IFS_SUPER_OFS, (uint8_t*)&ifs_super_d, sizeof(struct ifs_super_d)) != IFS_ERROR_NONE) {
        return -IFS_ERROR_IO;
    }

    if (ifs_driver_write(ifs_super_d.map_inode_offset, (uint8_t*)(super.map_inode), IFS_BLKS_SZ(ifs_super_d.map_inode_blks))
        != IFS_ERROR_NONE) {
        return -IFS_ERROR_IO;
    }

    if (ifs_driver_write(ifs_super_d.map_data_offset, (uint8_t*)super.map_data, IFS_BLKS_SZ(ifs_super_d.map_data_blks))
        != IFS_ERROR_NONE) {
        return -IFS_ERROR_IO;
    }

    free(super.map_inode);
    free(super.map_data);
    ddriver_close(IFS_DRIVER());

    return IFS_ERROR_NONE;
}