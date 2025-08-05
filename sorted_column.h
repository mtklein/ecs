#pragma once

#include "column.h"

struct column* sorted_column(size_t size,
                             int (*cmp)(int, void const*, int, void const*));
