#pragma once

#include <stddef.h>

struct table {
    size_t size;
    void  *data;
    int    n,slots;
    int   *key,*ix;
};

void  table_set  (struct table *table, int key, void const *val);
void  table_del  (struct table *table, int key);
void  table_reset(struct table *table);

void* table_get (struct table const *table, int key);
_Bool table_join(struct table const *table[], int tables, int *key, void *vals);
