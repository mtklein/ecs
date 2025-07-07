#pragma once

#include <stddef.h>

struct table {
    size_t size;
    void  *data;
    int    n,slots;
    int   *key,*ix;
};

void* table_get  (struct table const *table, int key);
void  table_set  (struct table       *table, int key, void const *val);
void  table_del  (struct table       *table, int key);
void  table_reset(struct table       *table);

typedef void join_cb(int key, void const *a_val, void const *b_val, void *ctx);
void  table_join(struct table const *a, struct table const *b,
                 join_cb *cb, void *ctx);

typedef void join_many_cb(int key, void const *const*vals, void *ctx);
void  table_join_many(struct table const **tables, int count,
                      join_many_cb *cb, void *ctx);
