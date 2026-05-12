#include "uid_table.h"

/*
 * GB2312-encoded name table.
 * Add entries here. Example encoding:
 *   "卡片张三" = {0xBF, 0xA8, 0xC6, 0xAC, 0xD5, 0xC5, 0xC8, 0xFD}
 */
const uid_entry_t uid_table[] = {
    /* {UID (hex), GB2312 bytes, byte length} */
};

const uint8_t uid_table_size = sizeof(uid_table) / sizeof(uid_table[0]);

const uid_entry_t *UIDTable_Lookup(uint32_t uid)
{
    for (uint8_t i = 0; i < uid_table_size; i++) {
        if (uid_table[i].uid == uid) {
            return &uid_table[i];
        }
    }
    return NULL;
}
