#pragma once

_Bool lookup_(int  id, ...);
_Bool survey_(int *id, ...);
void  update_(int  id, ...);
void   erase_(int  id, ...);

#define lookup(id, ...) lookup_(id, __VA_ARGS__, NULL)
#define survey(id, ...) survey_(id, __VA_ARGS__, NULL)
#define update(id, ...) update_(id, __VA_ARGS__, NULL)
#define  erase(id, ...)  erase_(id, __VA_ARGS__, NULL)
