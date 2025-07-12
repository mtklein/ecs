#include <stdlib.h>

__attribute__((constructor(101)))
static void premain(void) {
    setenv("LLVM_PROFILE_FILE", "%t/tmp.profraw", 0);
}

int main(void) {
    return 0;
}
