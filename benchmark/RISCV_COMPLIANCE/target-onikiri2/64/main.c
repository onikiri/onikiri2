#include <stdio.h>
#include <stdint.h>

extern void onikiri2_test_main();

void onikiri2_rvtest_print_result(uint32_t* begin, uint32_t* end)
{
    for (uint32_t* i = begin; i < end; i++) {
        printf("%08x\n", *i);
    }
}


int main(int argc, const char* argv[])
{
    onikiri2_test_main();
    return 0;
}
