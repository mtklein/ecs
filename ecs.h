#pragma once

#include <stddef.h>

struct table {
    size_t size;
    void  *data;
    int    n,slots;
    int   *key,*ix;
};

void  table_set (struct table*       , int key, void const *val);
void  table_del (struct table*       , int key);
void* table_get (struct table const *, int key);

void  table_drop(struct table*);

_Bool table_join(struct table *table[], int tables, int *key, void *vals);
