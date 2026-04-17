#ifndef MOD_A_GENERATED_H
#define MOD_A_GENERATED_H

#include <inttypes.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <lfortran_intrinsics.h>



struct dimension_descriptor
{
    int32_t lower_bound, length, stride;
};

struct r64___
{
    double *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};

void __lfortran_mod_a__foo_real____0(struct r64___* a, int32_t __1a, struct r64___* b);


#endif
