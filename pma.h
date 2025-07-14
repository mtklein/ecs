#pragma once
#include <stddef.h>

struct pma {
    size_t size;
    void  *data;
    int    n,cap;
    int   *id;
};

void pma_reset(struct pma*);

void  pma_attach(int id, struct pma*, void const *val);
void  pma_detach(int id, struct pma*);
void* pma_lookup(int id, struct pma const*);
