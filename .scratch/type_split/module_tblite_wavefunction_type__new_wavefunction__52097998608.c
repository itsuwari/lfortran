#include "type_generated.h"

void __lfortran_tblite_wavefunction_type__new_wavefunction__StructType(struct tblite_wavefunction_type__wavefunction_type* self, int32_t nat, int32_t nsh, int32_t nao, int32_t nspin, double kt, bool grad, bool __libasr_is_present_grad)
{
    struct r64_________ __libasr_created__array_section__value;
    struct r64_________* __libasr_created__array_section_ = &__libasr_created__array_section__value;
    struct r64_________ __libasr_created__array_section_1_value;
    struct r64_________* __libasr_created__array_section_1 = &__libasr_created__array_section_1_value;
    struct r64_________ __libasr_created__array_section_2_value;
    struct r64_________* __libasr_created__array_section_2 = &__libasr_created__array_section_2_value;
    struct r64_________ __libasr_created__array_section_3_value;
    struct r64_________* __libasr_created__array_section_3 = &__libasr_created__array_section_3_value;
    struct r64_______ __libasr_created__array_section_4_value;
    struct r64_______* __libasr_created__array_section_4 = &__libasr_created__array_section_4_value;
    struct r64_______ __libasr_created__array_section_5_value;
    struct r64_______* __libasr_created__array_section_5 = &__libasr_created__array_section_5_value;
    struct r64_____ __libasr_created__array_section_6_value;
    struct r64_____* __libasr_created__array_section_6 = &__libasr_created__array_section_6_value;
    struct r64_____ __libasr_created__array_section_7_value;
    struct r64_____* __libasr_created__array_section_7 = &__libasr_created__array_section_7_value;
    struct r64_______ __libasr_created__array_section_8_value;
    struct r64_______* __libasr_created__array_section_8 = &__libasr_created__array_section_8_value;
    struct r64_______ __libasr_created__array_section_9_value;
    struct r64_______* __libasr_created__array_section_9 = &__libasr_created__array_section_9_value;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    int32_t __libasr_index_0_2;
    int32_t __libasr_index_0_3;
    int32_t __libasr_index_0_4;
    int32_t __libasr_index_0_5;
    int32_t __libasr_index_0_6;
    int32_t __libasr_index_0_7;
    int32_t __libasr_index_0_8;
    int32_t __libasr_index_0_9;
    int32_t __libasr_index_1_;
    int32_t __libasr_index_1_1;
    int32_t __libasr_index_1_2;
    int32_t __libasr_index_1_3;
    int32_t __libasr_index_1_4;
    int32_t __libasr_index_1_5;
    int32_t __libasr_index_1_6;
    int32_t __libasr_index_1_7;
    int32_t __libasr_index_1_8;
    int32_t __libasr_index_1_9;
    int32_t __libasr_index_2_;
    int32_t __libasr_index_2_1;
    int32_t __libasr_index_2_2;
    int32_t __libasr_index_2_3;
    int32_t __libasr_index_2_4;
    int32_t __libasr_index_2_5;
    int32_t __libasr_index_2_6;
    int32_t __libasr_index_2_7;
    int32_t __libasr_index_3_;
    int32_t __libasr_index_3_1;
    int32_t __libasr_index_3_2;
    int32_t __libasr_index_3_3;
    if ((((*self).coeff) != NULL && ((*self).coeff)->data != NULL)) {
        // FIXME: deallocate((*self).coeff, );
    }
    if ((((*self).density) != NULL && ((*self).density)->data != NULL)) {
        // FIXME: deallocate((*self).density, );
    }
    if ((((*self).dpat) != NULL && ((*self).dpat)->data != NULL)) {
        // FIXME: deallocate((*self).dpat, );
    }
    if ((((*self).dqatdl) != NULL && ((*self).dqatdl)->data != NULL)) {
        // FIXME: deallocate((*self).dqatdl, );
    }
    if ((((*self).dqatdr) != NULL && ((*self).dqatdr)->data != NULL)) {
        // FIXME: deallocate((*self).dqatdr, );
    }
    if ((((*self).dqshdl) != NULL && ((*self).dqshdl)->data != NULL)) {
        // FIXME: deallocate((*self).dqshdl, );
    }
    if ((((*self).dqshdr) != NULL && ((*self).dqshdr)->data != NULL)) {
        // FIXME: deallocate((*self).dqshdr, );
    }
    if ((((*self).emo) != NULL && ((*self).emo)->data != NULL)) {
        // FIXME: deallocate((*self).emo, );
    }
    if ((((*self).focc) != NULL && ((*self).focc)->data != NULL)) {
        // FIXME: deallocate((*self).focc, );
    }
    if ((((*self).n0at) != NULL && ((*self).n0at)->data != NULL)) {
        // FIXME: deallocate((*self).n0at, );
    }
    if ((((*self).n0sh) != NULL && ((*self).n0sh)->data != NULL)) {
        // FIXME: deallocate((*self).n0sh, );
    }
    if ((((*self).nel) != NULL && ((*self).nel)->data != NULL)) {
        // FIXME: deallocate((*self).nel, );
    }
    if ((((*self).qat) != NULL && ((*self).qat)->data != NULL)) {
        // FIXME: deallocate((*self).qat, );
    }
    if ((((*self).qpat) != NULL && ((*self).qpat)->data != NULL)) {
        // FIXME: deallocate((*self).qpat, );
    }
    if ((((*self).qsh) != NULL && ((*self).qsh)->data != NULL)) {
        // FIXME: deallocate((*self).qsh, );
    }
    (*self).nspin = nspin;
    (*self).kt = kt;
    (*self).nel->n_dims = 1;
    (*self).nel->dims[0].lower_bound = 1;
    (*self).nel->dims[0].length = ((2) > (nspin) ? (2) : (nspin));
    (*self).nel->dims[0].stride = 1;
    (*self).nel->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).nel->dims[0].length*sizeof(double));
    (*self).nel->is_allocated = true;
    (*self).n0at->n_dims = 1;
    (*self).n0at->dims[0].lower_bound = 1;
    (*self).n0at->dims[0].length = nat;
    (*self).n0at->dims[0].stride = 1;
    (*self).n0at->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).n0at->dims[0].length*sizeof(double));
    (*self).n0at->is_allocated = true;
    (*self).n0sh->n_dims = 1;
    (*self).n0sh->dims[0].lower_bound = 1;
    (*self).n0sh->dims[0].length = nsh;
    (*self).n0sh->dims[0].stride = 1;
    (*self).n0sh->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).n0sh->dims[0].length*sizeof(double));
    (*self).n0sh->is_allocated = true;
    (*self).density->n_dims = 3;
    (*self).density->dims[2].lower_bound = 1;
    (*self).density->dims[2].length = nspin;
    (*self).density->dims[2].stride = 1;
    (*self).density->dims[1].lower_bound = 1;
    (*self).density->dims[1].length = nao;
    (*self).density->dims[1].stride = (1 * nspin);
    (*self).density->dims[0].lower_bound = 1;
    (*self).density->dims[0].length = nao;
    (*self).density->dims[0].stride = ((1 * nspin) * nao);
    (*self).density->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).density->dims[2].length*(*self).density->dims[1].length*(*self).density->dims[0].length*sizeof(double));
    (*self).density->is_allocated = true;
    (*self).coeff->n_dims = 3;
    (*self).coeff->dims[2].lower_bound = 1;
    (*self).coeff->dims[2].length = nspin;
    (*self).coeff->dims[2].stride = 1;
    (*self).coeff->dims[1].lower_bound = 1;
    (*self).coeff->dims[1].length = nao;
    (*self).coeff->dims[1].stride = (1 * nspin);
    (*self).coeff->dims[0].lower_bound = 1;
    (*self).coeff->dims[0].length = nao;
    (*self).coeff->dims[0].stride = ((1 * nspin) * nao);
    (*self).coeff->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).coeff->dims[2].length*(*self).coeff->dims[1].length*(*self).coeff->dims[0].length*sizeof(double));
    (*self).coeff->is_allocated = true;
    (*self).emo->n_dims = 2;
    (*self).emo->dims[1].lower_bound = 1;
    (*self).emo->dims[1].length = nspin;
    (*self).emo->dims[1].stride = 1;
    (*self).emo->dims[0].lower_bound = 1;
    (*self).emo->dims[0].length = nao;
    (*self).emo->dims[0].stride = (1 * nspin);
    (*self).emo->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).emo->dims[1].length*(*self).emo->dims[0].length*sizeof(double));
    (*self).emo->is_allocated = true;
    (*self).focc->n_dims = 2;
    (*self).focc->dims[1].lower_bound = 1;
    (*self).focc->dims[1].length = ((2) > (nspin) ? (2) : (nspin));
    (*self).focc->dims[1].stride = 1;
    (*self).focc->dims[0].lower_bound = 1;
    (*self).focc->dims[0].length = nao;
    (*self).focc->dims[0].stride = (1 * ((2) > (nspin) ? (2) : (nspin)));
    (*self).focc->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).focc->dims[1].length*(*self).focc->dims[0].length*sizeof(double));
    (*self).focc->is_allocated = true;
    (*self).qat->n_dims = 2;
    (*self).qat->dims[1].lower_bound = 1;
    (*self).qat->dims[1].length = nspin;
    (*self).qat->dims[1].stride = 1;
    (*self).qat->dims[0].lower_bound = 1;
    (*self).qat->dims[0].length = nat;
    (*self).qat->dims[0].stride = (1 * nspin);
    (*self).qat->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).qat->dims[1].length*(*self).qat->dims[0].length*sizeof(double));
    (*self).qat->is_allocated = true;
    (*self).qsh->n_dims = 2;
    (*self).qsh->dims[1].lower_bound = 1;
    (*self).qsh->dims[1].length = nspin;
    (*self).qsh->dims[1].stride = 1;
    (*self).qsh->dims[0].lower_bound = 1;
    (*self).qsh->dims[0].length = nsh;
    (*self).qsh->dims[0].stride = (1 * nspin);
    (*self).qsh->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).qsh->dims[1].length*(*self).qsh->dims[0].length*sizeof(double));
    (*self).qsh->is_allocated = true;
    (*self).dpat->n_dims = 3;
    (*self).dpat->dims[2].lower_bound = 1;
    (*self).dpat->dims[2].length = nspin;
    (*self).dpat->dims[2].stride = 1;
    (*self).dpat->dims[1].lower_bound = 1;
    (*self).dpat->dims[1].length = nat;
    (*self).dpat->dims[1].stride = (1 * nspin);
    (*self).dpat->dims[0].lower_bound = 1;
    (*self).dpat->dims[0].length = 3;
    (*self).dpat->dims[0].stride = ((1 * nspin) * nat);
    (*self).dpat->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).dpat->dims[2].length*(*self).dpat->dims[1].length*(*self).dpat->dims[0].length*sizeof(double));
    (*self).dpat->is_allocated = true;
    (*self).qpat->n_dims = 3;
    (*self).qpat->dims[2].lower_bound = 1;
    (*self).qpat->dims[2].length = nspin;
    (*self).qpat->dims[2].stride = 1;
    (*self).qpat->dims[1].lower_bound = 1;
    (*self).qpat->dims[1].length = nat;
    (*self).qpat->dims[1].stride = (1 * nspin);
    (*self).qpat->dims[0].lower_bound = 1;
    (*self).qpat->dims[0].length = 6;
    (*self).qpat->dims[0].stride = ((1 * nspin) * nat);
    (*self).qpat->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).qpat->dims[2].length*(*self).qpat->dims[1].length*(*self).qpat->dims[0].length*sizeof(double));
    (*self).qpat->is_allocated = true;
    if (__libasr_is_present_grad) {
        if (grad) {
            (*self).dqatdr->n_dims = 4;
            (*self).dqatdr->dims[3].lower_bound = 1;
            (*self).dqatdr->dims[3].length = nspin;
            (*self).dqatdr->dims[3].stride = 1;
            (*self).dqatdr->dims[2].lower_bound = 1;
            (*self).dqatdr->dims[2].length = nat;
            (*self).dqatdr->dims[2].stride = (1 * nspin);
            (*self).dqatdr->dims[1].lower_bound = 1;
            (*self).dqatdr->dims[1].length = nat;
            (*self).dqatdr->dims[1].stride = ((1 * nspin) * nat);
            (*self).dqatdr->dims[0].lower_bound = 1;
            (*self).dqatdr->dims[0].length = 3;
            (*self).dqatdr->dims[0].stride = (((1 * nspin) * nat) * nat);
            (*self).dqatdr->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).dqatdr->dims[3].length*(*self).dqatdr->dims[2].length*(*self).dqatdr->dims[1].length*(*self).dqatdr->dims[0].length*sizeof(double));
            (*self).dqatdr->is_allocated = true;
            (*self).dqatdl->n_dims = 4;
            (*self).dqatdl->dims[3].lower_bound = 1;
            (*self).dqatdl->dims[3].length = nspin;
            (*self).dqatdl->dims[3].stride = 1;
            (*self).dqatdl->dims[2].lower_bound = 1;
            (*self).dqatdl->dims[2].length = nat;
            (*self).dqatdl->dims[2].stride = (1 * nspin);
            (*self).dqatdl->dims[1].lower_bound = 1;
            (*self).dqatdl->dims[1].length = 3;
            (*self).dqatdl->dims[1].stride = ((1 * nspin) * nat);
            (*self).dqatdl->dims[0].lower_bound = 1;
            (*self).dqatdl->dims[0].length = 3;
            (*self).dqatdl->dims[0].stride = (((1 * nspin) * nat) * 3);
            (*self).dqatdl->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).dqatdl->dims[3].length*(*self).dqatdl->dims[2].length*(*self).dqatdl->dims[1].length*(*self).dqatdl->dims[0].length*sizeof(double));
            (*self).dqatdl->is_allocated = true;
            (*self).dqshdr->n_dims = 4;
            (*self).dqshdr->dims[3].lower_bound = 1;
            (*self).dqshdr->dims[3].length = nspin;
            (*self).dqshdr->dims[3].stride = 1;
            (*self).dqshdr->dims[2].lower_bound = 1;
            (*self).dqshdr->dims[2].length = nsh;
            (*self).dqshdr->dims[2].stride = (1 * nspin);
            (*self).dqshdr->dims[1].lower_bound = 1;
            (*self).dqshdr->dims[1].length = nat;
            (*self).dqshdr->dims[1].stride = ((1 * nspin) * nsh);
            (*self).dqshdr->dims[0].lower_bound = 1;
            (*self).dqshdr->dims[0].length = 3;
            (*self).dqshdr->dims[0].stride = (((1 * nspin) * nsh) * nat);
            (*self).dqshdr->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).dqshdr->dims[3].length*(*self).dqshdr->dims[2].length*(*self).dqshdr->dims[1].length*(*self).dqshdr->dims[0].length*sizeof(double));
            (*self).dqshdr->is_allocated = true;
            (*self).dqshdl->n_dims = 4;
            (*self).dqshdl->dims[3].lower_bound = 1;
            (*self).dqshdl->dims[3].length = nspin;
            (*self).dqshdl->dims[3].stride = 1;
            (*self).dqshdl->dims[2].lower_bound = 1;
            (*self).dqshdl->dims[2].length = nsh;
            (*self).dqshdl->dims[2].stride = (1 * nspin);
            (*self).dqshdl->dims[1].lower_bound = 1;
            (*self).dqshdl->dims[1].length = 3;
            (*self).dqshdl->dims[1].stride = ((1 * nspin) * nsh);
            (*self).dqshdl->dims[0].lower_bound = 1;
            (*self).dqshdl->dims[0].length = 3;
            (*self).dqshdl->dims[0].stride = (((1 * nspin) * nsh) * 3);
            (*self).dqshdl->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*(*self).dqshdl->dims[3].length*(*self).dqshdl->dims[2].length*(*self).dqshdl->dims[1].length*(*self).dqshdl->dims[0].length*sizeof(double));
            (*self).dqshdl->is_allocated = true;
            __libasr_created__array_section_->data = ((*self).dqatdr->data + ((((0 + (1 * (((int32_t)(*self).dqatdr->dims[4-1].lower_bound) - (*self).dqatdr->dims[3].lower_bound))) + ((1 * (*self).dqatdr->dims[3].length) * (((int32_t)(*self).dqatdr->dims[3-1].lower_bound) - (*self).dqatdr->dims[2].lower_bound))) + (((1 * (*self).dqatdr->dims[3].length) * (*self).dqatdr->dims[2].length) * (((int32_t)(*self).dqatdr->dims[2-1].lower_bound) - (*self).dqatdr->dims[1].lower_bound))) + ((((1 * (*self).dqatdr->dims[3].length) * (*self).dqatdr->dims[2].length) * (*self).dqatdr->dims[1].length) * (((int32_t)(*self).dqatdr->dims[1-1].lower_bound) - (*self).dqatdr->dims[0].lower_bound))));
            __libasr_created__array_section_->offset = 0;
            __libasr_created__array_section_->dims[3].stride = 1;
            __libasr_created__array_section_->dims[3].lower_bound = 1;
            __libasr_created__array_section_->dims[3].length = ((( (((int32_t) (*self).dqatdr->dims[4-1].length + (*self).dqatdr->dims[4-1].lower_bound - 1)) - (((int32_t)(*self).dqatdr->dims[4-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_->dims[2].stride = (1*(*self).dqatdr->dims[3].length);
            __libasr_created__array_section_->dims[2].lower_bound = 1;
            __libasr_created__array_section_->dims[2].length = ((( (((int32_t) (*self).dqatdr->dims[3-1].length + (*self).dqatdr->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).dqatdr->dims[3-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_->dims[1].stride = ((1*(*self).dqatdr->dims[3].length)*(*self).dqatdr->dims[2].length);
            __libasr_created__array_section_->dims[1].lower_bound = 1;
            __libasr_created__array_section_->dims[1].length = ((( (((int32_t) (*self).dqatdr->dims[2-1].length + (*self).dqatdr->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).dqatdr->dims[2-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_->dims[0].stride = (((1*(*self).dqatdr->dims[3].length)*(*self).dqatdr->dims[2].length)*(*self).dqatdr->dims[1].length);
            __libasr_created__array_section_->dims[0].lower_bound = 1;
            __libasr_created__array_section_->dims[0].length = ((( (((int32_t) (*self).dqatdr->dims[1-1].length + (*self).dqatdr->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).dqatdr->dims[1-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_->n_dims = 4;
            for (__libasr_index_3_=((int32_t)__libasr_created__array_section_->dims[4-1].lower_bound); __libasr_index_3_<=((int32_t) __libasr_created__array_section_->dims[4-1].length + __libasr_created__array_section_->dims[4-1].lower_bound - 1); __libasr_index_3_++) {
                for (__libasr_index_2_=((int32_t)__libasr_created__array_section_->dims[3-1].lower_bound); __libasr_index_2_<=((int32_t) __libasr_created__array_section_->dims[3-1].length + __libasr_created__array_section_->dims[3-1].lower_bound - 1); __libasr_index_2_++) {
                    for (__libasr_index_1_=((int32_t)__libasr_created__array_section_->dims[2-1].lower_bound); __libasr_index_1_<=((int32_t) __libasr_created__array_section_->dims[2-1].length + __libasr_created__array_section_->dims[2-1].lower_bound - 1); __libasr_index_1_++) {
                        for (__libasr_index_0_=((int32_t)__libasr_created__array_section_->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__array_section_->dims[1-1].length + __libasr_created__array_section_->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
                            __libasr_created__array_section_->data[(((((0 + (__libasr_created__array_section_->dims[0].stride * (__libasr_index_0_ - __libasr_created__array_section_->dims[0].lower_bound))) + (__libasr_created__array_section_->dims[1].stride * (__libasr_index_1_ - __libasr_created__array_section_->dims[1].lower_bound))) + (__libasr_created__array_section_->dims[2].stride * (__libasr_index_2_ - __libasr_created__array_section_->dims[2].lower_bound))) + (__libasr_created__array_section_->dims[3].stride * (__libasr_index_3_ - __libasr_created__array_section_->dims[3].lower_bound))) + __libasr_created__array_section_->offset)] =   0.00000000000000000e+00;
                        }
                    }
                }
            }
            __libasr_created__array_section_1->data = ((*self).dqatdl->data + ((((0 + (1 * (((int32_t)(*self).dqatdl->dims[4-1].lower_bound) - (*self).dqatdl->dims[3].lower_bound))) + ((1 * (*self).dqatdl->dims[3].length) * (((int32_t)(*self).dqatdl->dims[3-1].lower_bound) - (*self).dqatdl->dims[2].lower_bound))) + (((1 * (*self).dqatdl->dims[3].length) * (*self).dqatdl->dims[2].length) * (((int32_t)(*self).dqatdl->dims[2-1].lower_bound) - (*self).dqatdl->dims[1].lower_bound))) + ((((1 * (*self).dqatdl->dims[3].length) * (*self).dqatdl->dims[2].length) * (*self).dqatdl->dims[1].length) * (((int32_t)(*self).dqatdl->dims[1-1].lower_bound) - (*self).dqatdl->dims[0].lower_bound))));
            __libasr_created__array_section_1->offset = 0;
            __libasr_created__array_section_1->dims[3].stride = 1;
            __libasr_created__array_section_1->dims[3].lower_bound = 1;
            __libasr_created__array_section_1->dims[3].length = ((( (((int32_t) (*self).dqatdl->dims[4-1].length + (*self).dqatdl->dims[4-1].lower_bound - 1)) - (((int32_t)(*self).dqatdl->dims[4-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_1->dims[2].stride = (1*(*self).dqatdl->dims[3].length);
            __libasr_created__array_section_1->dims[2].lower_bound = 1;
            __libasr_created__array_section_1->dims[2].length = ((( (((int32_t) (*self).dqatdl->dims[3-1].length + (*self).dqatdl->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).dqatdl->dims[3-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_1->dims[1].stride = ((1*(*self).dqatdl->dims[3].length)*(*self).dqatdl->dims[2].length);
            __libasr_created__array_section_1->dims[1].lower_bound = 1;
            __libasr_created__array_section_1->dims[1].length = ((( (((int32_t) (*self).dqatdl->dims[2-1].length + (*self).dqatdl->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).dqatdl->dims[2-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_1->dims[0].stride = (((1*(*self).dqatdl->dims[3].length)*(*self).dqatdl->dims[2].length)*(*self).dqatdl->dims[1].length);
            __libasr_created__array_section_1->dims[0].lower_bound = 1;
            __libasr_created__array_section_1->dims[0].length = ((( (((int32_t) (*self).dqatdl->dims[1-1].length + (*self).dqatdl->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).dqatdl->dims[1-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_1->n_dims = 4;
            for (__libasr_index_3_1=((int32_t)__libasr_created__array_section_1->dims[4-1].lower_bound); __libasr_index_3_1<=((int32_t) __libasr_created__array_section_1->dims[4-1].length + __libasr_created__array_section_1->dims[4-1].lower_bound - 1); __libasr_index_3_1++) {
                for (__libasr_index_2_1=((int32_t)__libasr_created__array_section_1->dims[3-1].lower_bound); __libasr_index_2_1<=((int32_t) __libasr_created__array_section_1->dims[3-1].length + __libasr_created__array_section_1->dims[3-1].lower_bound - 1); __libasr_index_2_1++) {
                    for (__libasr_index_1_1=((int32_t)__libasr_created__array_section_1->dims[2-1].lower_bound); __libasr_index_1_1<=((int32_t) __libasr_created__array_section_1->dims[2-1].length + __libasr_created__array_section_1->dims[2-1].lower_bound - 1); __libasr_index_1_1++) {
                        for (__libasr_index_0_1=((int32_t)__libasr_created__array_section_1->dims[1-1].lower_bound); __libasr_index_0_1<=((int32_t) __libasr_created__array_section_1->dims[1-1].length + __libasr_created__array_section_1->dims[1-1].lower_bound - 1); __libasr_index_0_1++) {
                            __libasr_created__array_section_1->data[(((((0 + (__libasr_created__array_section_1->dims[0].stride * (__libasr_index_0_1 - __libasr_created__array_section_1->dims[0].lower_bound))) + (__libasr_created__array_section_1->dims[1].stride * (__libasr_index_1_1 - __libasr_created__array_section_1->dims[1].lower_bound))) + (__libasr_created__array_section_1->dims[2].stride * (__libasr_index_2_1 - __libasr_created__array_section_1->dims[2].lower_bound))) + (__libasr_created__array_section_1->dims[3].stride * (__libasr_index_3_1 - __libasr_created__array_section_1->dims[3].lower_bound))) + __libasr_created__array_section_1->offset)] =   0.00000000000000000e+00;
                        }
                    }
                }
            }
            __libasr_created__array_section_2->data = ((*self).dqshdr->data + ((((0 + (1 * (((int32_t)(*self).dqshdr->dims[4-1].lower_bound) - (*self).dqshdr->dims[3].lower_bound))) + ((1 * (*self).dqshdr->dims[3].length) * (((int32_t)(*self).dqshdr->dims[3-1].lower_bound) - (*self).dqshdr->dims[2].lower_bound))) + (((1 * (*self).dqshdr->dims[3].length) * (*self).dqshdr->dims[2].length) * (((int32_t)(*self).dqshdr->dims[2-1].lower_bound) - (*self).dqshdr->dims[1].lower_bound))) + ((((1 * (*self).dqshdr->dims[3].length) * (*self).dqshdr->dims[2].length) * (*self).dqshdr->dims[1].length) * (((int32_t)(*self).dqshdr->dims[1-1].lower_bound) - (*self).dqshdr->dims[0].lower_bound))));
            __libasr_created__array_section_2->offset = 0;
            __libasr_created__array_section_2->dims[3].stride = 1;
            __libasr_created__array_section_2->dims[3].lower_bound = 1;
            __libasr_created__array_section_2->dims[3].length = ((( (((int32_t) (*self).dqshdr->dims[4-1].length + (*self).dqshdr->dims[4-1].lower_bound - 1)) - (((int32_t)(*self).dqshdr->dims[4-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_2->dims[2].stride = (1*(*self).dqshdr->dims[3].length);
            __libasr_created__array_section_2->dims[2].lower_bound = 1;
            __libasr_created__array_section_2->dims[2].length = ((( (((int32_t) (*self).dqshdr->dims[3-1].length + (*self).dqshdr->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).dqshdr->dims[3-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_2->dims[1].stride = ((1*(*self).dqshdr->dims[3].length)*(*self).dqshdr->dims[2].length);
            __libasr_created__array_section_2->dims[1].lower_bound = 1;
            __libasr_created__array_section_2->dims[1].length = ((( (((int32_t) (*self).dqshdr->dims[2-1].length + (*self).dqshdr->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).dqshdr->dims[2-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_2->dims[0].stride = (((1*(*self).dqshdr->dims[3].length)*(*self).dqshdr->dims[2].length)*(*self).dqshdr->dims[1].length);
            __libasr_created__array_section_2->dims[0].lower_bound = 1;
            __libasr_created__array_section_2->dims[0].length = ((( (((int32_t) (*self).dqshdr->dims[1-1].length + (*self).dqshdr->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).dqshdr->dims[1-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_2->n_dims = 4;
            for (__libasr_index_3_2=((int32_t)__libasr_created__array_section_2->dims[4-1].lower_bound); __libasr_index_3_2<=((int32_t) __libasr_created__array_section_2->dims[4-1].length + __libasr_created__array_section_2->dims[4-1].lower_bound - 1); __libasr_index_3_2++) {
                for (__libasr_index_2_2=((int32_t)__libasr_created__array_section_2->dims[3-1].lower_bound); __libasr_index_2_2<=((int32_t) __libasr_created__array_section_2->dims[3-1].length + __libasr_created__array_section_2->dims[3-1].lower_bound - 1); __libasr_index_2_2++) {
                    for (__libasr_index_1_2=((int32_t)__libasr_created__array_section_2->dims[2-1].lower_bound); __libasr_index_1_2<=((int32_t) __libasr_created__array_section_2->dims[2-1].length + __libasr_created__array_section_2->dims[2-1].lower_bound - 1); __libasr_index_1_2++) {
                        for (__libasr_index_0_2=((int32_t)__libasr_created__array_section_2->dims[1-1].lower_bound); __libasr_index_0_2<=((int32_t) __libasr_created__array_section_2->dims[1-1].length + __libasr_created__array_section_2->dims[1-1].lower_bound - 1); __libasr_index_0_2++) {
                            __libasr_created__array_section_2->data[(((((0 + (__libasr_created__array_section_2->dims[0].stride * (__libasr_index_0_2 - __libasr_created__array_section_2->dims[0].lower_bound))) + (__libasr_created__array_section_2->dims[1].stride * (__libasr_index_1_2 - __libasr_created__array_section_2->dims[1].lower_bound))) + (__libasr_created__array_section_2->dims[2].stride * (__libasr_index_2_2 - __libasr_created__array_section_2->dims[2].lower_bound))) + (__libasr_created__array_section_2->dims[3].stride * (__libasr_index_3_2 - __libasr_created__array_section_2->dims[3].lower_bound))) + __libasr_created__array_section_2->offset)] =   0.00000000000000000e+00;
                        }
                    }
                }
            }
            __libasr_created__array_section_3->data = ((*self).dqshdl->data + ((((0 + (1 * (((int32_t)(*self).dqshdl->dims[4-1].lower_bound) - (*self).dqshdl->dims[3].lower_bound))) + ((1 * (*self).dqshdl->dims[3].length) * (((int32_t)(*self).dqshdl->dims[3-1].lower_bound) - (*self).dqshdl->dims[2].lower_bound))) + (((1 * (*self).dqshdl->dims[3].length) * (*self).dqshdl->dims[2].length) * (((int32_t)(*self).dqshdl->dims[2-1].lower_bound) - (*self).dqshdl->dims[1].lower_bound))) + ((((1 * (*self).dqshdl->dims[3].length) * (*self).dqshdl->dims[2].length) * (*self).dqshdl->dims[1].length) * (((int32_t)(*self).dqshdl->dims[1-1].lower_bound) - (*self).dqshdl->dims[0].lower_bound))));
            __libasr_created__array_section_3->offset = 0;
            __libasr_created__array_section_3->dims[3].stride = 1;
            __libasr_created__array_section_3->dims[3].lower_bound = 1;
            __libasr_created__array_section_3->dims[3].length = ((( (((int32_t) (*self).dqshdl->dims[4-1].length + (*self).dqshdl->dims[4-1].lower_bound - 1)) - (((int32_t)(*self).dqshdl->dims[4-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_3->dims[2].stride = (1*(*self).dqshdl->dims[3].length);
            __libasr_created__array_section_3->dims[2].lower_bound = 1;
            __libasr_created__array_section_3->dims[2].length = ((( (((int32_t) (*self).dqshdl->dims[3-1].length + (*self).dqshdl->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).dqshdl->dims[3-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_3->dims[1].stride = ((1*(*self).dqshdl->dims[3].length)*(*self).dqshdl->dims[2].length);
            __libasr_created__array_section_3->dims[1].lower_bound = 1;
            __libasr_created__array_section_3->dims[1].length = ((( (((int32_t) (*self).dqshdl->dims[2-1].length + (*self).dqshdl->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).dqshdl->dims[2-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_3->dims[0].stride = (((1*(*self).dqshdl->dims[3].length)*(*self).dqshdl->dims[2].length)*(*self).dqshdl->dims[1].length);
            __libasr_created__array_section_3->dims[0].lower_bound = 1;
            __libasr_created__array_section_3->dims[0].length = ((( (((int32_t) (*self).dqshdl->dims[1-1].length + (*self).dqshdl->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).dqshdl->dims[1-1].lower_bound)) )/1) + 1);
            __libasr_created__array_section_3->n_dims = 4;
            for (__libasr_index_3_3=((int32_t)__libasr_created__array_section_3->dims[4-1].lower_bound); __libasr_index_3_3<=((int32_t) __libasr_created__array_section_3->dims[4-1].length + __libasr_created__array_section_3->dims[4-1].lower_bound - 1); __libasr_index_3_3++) {
                for (__libasr_index_2_3=((int32_t)__libasr_created__array_section_3->dims[3-1].lower_bound); __libasr_index_2_3<=((int32_t) __libasr_created__array_section_3->dims[3-1].length + __libasr_created__array_section_3->dims[3-1].lower_bound - 1); __libasr_index_2_3++) {
                    for (__libasr_index_1_3=((int32_t)__libasr_created__array_section_3->dims[2-1].lower_bound); __libasr_index_1_3<=((int32_t) __libasr_created__array_section_3->dims[2-1].length + __libasr_created__array_section_3->dims[2-1].lower_bound - 1); __libasr_index_1_3++) {
                        for (__libasr_index_0_3=((int32_t)__libasr_created__array_section_3->dims[1-1].lower_bound); __libasr_index_0_3<=((int32_t) __libasr_created__array_section_3->dims[1-1].length + __libasr_created__array_section_3->dims[1-1].lower_bound - 1); __libasr_index_0_3++) {
                            __libasr_created__array_section_3->data[(((((0 + (__libasr_created__array_section_3->dims[0].stride * (__libasr_index_0_3 - __libasr_created__array_section_3->dims[0].lower_bound))) + (__libasr_created__array_section_3->dims[1].stride * (__libasr_index_1_3 - __libasr_created__array_section_3->dims[1].lower_bound))) + (__libasr_created__array_section_3->dims[2].stride * (__libasr_index_2_3 - __libasr_created__array_section_3->dims[2].lower_bound))) + (__libasr_created__array_section_3->dims[3].stride * (__libasr_index_3_3 - __libasr_created__array_section_3->dims[3].lower_bound))) + __libasr_created__array_section_3->offset)] =   0.00000000000000000e+00;
                        }
                    }
                }
            }
        }
    }
    __libasr_created__array_section_4->data = ((*self).density->data + (((0 + (1 * (((int32_t)(*self).density->dims[3-1].lower_bound) - (*self).density->dims[2].lower_bound))) + ((1 * (*self).density->dims[2].length) * (((int32_t)(*self).density->dims[2-1].lower_bound) - (*self).density->dims[1].lower_bound))) + (((1 * (*self).density->dims[2].length) * (*self).density->dims[1].length) * (((int32_t)(*self).density->dims[1-1].lower_bound) - (*self).density->dims[0].lower_bound))));
    __libasr_created__array_section_4->offset = 0;
    __libasr_created__array_section_4->dims[2].stride = 1;
    __libasr_created__array_section_4->dims[2].lower_bound = 1;
    __libasr_created__array_section_4->dims[2].length = ((( (((int32_t) (*self).density->dims[3-1].length + (*self).density->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).density->dims[3-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_4->dims[1].stride = (1*(*self).density->dims[2].length);
    __libasr_created__array_section_4->dims[1].lower_bound = 1;
    __libasr_created__array_section_4->dims[1].length = ((( (((int32_t) (*self).density->dims[2-1].length + (*self).density->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).density->dims[2-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_4->dims[0].stride = ((1*(*self).density->dims[2].length)*(*self).density->dims[1].length);
    __libasr_created__array_section_4->dims[0].lower_bound = 1;
    __libasr_created__array_section_4->dims[0].length = ((( (((int32_t) (*self).density->dims[1-1].length + (*self).density->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).density->dims[1-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_4->n_dims = 3;
    for (__libasr_index_2_4=((int32_t)__libasr_created__array_section_4->dims[3-1].lower_bound); __libasr_index_2_4<=((int32_t) __libasr_created__array_section_4->dims[3-1].length + __libasr_created__array_section_4->dims[3-1].lower_bound - 1); __libasr_index_2_4++) {
        for (__libasr_index_1_4=((int32_t)__libasr_created__array_section_4->dims[2-1].lower_bound); __libasr_index_1_4<=((int32_t) __libasr_created__array_section_4->dims[2-1].length + __libasr_created__array_section_4->dims[2-1].lower_bound - 1); __libasr_index_1_4++) {
            for (__libasr_index_0_4=((int32_t)__libasr_created__array_section_4->dims[1-1].lower_bound); __libasr_index_0_4<=((int32_t) __libasr_created__array_section_4->dims[1-1].length + __libasr_created__array_section_4->dims[1-1].lower_bound - 1); __libasr_index_0_4++) {
                __libasr_created__array_section_4->data[((((0 + (__libasr_created__array_section_4->dims[0].stride * (__libasr_index_0_4 - __libasr_created__array_section_4->dims[0].lower_bound))) + (__libasr_created__array_section_4->dims[1].stride * (__libasr_index_1_4 - __libasr_created__array_section_4->dims[1].lower_bound))) + (__libasr_created__array_section_4->dims[2].stride * (__libasr_index_2_4 - __libasr_created__array_section_4->dims[2].lower_bound))) + __libasr_created__array_section_4->offset)] =   0.00000000000000000e+00;
            }
        }
    }
    __libasr_created__array_section_5->data = ((*self).coeff->data + (((0 + (1 * (((int32_t)(*self).coeff->dims[3-1].lower_bound) - (*self).coeff->dims[2].lower_bound))) + ((1 * (*self).coeff->dims[2].length) * (((int32_t)(*self).coeff->dims[2-1].lower_bound) - (*self).coeff->dims[1].lower_bound))) + (((1 * (*self).coeff->dims[2].length) * (*self).coeff->dims[1].length) * (((int32_t)(*self).coeff->dims[1-1].lower_bound) - (*self).coeff->dims[0].lower_bound))));
    __libasr_created__array_section_5->offset = 0;
    __libasr_created__array_section_5->dims[2].stride = 1;
    __libasr_created__array_section_5->dims[2].lower_bound = 1;
    __libasr_created__array_section_5->dims[2].length = ((( (((int32_t) (*self).coeff->dims[3-1].length + (*self).coeff->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).coeff->dims[3-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_5->dims[1].stride = (1*(*self).coeff->dims[2].length);
    __libasr_created__array_section_5->dims[1].lower_bound = 1;
    __libasr_created__array_section_5->dims[1].length = ((( (((int32_t) (*self).coeff->dims[2-1].length + (*self).coeff->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).coeff->dims[2-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_5->dims[0].stride = ((1*(*self).coeff->dims[2].length)*(*self).coeff->dims[1].length);
    __libasr_created__array_section_5->dims[0].lower_bound = 1;
    __libasr_created__array_section_5->dims[0].length = ((( (((int32_t) (*self).coeff->dims[1-1].length + (*self).coeff->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).coeff->dims[1-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_5->n_dims = 3;
    for (__libasr_index_2_5=((int32_t)__libasr_created__array_section_5->dims[3-1].lower_bound); __libasr_index_2_5<=((int32_t) __libasr_created__array_section_5->dims[3-1].length + __libasr_created__array_section_5->dims[3-1].lower_bound - 1); __libasr_index_2_5++) {
        for (__libasr_index_1_5=((int32_t)__libasr_created__array_section_5->dims[2-1].lower_bound); __libasr_index_1_5<=((int32_t) __libasr_created__array_section_5->dims[2-1].length + __libasr_created__array_section_5->dims[2-1].lower_bound - 1); __libasr_index_1_5++) {
            for (__libasr_index_0_5=((int32_t)__libasr_created__array_section_5->dims[1-1].lower_bound); __libasr_index_0_5<=((int32_t) __libasr_created__array_section_5->dims[1-1].length + __libasr_created__array_section_5->dims[1-1].lower_bound - 1); __libasr_index_0_5++) {
                __libasr_created__array_section_5->data[((((0 + (__libasr_created__array_section_5->dims[0].stride * (__libasr_index_0_5 - __libasr_created__array_section_5->dims[0].lower_bound))) + (__libasr_created__array_section_5->dims[1].stride * (__libasr_index_1_5 - __libasr_created__array_section_5->dims[1].lower_bound))) + (__libasr_created__array_section_5->dims[2].stride * (__libasr_index_2_5 - __libasr_created__array_section_5->dims[2].lower_bound))) + __libasr_created__array_section_5->offset)] =   0.00000000000000000e+00;
            }
        }
    }
    __libasr_created__array_section_6->data = ((*self).qat->data + ((0 + (1 * (((int32_t)(*self).qat->dims[2-1].lower_bound) - (*self).qat->dims[1].lower_bound))) + ((1 * (*self).qat->dims[1].length) * (((int32_t)(*self).qat->dims[1-1].lower_bound) - (*self).qat->dims[0].lower_bound))));
    __libasr_created__array_section_6->offset = 0;
    __libasr_created__array_section_6->dims[1].stride = 1;
    __libasr_created__array_section_6->dims[1].lower_bound = 1;
    __libasr_created__array_section_6->dims[1].length = ((( (((int32_t) (*self).qat->dims[2-1].length + (*self).qat->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).qat->dims[2-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_6->dims[0].stride = (1*(*self).qat->dims[1].length);
    __libasr_created__array_section_6->dims[0].lower_bound = 1;
    __libasr_created__array_section_6->dims[0].length = ((( (((int32_t) (*self).qat->dims[1-1].length + (*self).qat->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).qat->dims[1-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_6->n_dims = 2;
    for (__libasr_index_1_6=((int32_t)__libasr_created__array_section_6->dims[2-1].lower_bound); __libasr_index_1_6<=((int32_t) __libasr_created__array_section_6->dims[2-1].length + __libasr_created__array_section_6->dims[2-1].lower_bound - 1); __libasr_index_1_6++) {
        for (__libasr_index_0_6=((int32_t)__libasr_created__array_section_6->dims[1-1].lower_bound); __libasr_index_0_6<=((int32_t) __libasr_created__array_section_6->dims[1-1].length + __libasr_created__array_section_6->dims[1-1].lower_bound - 1); __libasr_index_0_6++) {
            __libasr_created__array_section_6->data[(((0 + (__libasr_created__array_section_6->dims[0].stride * (__libasr_index_0_6 - __libasr_created__array_section_6->dims[0].lower_bound))) + (__libasr_created__array_section_6->dims[1].stride * (__libasr_index_1_6 - __libasr_created__array_section_6->dims[1].lower_bound))) + __libasr_created__array_section_6->offset)] =   0.00000000000000000e+00;
        }
    }
    __libasr_created__array_section_7->data = ((*self).qsh->data + ((0 + (1 * (((int32_t)(*self).qsh->dims[2-1].lower_bound) - (*self).qsh->dims[1].lower_bound))) + ((1 * (*self).qsh->dims[1].length) * (((int32_t)(*self).qsh->dims[1-1].lower_bound) - (*self).qsh->dims[0].lower_bound))));
    __libasr_created__array_section_7->offset = 0;
    __libasr_created__array_section_7->dims[1].stride = 1;
    __libasr_created__array_section_7->dims[1].lower_bound = 1;
    __libasr_created__array_section_7->dims[1].length = ((( (((int32_t) (*self).qsh->dims[2-1].length + (*self).qsh->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).qsh->dims[2-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_7->dims[0].stride = (1*(*self).qsh->dims[1].length);
    __libasr_created__array_section_7->dims[0].lower_bound = 1;
    __libasr_created__array_section_7->dims[0].length = ((( (((int32_t) (*self).qsh->dims[1-1].length + (*self).qsh->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).qsh->dims[1-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_7->n_dims = 2;
    for (__libasr_index_1_7=((int32_t)__libasr_created__array_section_7->dims[2-1].lower_bound); __libasr_index_1_7<=((int32_t) __libasr_created__array_section_7->dims[2-1].length + __libasr_created__array_section_7->dims[2-1].lower_bound - 1); __libasr_index_1_7++) {
        for (__libasr_index_0_7=((int32_t)__libasr_created__array_section_7->dims[1-1].lower_bound); __libasr_index_0_7<=((int32_t) __libasr_created__array_section_7->dims[1-1].length + __libasr_created__array_section_7->dims[1-1].lower_bound - 1); __libasr_index_0_7++) {
            __libasr_created__array_section_7->data[(((0 + (__libasr_created__array_section_7->dims[0].stride * (__libasr_index_0_7 - __libasr_created__array_section_7->dims[0].lower_bound))) + (__libasr_created__array_section_7->dims[1].stride * (__libasr_index_1_7 - __libasr_created__array_section_7->dims[1].lower_bound))) + __libasr_created__array_section_7->offset)] =   0.00000000000000000e+00;
        }
    }
    __libasr_created__array_section_8->data = ((*self).dpat->data + (((0 + (1 * (((int32_t)(*self).dpat->dims[3-1].lower_bound) - (*self).dpat->dims[2].lower_bound))) + ((1 * (*self).dpat->dims[2].length) * (((int32_t)(*self).dpat->dims[2-1].lower_bound) - (*self).dpat->dims[1].lower_bound))) + (((1 * (*self).dpat->dims[2].length) * (*self).dpat->dims[1].length) * (((int32_t)(*self).dpat->dims[1-1].lower_bound) - (*self).dpat->dims[0].lower_bound))));
    __libasr_created__array_section_8->offset = 0;
    __libasr_created__array_section_8->dims[2].stride = 1;
    __libasr_created__array_section_8->dims[2].lower_bound = 1;
    __libasr_created__array_section_8->dims[2].length = ((( (((int32_t) (*self).dpat->dims[3-1].length + (*self).dpat->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).dpat->dims[3-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_8->dims[1].stride = (1*(*self).dpat->dims[2].length);
    __libasr_created__array_section_8->dims[1].lower_bound = 1;
    __libasr_created__array_section_8->dims[1].length = ((( (((int32_t) (*self).dpat->dims[2-1].length + (*self).dpat->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).dpat->dims[2-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_8->dims[0].stride = ((1*(*self).dpat->dims[2].length)*(*self).dpat->dims[1].length);
    __libasr_created__array_section_8->dims[0].lower_bound = 1;
    __libasr_created__array_section_8->dims[0].length = ((( (((int32_t) (*self).dpat->dims[1-1].length + (*self).dpat->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).dpat->dims[1-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_8->n_dims = 3;
    for (__libasr_index_2_6=((int32_t)__libasr_created__array_section_8->dims[3-1].lower_bound); __libasr_index_2_6<=((int32_t) __libasr_created__array_section_8->dims[3-1].length + __libasr_created__array_section_8->dims[3-1].lower_bound - 1); __libasr_index_2_6++) {
        for (__libasr_index_1_8=((int32_t)__libasr_created__array_section_8->dims[2-1].lower_bound); __libasr_index_1_8<=((int32_t) __libasr_created__array_section_8->dims[2-1].length + __libasr_created__array_section_8->dims[2-1].lower_bound - 1); __libasr_index_1_8++) {
            for (__libasr_index_0_8=((int32_t)__libasr_created__array_section_8->dims[1-1].lower_bound); __libasr_index_0_8<=((int32_t) __libasr_created__array_section_8->dims[1-1].length + __libasr_created__array_section_8->dims[1-1].lower_bound - 1); __libasr_index_0_8++) {
                __libasr_created__array_section_8->data[((((0 + (__libasr_created__array_section_8->dims[0].stride * (__libasr_index_0_8 - __libasr_created__array_section_8->dims[0].lower_bound))) + (__libasr_created__array_section_8->dims[1].stride * (__libasr_index_1_8 - __libasr_created__array_section_8->dims[1].lower_bound))) + (__libasr_created__array_section_8->dims[2].stride * (__libasr_index_2_6 - __libasr_created__array_section_8->dims[2].lower_bound))) + __libasr_created__array_section_8->offset)] =   0.00000000000000000e+00;
            }
        }
    }
    __libasr_created__array_section_9->data = ((*self).qpat->data + (((0 + (1 * (((int32_t)(*self).qpat->dims[3-1].lower_bound) - (*self).qpat->dims[2].lower_bound))) + ((1 * (*self).qpat->dims[2].length) * (((int32_t)(*self).qpat->dims[2-1].lower_bound) - (*self).qpat->dims[1].lower_bound))) + (((1 * (*self).qpat->dims[2].length) * (*self).qpat->dims[1].length) * (((int32_t)(*self).qpat->dims[1-1].lower_bound) - (*self).qpat->dims[0].lower_bound))));
    __libasr_created__array_section_9->offset = 0;
    __libasr_created__array_section_9->dims[2].stride = 1;
    __libasr_created__array_section_9->dims[2].lower_bound = 1;
    __libasr_created__array_section_9->dims[2].length = ((( (((int32_t) (*self).qpat->dims[3-1].length + (*self).qpat->dims[3-1].lower_bound - 1)) - (((int32_t)(*self).qpat->dims[3-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_9->dims[1].stride = (1*(*self).qpat->dims[2].length);
    __libasr_created__array_section_9->dims[1].lower_bound = 1;
    __libasr_created__array_section_9->dims[1].length = ((( (((int32_t) (*self).qpat->dims[2-1].length + (*self).qpat->dims[2-1].lower_bound - 1)) - (((int32_t)(*self).qpat->dims[2-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_9->dims[0].stride = ((1*(*self).qpat->dims[2].length)*(*self).qpat->dims[1].length);
    __libasr_created__array_section_9->dims[0].lower_bound = 1;
    __libasr_created__array_section_9->dims[0].length = ((( (((int32_t) (*self).qpat->dims[1-1].length + (*self).qpat->dims[1-1].lower_bound - 1)) - (((int32_t)(*self).qpat->dims[1-1].lower_bound)) )/1) + 1);
    __libasr_created__array_section_9->n_dims = 3;
    for (__libasr_index_2_7=((int32_t)__libasr_created__array_section_9->dims[3-1].lower_bound); __libasr_index_2_7<=((int32_t) __libasr_created__array_section_9->dims[3-1].length + __libasr_created__array_section_9->dims[3-1].lower_bound - 1); __libasr_index_2_7++) {
        for (__libasr_index_1_9=((int32_t)__libasr_created__array_section_9->dims[2-1].lower_bound); __libasr_index_1_9<=((int32_t) __libasr_created__array_section_9->dims[2-1].length + __libasr_created__array_section_9->dims[2-1].lower_bound - 1); __libasr_index_1_9++) {
            for (__libasr_index_0_9=((int32_t)__libasr_created__array_section_9->dims[1-1].lower_bound); __libasr_index_0_9<=((int32_t) __libasr_created__array_section_9->dims[1-1].length + __libasr_created__array_section_9->dims[1-1].lower_bound - 1); __libasr_index_0_9++) {
                __libasr_created__array_section_9->data[((((0 + (__libasr_created__array_section_9->dims[0].stride * (__libasr_index_0_9 - __libasr_created__array_section_9->dims[0].lower_bound))) + (__libasr_created__array_section_9->dims[1].stride * (__libasr_index_1_9 - __libasr_created__array_section_9->dims[1].lower_bound))) + (__libasr_created__array_section_9->dims[2].stride * (__libasr_index_2_7 - __libasr_created__array_section_9->dims[2].lower_bound))) + __libasr_created__array_section_9->offset)] =   0.00000000000000000e+00;
            }
        }
    }
}

