#pragma once
#include <stddef.h>

void* component_attach(void*, size_t, int id);
void  component_detach(void*, size_t, int id);
void* component_lookup(void*, size_t, int id);
void  component_free(void*);

int   component_count   (void const*);
int   component_capacity(void const*);
int   component_index   (void const*, int id);
int   component_id_at   (void const*, int ix);
