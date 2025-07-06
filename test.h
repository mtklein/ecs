#pragma once

#define expect(x) if (!(x)) __builtin_verbose_trap("expect() failed", #x)
#define TODO(x) expect(!(x))
