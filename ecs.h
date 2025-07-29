#pragma once
#include <stddef.h>

void* component_attach(void*, size_t, int id);
void  component_detach(void*, size_t, int id);
void* component_lookup(void*, size_t, int id);
void  component_free(void*);

int   component_n (void const*);
int   component_ix(void const*, int id);
int   component_id(void const*, int ix);
