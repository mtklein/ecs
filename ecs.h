#pragma once

#include <stddef.h>

struct table {
    size_t size;
    void  *data;
    int    n,slots;
    int   *dense;
    int   *sparse;
};

void* table_get  (struct table const *table, int key);
void  table_set  (struct table       *table, int key, void const *val);
void  table_drop (struct table       *table, int key);
void  table_clear(struct table       *table);
