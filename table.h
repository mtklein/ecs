#pragma once

#include <stdarg.h>
#include <stddef.h>

struct table {
    size_t const *column;
};

_Bool lookup(struct table const*, int  id, void       *data, .../*columns*/);
_Bool survey(struct table const*, int *id, void       *data, .../*columns*/);
void  update(struct table      *, int  id, void const *data, .../*columns*/);

_Bool vlookup(struct table const*, int  id, void       *data, va_list columns);
_Bool vsurvey(struct table const*, int *id, void       *data, va_list columns);
void  vupdate(struct table      *, int  id, void const *data, va_list columns);
