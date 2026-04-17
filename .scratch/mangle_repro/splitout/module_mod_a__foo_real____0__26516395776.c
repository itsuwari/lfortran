#include "mod_a_generated.h"

void __lfortran_mod_a__foo_real____0(struct r64___* a, int32_t __1a, struct r64___* b)
{
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    __libasr_index_0_1 = 1;
    for (__libasr_index_0_=1; __libasr_index_0_<=1*__1a + 1 - 1; __libasr_index_0_++) {
        b->data[(0 + (1 * (__libasr_index_0_ - 1)))] = a->data[(0 + (1 * (__libasr_index_0_1 - 1)))]*  2.00000000000000000e+00;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
}

