#pragma once

#include <stdarg.h>
#include <stddef.h>

struct table {
    size_t const     *column_size;
    int               columns;
    int               :32;
    struct component *comp;
};
void drop_table(struct table *);

_Bool lookup(struct table const*, int  id, void       *data, .../*columns,~0*/);
_Bool survey(struct table const*, int *id, void       *data, .../*columns,~0*/);
void  update(struct table      *, int  id, void const *data, .../*columns,~0*/);

_Bool vlookup(struct table const*, int  id, void       *data, va_list columns);
_Bool vsurvey(struct table const*, int *id, void       *data, va_list columns);
void  vupdate(struct table      *, int  id, void const *data, va_list columns);
