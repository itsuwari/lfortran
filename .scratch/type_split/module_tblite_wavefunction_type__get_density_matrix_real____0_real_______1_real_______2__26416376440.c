#include "type_generated.h"

void __lfortran_tblite_wavefunction_type__get_density_matrix_real____0_real_______1_real_______2(struct r64___* focc, int32_t __1focc, struct r64_____* coeff, int32_t __1coeff, int32_t __2coeff, struct r64_____* pmat, int32_t __1pmat, int32_t __2pmat)
{
    int32_t __lcompilers_i_0;
    int32_t __lcompilers_i_1;
    struct r64_____ __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2_value;
    struct r64_____* __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2 = &__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2_value;
    char * __libasr_created_variable_ = NULL;
    double __libasr_created_variable_1;
    double __libasr_created_variable_2;
    int32_t iao;
    int32_t jao;
    struct r64_____ scratch_value;
    struct r64_____* scratch = &scratch_value;
    double *scratch_data;
    scratch->data = scratch_data;
    scratch->n_dims = 2;
    scratch->offset = 0;
    scratch->dims[1].lower_bound = 1;
    scratch->dims[1].length = 0;
    scratch->dims[1].stride = 1;
    scratch->dims[0].lower_bound = 1;
    scratch->dims[0].length = 0;
    scratch->dims[0].stride = (1*0);
    scratch->n_dims = 2;
    scratch->dims[1].lower_bound = 1;
    scratch->dims[1].length = __2pmat;
    scratch->dims[1].stride = 1;
    scratch->dims[0].lower_bound = 1;
    scratch->dims[0].length = __1pmat;
    scratch->dims[0].stride = (1 * __2pmat);
    scratch->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*scratch->dims[1].length*scratch->dims[0].length*sizeof(double));
    scratch->is_allocated = true;
    for (iao=1; iao<=__1pmat; iao++) {
        for (jao=1; jao<=__2pmat; jao++) {
            scratch->data[(((0 + (scratch->dims[0].stride * (jao - scratch->dims[0].lower_bound))) + (scratch->dims[1].stride * (iao - scratch->dims[1].lower_bound))) + scratch->offset)] = coeff->data[((0 + (1 * (iao - 1))) + ((1 * __2coeff) * (jao - 1)))]*focc->data[(0 + (1 * (iao - 1)))];
        }
    }
    if (!(!scratch->is_allocated || (true && (scratch->dims[0].stride == 1) && (scratch->dims[1].stride == (1 * scratch->dims[0].length))))) {
        // FIXME: deallocate(tblite_wavefunction_type__get_density_matrix_real____0_real_______1_real_______2____libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2, );
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->n_dims = 2;
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1].length = ((int32_t) scratch->dims[2-1].length + scratch->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1].stride = 1;
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[0].length = ((int32_t) scratch->dims[1-1].length + scratch->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[0].stride = (1 * ((int32_t) scratch->dims[2-1].length + scratch->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1].length*__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[0].length*sizeof(double));
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->is_allocated = true;
        for (__lcompilers_i_1=((int32_t)__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[2-1].lower_bound); __lcompilers_i_1<=((int32_t) __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[2-1].length + __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[2-1].lower_bound - 1); __lcompilers_i_1++) {
            for (__lcompilers_i_0=((int32_t)__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1-1].lower_bound); __lcompilers_i_0<=((int32_t) __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1-1].length + __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1-1].lower_bound - 1); __lcompilers_i_0++) {
                __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->data[(((0 + (__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[0].stride * (__lcompilers_i_0 - __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[0].lower_bound))) + (__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1].stride * (__lcompilers_i_1 - __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->dims[1].lower_bound))) + __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2->offset)] = scratch->data[(((0 + (scratch->dims[0].stride * (__lcompilers_i_0 - scratch->dims[0].lower_bound))) + (scratch->dims[1].stride * (__lcompilers_i_1 - scratch->dims[1].lower_bound))) + scratch->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2 = ((struct r64_____*)(scratch));
    }
    __lfortran_tblite_blas_level3__wrap_dgemm_real_______0_real_______1_real_______2(((struct r64_____*)(((struct Allocatable_r64______*)(__libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2)))), ((int32_t)scratch->dims[1-1].length), ((int32_t)scratch->dims[2-1].length), coeff, __1coeff, __2coeff, pmat, __1pmat, __2pmat, __libasr_created_variable_, false, (char*)"t", true, __libasr_created_variable_1, false, __libasr_created_variable_2, false);
    if ((!scratch->is_allocated || (true && (scratch->dims[0].stride == 1) && (scratch->dims[1].stride == (1 * scratch->dims[0].length))))) {
        __libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2 = NULL;
    } else {
        // FIXME: deallocate(tblite_wavefunction_type__get_density_matrix_real____0_real_______1_real_______2____libasr_created__subroutine_call_wrap_dgemm_real_______0_real_______1_real_______2, );
    }
}

