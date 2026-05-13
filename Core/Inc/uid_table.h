#ifndef __UID_TABLE_H
#define __UID_TABLE_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t uid;
    const uint8_t *name_gb2312;
    uint8_t  name_len;
} uid_entry_t;

extern const uid_entry_t uid_table[];
extern const uint8_t uid_table_size;

const uid_entry_t *UIDTable_Lookup(uint32_t uid);

#endif
