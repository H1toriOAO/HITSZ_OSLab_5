#include "../include/ifs.h"

extern struct ifs_super      super;
extern struct custom_options ifs_options;

/*DEBUG*/
void ifs_dump_map()
{
    int byte_cursor = 0;
    int bit_cursor  = 0;

    for (byte_cursor = 0; byte_cursor < IFS_BLKS_SZ(super.map_inode_blks);
         byte_cursor += 4) {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor + 1] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor + 2] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_inode[byte_cursor + 3] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\n");
    }

    printf("\n");

    for (byte_cursor = 0; byte_cursor < IFS_BLKS_SZ(super.map_data_blks);
         byte_cursor += 4) {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_data[byte_cursor] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_data[byte_cursor + 1] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_data[byte_cursor + 2] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (super.map_data[byte_cursor + 3] & (0x1 << bit_cursor)) >> bit_cursor);
        }
        printf("\n");
    }
}