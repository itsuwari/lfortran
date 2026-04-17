#include <inttypes.h>
#include <math.h>
#include <stdarg.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <lfortran_intrinsics.h>


struct dimension_descriptor
{
    int32_t lower_bound, length, stride;
};

struct l32___
{
    bool *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};

struct mctc_env_error__enum_stat {
 int64_t __lfortran_type_tag;
 int32_t success;
 int32_t fatal;
};

struct mctc_env_error__error_type {
 int64_t __lfortran_type_tag;
 int32_t stat;
 char * message;
};


struct r64___
{
    double *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct r32___
{
    float *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct r64_____
{
    double *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct r32_____
{
    float *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct r64_______
{
    double *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct str___
{
    char* *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct r64_________
{
    double *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct r32_______
{
    float *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


struct r32_________
{
    float *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};

struct tblite_wavefunction_type__wavefunction_type {
 int64_t __lfortran_type_tag;
 double kt;
 double nocc;
 double nuhf;
 int32_t nspin;
 struct r64___* nel;
 struct r64___* n0at;
 struct r64___* n0sh;
 struct r64_______* density;
 struct r64_______* coeff;
 struct r64_____* emo;
 struct r64_____* focc;
 struct r64_____* qat;
 struct r64_____* qsh;
 struct r64_______* dpat;
 struct r64_______* qpat;
 struct r64_________* dqatdr;
 struct r64_________* dqatdl;
 struct r64_________* dqshdr;
 struct r64_________* dqshdl;
};


struct Allocatable_r64______
{
    double *data;
    struct dimension_descriptor dims[32];
    int32_t n_dims;
    int32_t offset;
    bool is_allocated;
};


int32_t array_size(struct dimension_descriptor dims[], size_t n);
struct str___* array_constant_str___(int32_t n, ...);

bool __lfortran__lcompilers_Any_4_1_0_logical____0(struct l32___* mask, int32_t __1mask);
void __lfortran__lcompilers_get_command_argument_(int32_t number, int32_t *length, int32_t *status);
void __lfortran__lcompilers_get_command_argument_1(int32_t number, char * *value, int32_t *status);
void __lfortran__lcompilers_get_environment_variable_(char * name, int32_t *length, int32_t *status);
void __lfortran__lcompilers_get_environment_variable_1(char * name, char * *value, int32_t *status);
int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_(char * str, char * substr, bool back, int32_t kind);
int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_1(char * str, char * substr, bool back, int32_t kind);
int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_2(char * str, char * substr, bool back, int32_t kind);
void __lfortran_mctc_env_error__fatal_error__Allocatable_StructType_(struct mctc_env_error__error_type* *error, char * message, bool __libasr_is_present_message, int32_t stat, bool __libasr_is_present_stat);
void __lfortran_mctc_env_system__get_argument(int32_t idx, char * *arg);
void __lfortran_mctc_env_system__get_variable(char * var, char * *val);
bool __lfortran_mctc_env_system__is_windows();
bool __lfortran_mctc_env_system__is_unix();
double __lfortran_tblite_blas_level1__wrap_ddot(struct r64___* xvec, struct r64___* yvec);
double __lfortran_tblite_blas_level1__wrap_ddot12_real____0(struct r64___* xvec, int32_t __1xvec, struct r64_____* yvec);
double __lfortran_tblite_blas_level1__wrap_ddot21_real____1(struct r64_____* xvec, struct r64___* yvec, int32_t __1yvec);
double __lfortran_tblite_blas_level1__wrap_ddot22(struct r64_____* xvec, struct r64_____* yvec);
double __lfortran_tblite_blas_level1__wrap_ddot_real____0_real____1(struct r64___* xvec, int32_t __1xvec, struct r64___* yvec, int32_t __1yvec);
float __lfortran_tblite_blas_level1__wrap_sdot(struct r32___* xvec, struct r32___* yvec);
float __lfortran_tblite_blas_level1__wrap_sdot12_real____0(struct r32___* xvec, int32_t __1xvec, struct r32_____* yvec);
float __lfortran_tblite_blas_level1__wrap_sdot21_real____1(struct r32_____* xvec, struct r32___* yvec, int32_t __1yvec);
float __lfortran_tblite_blas_level1__wrap_sdot22(struct r32_____* xvec, struct r32_____* yvec);
float __lfortran_tblite_blas_level1__wrap_sdot_real____0_real____1(struct r32___* xvec, int32_t __1xvec, struct r32___* yvec, int32_t __1yvec);
float __lfortran_tblite_blas_level1__wrap_sdot12(struct r32___* xvec, struct r32_____* yvec);
float __lfortran_tblite_blas_level1__wrap_sdot21(struct r32_____* xvec, struct r32___* yvec);
double __lfortran_tblite_blas_level1__wrap_ddot12(struct r64___* xvec, struct r64_____* yvec);
double __lfortran_tblite_blas_level1__wrap_ddot21(struct r64_____* xvec, struct r64___* yvec);
void __lfortran_tblite_blas_level2__wrap_dgemv(struct r64_____* amat, struct r64___* xvec, struct r64___* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_dgemv312_real____1(struct r64_______* amat, struct r64___* xvec, int32_t __1xvec, struct r64_____* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_dgemv321_real____2(struct r64_______* amat, struct r64_____* xvec, struct r64___* yvec, int32_t __1yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_dgemv422(struct r64_________* amat, struct r64_____* xvec, struct r64_____* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_dgemv_real_______0_real____1_real____2(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64___* xvec, int32_t __1xvec, struct r64___* yvec, int32_t __1yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_dsymv_real_______0_real____1_real____2(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64___* xvec, int32_t __1xvec, struct r64___* yvec, int32_t __1yvec, char * uplo, bool __libasr_is_present_uplo, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level2__wrap_sgemv(struct r32_____* amat, struct r32___* xvec, struct r32___* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_sgemv312_real____1(struct r32_______* amat, struct r32___* xvec, int32_t __1xvec, struct r32_____* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_sgemv321_real____2(struct r32_______* amat, struct r32_____* xvec, struct r32___* yvec, int32_t __1yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_sgemv422(struct r32_________* amat, struct r32_____* xvec, struct r32_____* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_sgemv_real_______0_real____1_real____2(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32___* xvec, int32_t __1xvec, struct r32___* yvec, int32_t __1yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_ssymv_real_______0_real____1_real____2(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32___* xvec, int32_t __1xvec, struct r32___* yvec, int32_t __1yvec, char * uplo, bool __libasr_is_present_uplo, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level2__wrap_sgemv312(struct r32_______* amat, struct r32___* xvec, struct r32_____* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_sgemv321(struct r32_______* amat, struct r32_____* xvec, struct r32___* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_dgemv312(struct r64_______* amat, struct r64___* xvec, struct r64_____* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_dgemv321(struct r64_______* amat, struct r64_____* xvec, struct r64___* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans);
void __lfortran_tblite_blas_level2__wrap_ssymv(struct r32_____* amat, struct r32___* xvec, struct r32___* yvec, char * uplo, bool __libasr_is_present_uplo, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level2__wrap_dsymv(struct r64_____* amat, struct r64___* xvec, struct r64___* yvec, char * uplo, bool __libasr_is_present_uplo, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm(struct r64_____* amat, struct r64_____* bmat, struct r64_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm233_real_______0(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_______* bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm323_real_______1(struct r64_______* amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm332_real_______2(struct r64_______* amat, struct r64_______* bmat, struct r64_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm_real_______0_real_______1_real_______2(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r64_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dtrsm_real_______0_real_______1(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, double alpha, bool __libasr_is_present_alpha);
void __lfortran_tblite_blas_level3__wrap_sgemm(struct r32_____* amat, struct r32_____* bmat, struct r32_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_sgemm233_real_______0(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32_______* bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_sgemm323_real_______1(struct r32_______* amat, struct r32_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_sgemm332_real_______2(struct r32_______* amat, struct r32_______* bmat, struct r32_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_sgemm_real_______0_real_______1_real_______2(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r32_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_strsm_real_______0_real_______1(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32_____* bmat, int32_t __1bmat, int32_t __2bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, float alpha, bool __libasr_is_present_alpha);
void __lfortran_tblite_blas_level3__wrap_sgemm323(struct r32_______* amat, struct r32_____* bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_sgemm233(struct r32_____* amat, struct r32_______* bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_sgemm332(struct r32_______* amat, struct r32_______* bmat, struct r32_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm323(struct r64_______* amat, struct r64_____* bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm233(struct r64_____* amat, struct r64_______* bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm332(struct r64_______* amat, struct r64_______* bmat, struct r64_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_strsm(struct r32_____* amat, struct r32_____* bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, float alpha, bool __libasr_is_present_alpha);
void __lfortran_tblite_blas_level3__wrap_dtrsm(struct r64_____* amat, struct r64_____* bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, double alpha, bool __libasr_is_present_alpha);
void __lfortran_tblite_wavefunction_type__get_alpha_beta_occupation(double nocc, double nuhf, double *nalp, double *nbet);
void __lfortran_tblite_wavefunction_type__get_density_matrix_real____0_real_______1_real_______2(struct r64___* focc, int32_t __1focc, struct r64_____* coeff, int32_t __1coeff, int32_t __2coeff, struct r64_____* pmat, int32_t __1pmat, int32_t __2pmat);
void __lfortran_tblite_wavefunction_type__new_wavefunction__StructType(struct tblite_wavefunction_type__wavefunction_type* self, int32_t nat, int32_t nsh, int32_t nao, int32_t nspin, double kt, bool grad, bool __libasr_is_present_grad);




// Implementations
bool __lfortran__lcompilers_Any_4_1_0_logical____0(struct l32___* mask, int32_t __1mask)
{
    int32_t __1_i;
    bool _lcompilers_Any_4_1_0;
    _lcompilers_Any_4_1_0 = false;
    for (__1_i=1; __1_i<=__1mask + 1 - 1; __1_i++) {
        _lcompilers_Any_4_1_0 = _lcompilers_Any_4_1_0 || mask->data[(0 + (1 * (__1_i - 1)))];
    }
    return _lcompilers_Any_4_1_0;
}

;


;


void __lfortran__lcompilers_get_command_argument_(int32_t number, int32_t *length, int32_t *status)
{
    int32_t length_to_allocate;
    int32_t stat;
    length_to_allocate = 0;
    length_to_allocate = _lfortran_get_command_argument_length(number);
    (*length) = length_to_allocate;
    stat = _lfortran_get_command_argument_status(number, length_to_allocate, length_to_allocate);
    (*status) = stat;
}

;


;


;


void __lfortran__lcompilers_get_command_argument_1(int32_t number, char * *value, int32_t *status)
{
    char * command_argument_holder = NULL;
    int32_t length_to_allocate;
    int32_t stat;
    length_to_allocate = 0;
    length_to_allocate = _lfortran_get_command_argument_length(number);
    command_argument_holder = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), length_to_allocate);
    _lfortran_get_command_argument_value(number, command_argument_holder);
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &(*value), NULL, true, true, command_argument_holder, strlen(command_argument_holder));
    stat = _lfortran_get_command_argument_status(number, strlen((*value)), length_to_allocate);
    (*status) = stat;
}

;


;


void __lfortran__lcompilers_get_environment_variable_(char * name, int32_t *length, int32_t *status)
{
    (*length) = _lfortran_get_length_of_environment_variable(name, strlen(name));
    (*status) = _lfortran_get_environment_variable_status(name, strlen(name));
}

;


;


;


void __lfortran__lcompilers_get_environment_variable_1(char * name, char * *value, int32_t *status)
{
    char * envVar_string_holder = NULL;
    int32_t length_to_allocate;
    length_to_allocate = _lfortran_get_length_of_environment_variable(name, strlen(name));
    envVar_string_holder = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), length_to_allocate);
    _lfortran_get_environment_variable(name, strlen(name), envVar_string_holder);
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &(*value), NULL, true, true, envVar_string_holder, strlen(envVar_string_holder));
    // FIXME: deallocate(_lcompilers_get_environment_variable_1__envVar_string_holder, );
    (*status) = _lfortran_get_environment_variable_status(name, strlen(name));
}

int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_(char * str, char * substr, bool back, int32_t kind)
{
    int32_t _lcompilers_index_Allocatable_x5B_str_x5D_;
    bool found;
    int32_t i;
    int32_t j;
    int32_t k;
    int32_t pos;
    _lcompilers_index_Allocatable_x5B_str_x5D_ = 0;
    i = 1;
    found = true;
    if (strlen(str) < strlen(substr)) {
        found = false;
    }
    while (i < strlen(str) + 1 && found == true) {
        k = 0;
        j = 1;
        while (j <= strlen(substr) && found == true) {
            pos = i + k;
            if (str_compare(_lfortran_str_slice_alloc(_lfortran_get_default_allocator(), str, strlen(str), ((pos) - 1), ((1) > 0 ? (pos) : ((pos) - 2)), 1, true, true), pos - pos + 1, _lfortran_str_slice_alloc(_lfortran_get_default_allocator(), substr, strlen(substr), ((j) - 1), ((1) > 0 ? (j) : ((j) - 2)), 1, true, true), j - j + 1)  !=  0) {
                found = false;
            }
            j = j + 1;
            k = k + 1;
        }
        if (found == true) {
            _lcompilers_index_Allocatable_x5B_str_x5D_ = i;
            found = back;
        } else {
            found = true;
        }
        i = i + 1;
    }
    return _lcompilers_index_Allocatable_x5B_str_x5D_;
}

int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_1(char * str, char * substr, bool back, int32_t kind)
{
    int32_t _lcompilers_index_Allocatable_x5B_str_x5D_1;
    bool found;
    int32_t i;
    int32_t j;
    int32_t k;
    int32_t pos;
    _lcompilers_index_Allocatable_x5B_str_x5D_1 = 0;
    i = 1;
    found = true;
    if (strlen(str) < strlen(substr)) {
        found = false;
    }
    while (i < strlen(str) + 1 && found == true) {
        k = 0;
        j = 1;
        while (j <= strlen(substr) && found == true) {
            pos = i + k;
            if (str_compare(_lfortran_str_slice_alloc(_lfortran_get_default_allocator(), str, strlen(str), ((pos) - 1), ((1) > 0 ? (pos) : ((pos) - 2)), 1, true, true), pos - pos + 1, _lfortran_str_slice_alloc(_lfortran_get_default_allocator(), substr, strlen(substr), ((j) - 1), ((1) > 0 ? (j) : ((j) - 2)), 1, true, true), j - j + 1)  !=  0) {
                found = false;
            }
            j = j + 1;
            k = k + 1;
        }
        if (found == true) {
            _lcompilers_index_Allocatable_x5B_str_x5D_1 = i;
            found = back;
        } else {
            found = true;
        }
        i = i + 1;
    }
    return _lcompilers_index_Allocatable_x5B_str_x5D_1;
}

int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_2(char * str, char * substr, bool back, int32_t kind)
{
    int32_t _lcompilers_index_Allocatable_x5B_str_x5D_2;
    bool found;
    int32_t i;
    int32_t j;
    int32_t k;
    int32_t pos;
    _lcompilers_index_Allocatable_x5B_str_x5D_2 = 0;
    i = 1;
    found = true;
    if (strlen(str) < strlen(substr)) {
        found = false;
    }
    while (i < strlen(str) + 1 && found == true) {
        k = 0;
        j = 1;
        while (j <= strlen(substr) && found == true) {
            pos = i + k;
            if (str_compare(_lfortran_str_slice_alloc(_lfortran_get_default_allocator(), str, strlen(str), ((pos) - 1), ((1) > 0 ? (pos) : ((pos) - 2)), 1, true, true), pos - pos + 1, _lfortran_str_slice_alloc(_lfortran_get_default_allocator(), substr, strlen(substr), ((j) - 1), ((1) > 0 ? (j) : ((j) - 2)), 1, true, true), j - j + 1)  !=  0) {
                found = false;
            }
            j = j + 1;
            k = k + 1;
        }
        if (found == true) {
            _lcompilers_index_Allocatable_x5B_str_x5D_2 = i;
            found = back;
        } else {
            found = true;
        }
        i = i + 1;
    }
    return _lcompilers_index_Allocatable_x5B_str_x5D_2;
}

const int32_t mctc_env_accuracy__dp = 8;
const int32_t mctc_env_accuracy__i1 = 1;
const int32_t mctc_env_accuracy__i2 = 2;
const int32_t mctc_env_accuracy__i4 = 4;
const int32_t mctc_env_accuracy__i8 = 8;
const int32_t mctc_env_accuracy__sp = 4;
const int32_t mctc_env_accuracy__wp = 8;
const struct mctc_env_error__enum_stat mctc_env_error__mctc_stat = {.__lfortran_type_tag = 153096130620335663, .success = 0, .fatal = 1};
void __lfortran_mctc_env_error__fatal_error__Allocatable_StructType_(struct mctc_env_error__error_type* *error, char * message, bool __libasr_is_present_message, int32_t stat, bool __libasr_is_present_stat)
{
    if ((((*error)) != NULL)) {
        // FIXME: deallocate(mctc_env_error__fatal_error__error, );
    }
    (*error) = (struct mctc_env_error__error_type*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof(struct mctc_env_error__error_type));
    memset((*error), 0, sizeof(struct mctc_env_error__error_type));
    ((struct mctc_env_error__error_type*)((*error)))->message = NULL;
    (*error)->__lfortran_type_tag = 1045734792439692730;
    if (__libasr_is_present_stat) {
        (*error)->stat = stat;
    } else {
        (*error)->stat = mctc_env_error__mctc_stat.fatal;
    }
    if (__libasr_is_present_message) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &(*error)->message, NULL, true, true, message, strlen(message));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &(*error)->message, NULL, true, true, (char*)"Fatal error", strlen((char*)"Fatal error"));
    }
}

void __lfortran_mctc_env_system__get_argument(int32_t idx, char * *arg)
{
    int32_t length;
    int32_t stat;
    if ((((*arg)) != NULL)) {
        // FIXME: deallocate(mctc_env_system__get_argument__arg, );
    }
    __lfortran__lcompilers_get_command_argument_(idx, &length, &stat);
    if (stat != 0) {
        return;
    }
    (*arg) = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), length);
    if (stat != 0) {
        return;
    }
    if (length > 0) {
        __lfortran__lcompilers_get_command_argument_1(idx, &(*arg), &stat);
        if (stat != 0) {
            // FIXME: deallocate(mctc_env_system__get_argument__arg, );
            return;
        }
    }
}

void __lfortran_mctc_env_system__get_variable(char * var, char * *val)
{
    int32_t length;
    int32_t stat;
    if ((((*val)) != NULL)) {
        // FIXME: deallocate(mctc_env_system__get_variable__val, );
    }
    __lfortran__lcompilers_get_environment_variable_(var, &length, &stat);
    if (stat != 0) {
        return;
    }
    (*val) = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), length);
    if (stat != 0) {
        return;
    }
    if (length > 0) {
        __lfortran__lcompilers_get_environment_variable_1(var, &(*val), &stat);
        if (stat != 0) {
            // FIXME: deallocate(mctc_env_system__get_variable__val, );
            return;
        }
    }
}

bool __lfortran_mctc_env_system__is_windows()
{
    bool is_windows;
    char * tmp = NULL;
    is_windows = false;
    __lfortran_mctc_env_system__get_variable((char*)"OS", &tmp);
    if (((tmp) != NULL)) {
        is_windows = __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_(tmp, (char*)"Windows_NT", false, 4) > 0;
    }
    if (!is_windows) {
        __lfortran_mctc_env_system__get_variable((char*)"OSTYPE", &tmp);
        if (((tmp) != NULL)) {
            is_windows = __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_1(tmp, (char*)"win", false, 4) > 0 || __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_2(tmp, (char*)"msys", false, 4) > 0;
        }
    }
    return is_windows;
}

bool __lfortran_mctc_env_system__is_unix()
{
    bool is_unix;
    is_unix = !__lfortran_mctc_env_system__is_windows();
    return is_unix;
}

double __lfortran_tblite_blas_level1__wrap_ddot(struct r64___* xvec, struct r64___* yvec)
{
    int32_t __lcompilers_i_0;
    int32_t __lcompilers_i_01;
    struct r64___ __libasr_created__function_call_ddot_value;
    struct r64___* __libasr_created__function_call_ddot = &__libasr_created__function_call_ddot_value;
    struct r64___ __libasr_created__function_call_ddot1_value;
    struct r64___* __libasr_created__function_call_ddot1 = &__libasr_created__function_call_ddot1_value;
    double dot;
    int32_t incx;
    int32_t incy;
    int32_t n;
    incx = 1;
    incy = 1;
    n = ((int32_t) array_size(xvec->dims, 1));
    if (!(!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level1__wrap_ddot____libasr_created__function_call_ddot, );
        __libasr_created__function_call_ddot->n_dims = 1;
        __libasr_created__function_call_ddot->dims[0].lower_bound = 1;
        __libasr_created__function_call_ddot->dims[0].length = ((int32_t) xvec->dims[1-1].length + xvec->dims[1-1].lower_bound - 1);
        __libasr_created__function_call_ddot->dims[0].stride = 1;
        __libasr_created__function_call_ddot->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__function_call_ddot->dims[0].length*sizeof(double));
        __libasr_created__function_call_ddot->is_allocated = true;
        for (__lcompilers_i_0=((int32_t)__libasr_created__function_call_ddot->dims[1-1].lower_bound); __lcompilers_i_0<=((int32_t) __libasr_created__function_call_ddot->dims[1-1].length + __libasr_created__function_call_ddot->dims[1-1].lower_bound - 1); __lcompilers_i_0++) {
            __libasr_created__function_call_ddot->data[((0 + (__libasr_created__function_call_ddot->dims[0].stride * (__lcompilers_i_0 - __libasr_created__function_call_ddot->dims[0].lower_bound))) + __libasr_created__function_call_ddot->offset)] = xvec->data[((0 + (xvec->dims[0].stride * (__lcompilers_i_0 - xvec->dims[0].lower_bound))) + xvec->offset)];
        }
    } else {
        __libasr_created__function_call_ddot = xvec;
    }
    if (!(!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level1__wrap_ddot____libasr_created__function_call_ddot1, );
        __libasr_created__function_call_ddot1->n_dims = 1;
        __libasr_created__function_call_ddot1->dims[0].lower_bound = 1;
        __libasr_created__function_call_ddot1->dims[0].length = ((int32_t) yvec->dims[1-1].length + yvec->dims[1-1].lower_bound - 1);
        __libasr_created__function_call_ddot1->dims[0].stride = 1;
        __libasr_created__function_call_ddot1->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__function_call_ddot1->dims[0].length*sizeof(double));
        __libasr_created__function_call_ddot1->is_allocated = true;
        for (__lcompilers_i_01=((int32_t)__libasr_created__function_call_ddot1->dims[1-1].lower_bound); __lcompilers_i_01<=((int32_t) __libasr_created__function_call_ddot1->dims[1-1].length + __libasr_created__function_call_ddot1->dims[1-1].lower_bound - 1); __lcompilers_i_01++) {
            __libasr_created__function_call_ddot1->data[((0 + (__libasr_created__function_call_ddot1->dims[0].stride * (__lcompilers_i_01 - __libasr_created__function_call_ddot1->dims[0].lower_bound))) + __libasr_created__function_call_ddot1->offset)] = yvec->data[((0 + (yvec->dims[0].stride * (__lcompilers_i_01 - yvec->dims[0].lower_bound))) + yvec->offset)];
        }
    } else {
        __libasr_created__function_call_ddot1 = yvec;
    }
    dot = ddot(n, __libasr_created__function_call_ddot, incx, __libasr_created__function_call_ddot1, incy);
    if ((!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        __libasr_created__function_call_ddot = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level1__wrap_ddot____libasr_created__function_call_ddot, );
    }
    if ((!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        __libasr_created__function_call_ddot1 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level1__wrap_ddot____libasr_created__function_call_ddot1, );
    }
    return dot;
}

double __lfortran_tblite_blas_level1__wrap_ddot12_real____0(struct r64___* xvec, int32_t __1xvec, struct r64_____* yvec)
{
    double dot;
    struct r64___ yptr_value;
    struct r64___* yptr = &yptr_value;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t) array_size(yvec->dims, 2))) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_ddot(xvec, yptr);
    return dot;
}

double __lfortran_tblite_blas_level1__wrap_ddot21_real____1(struct r64_____* xvec, struct r64___* yvec, int32_t __1yvec)
{
    double dot;
    struct r64___ xptr_value;
    struct r64___* xptr = &xptr_value;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t) array_size(xvec->dims, 2))) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_ddot(xptr, yvec);
    return dot;
}

double __lfortran_tblite_blas_level1__wrap_ddot22(struct r64_____* xvec, struct r64_____* yvec)
{
    double dot;
    struct r64___ xptr_value;
    struct r64___* xptr = &xptr_value;
    struct r64___ yptr_value;
    struct r64___* yptr = &yptr_value;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t) array_size(xvec->dims, 2))) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t) array_size(yvec->dims, 2))) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_ddot(xptr, yptr);
    return dot;
}

double __lfortran_tblite_blas_level1__wrap_ddot_real____0_real____1(struct r64___* xvec, int32_t __1xvec, struct r64___* yvec, int32_t __1yvec)
{
    double dot;
    int32_t incx;
    int32_t incy;
    int32_t n;
    incx = 1;
    incy = 1;
    n = 1*__1xvec;
    dot = ddot(n, xvec, incx, yvec, incy);
    return dot;
}

float __lfortran_tblite_blas_level1__wrap_sdot(struct r32___* xvec, struct r32___* yvec)
{
    int32_t __lcompilers_i_0;
    int32_t __lcompilers_i_01;
    struct r32___ __libasr_created__function_call_sdot_value;
    struct r32___* __libasr_created__function_call_sdot = &__libasr_created__function_call_sdot_value;
    struct r32___ __libasr_created__function_call_sdot1_value;
    struct r32___* __libasr_created__function_call_sdot1 = &__libasr_created__function_call_sdot1_value;
    float dot;
    int32_t incx;
    int32_t incy;
    int32_t n;
    incx = 1;
    incy = 1;
    n = ((int32_t) array_size(xvec->dims, 1));
    if (!(!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level1__wrap_sdot____libasr_created__function_call_sdot, );
        __libasr_created__function_call_sdot->n_dims = 1;
        __libasr_created__function_call_sdot->dims[0].lower_bound = 1;
        __libasr_created__function_call_sdot->dims[0].length = ((int32_t) xvec->dims[1-1].length + xvec->dims[1-1].lower_bound - 1);
        __libasr_created__function_call_sdot->dims[0].stride = 1;
        __libasr_created__function_call_sdot->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__function_call_sdot->dims[0].length*sizeof(float));
        __libasr_created__function_call_sdot->is_allocated = true;
        for (__lcompilers_i_0=((int32_t)__libasr_created__function_call_sdot->dims[1-1].lower_bound); __lcompilers_i_0<=((int32_t) __libasr_created__function_call_sdot->dims[1-1].length + __libasr_created__function_call_sdot->dims[1-1].lower_bound - 1); __lcompilers_i_0++) {
            __libasr_created__function_call_sdot->data[((0 + (__libasr_created__function_call_sdot->dims[0].stride * (__lcompilers_i_0 - __libasr_created__function_call_sdot->dims[0].lower_bound))) + __libasr_created__function_call_sdot->offset)] = xvec->data[((0 + (xvec->dims[0].stride * (__lcompilers_i_0 - xvec->dims[0].lower_bound))) + xvec->offset)];
        }
    } else {
        __libasr_created__function_call_sdot = xvec;
    }
    if (!(!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level1__wrap_sdot____libasr_created__function_call_sdot1, );
        __libasr_created__function_call_sdot1->n_dims = 1;
        __libasr_created__function_call_sdot1->dims[0].lower_bound = 1;
        __libasr_created__function_call_sdot1->dims[0].length = ((int32_t) yvec->dims[1-1].length + yvec->dims[1-1].lower_bound - 1);
        __libasr_created__function_call_sdot1->dims[0].stride = 1;
        __libasr_created__function_call_sdot1->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__function_call_sdot1->dims[0].length*sizeof(float));
        __libasr_created__function_call_sdot1->is_allocated = true;
        for (__lcompilers_i_01=((int32_t)__libasr_created__function_call_sdot1->dims[1-1].lower_bound); __lcompilers_i_01<=((int32_t) __libasr_created__function_call_sdot1->dims[1-1].length + __libasr_created__function_call_sdot1->dims[1-1].lower_bound - 1); __lcompilers_i_01++) {
            __libasr_created__function_call_sdot1->data[((0 + (__libasr_created__function_call_sdot1->dims[0].stride * (__lcompilers_i_01 - __libasr_created__function_call_sdot1->dims[0].lower_bound))) + __libasr_created__function_call_sdot1->offset)] = yvec->data[((0 + (yvec->dims[0].stride * (__lcompilers_i_01 - yvec->dims[0].lower_bound))) + yvec->offset)];
        }
    } else {
        __libasr_created__function_call_sdot1 = yvec;
    }
    dot = sdot(n, __libasr_created__function_call_sdot, incx, __libasr_created__function_call_sdot1, incy);
    if ((!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        __libasr_created__function_call_sdot = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level1__wrap_sdot____libasr_created__function_call_sdot, );
    }
    if ((!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        __libasr_created__function_call_sdot1 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level1__wrap_sdot____libasr_created__function_call_sdot1, );
    }
    return dot;
}

float __lfortran_tblite_blas_level1__wrap_sdot12_real____0(struct r32___* xvec, int32_t __1xvec, struct r32_____* yvec)
{
    float dot;
    struct r32___ yptr_value;
    struct r32___* yptr = &yptr_value;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t) array_size(yvec->dims, 2))) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_sdot(xvec, yptr);
    return dot;
}

float __lfortran_tblite_blas_level1__wrap_sdot21_real____1(struct r32_____* xvec, struct r32___* yvec, int32_t __1yvec)
{
    float dot;
    struct r32___ xptr_value;
    struct r32___* xptr = &xptr_value;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t) array_size(xvec->dims, 2))) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_sdot(xptr, yvec);
    return dot;
}

float __lfortran_tblite_blas_level1__wrap_sdot22(struct r32_____* xvec, struct r32_____* yvec)
{
    float dot;
    struct r32___ xptr_value;
    struct r32___* xptr = &xptr_value;
    struct r32___ yptr_value;
    struct r32___* yptr = &yptr_value;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t) array_size(xvec->dims, 2))) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t) array_size(yvec->dims, 2))) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_sdot(xptr, yptr);
    return dot;
}

float __lfortran_tblite_blas_level1__wrap_sdot_real____0_real____1(struct r32___* xvec, int32_t __1xvec, struct r32___* yvec, int32_t __1yvec)
{
    float dot;
    int32_t incx;
    int32_t incy;
    int32_t n;
    incx = 1;
    incy = 1;
    n = 1*__1xvec;
    dot = sdot(n, xvec, incx, yvec, incy);
    return dot;
}

float __lfortran_tblite_blas_level1__wrap_sdot12(struct r32___* xvec, struct r32_____* yvec)
{
    float dot;
    struct r32___ yptr_value;
    struct r32___* yptr = &yptr_value;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t) array_size(yvec->dims, 2))) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_sdot(xvec, yptr);
    return dot;
}

float __lfortran_tblite_blas_level1__wrap_sdot21(struct r32_____* xvec, struct r32___* yvec)
{
    float dot;
    struct r32___ xptr_value;
    struct r32___* xptr = &xptr_value;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t) array_size(xvec->dims, 2))) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_sdot(xptr, yvec);
    return dot;
}

double __lfortran_tblite_blas_level1__wrap_ddot12(struct r64___* xvec, struct r64_____* yvec)
{
    double dot;
    struct r64___ yptr_value;
    struct r64___* yptr = &yptr_value;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t) array_size(yvec->dims, 2))) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_ddot(xvec, yptr);
    return dot;
}

double __lfortran_tblite_blas_level1__wrap_ddot21(struct r64_____* xvec, struct r64___* yvec)
{
    double dot;
    struct r64___ xptr_value;
    struct r64___* xptr = &xptr_value;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t) array_size(xvec->dims, 2))) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    dot = __lfortran_tblite_blas_level1__wrap_ddot(xptr, yvec);
    return dot;
}

void __lfortran_tblite_blas_level2__wrap_dgemv(struct r64_____* amat, struct r64___* xvec, struct r64___* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    int32_t __lcompilers_i_0;
    int32_t __lcompilers_i_01;
    int32_t __lcompilers_i_02;
    int32_t __lcompilers_i_1;
    struct r64_____ __libasr_created__subroutine_call_dgemv_value;
    struct r64_____* __libasr_created__subroutine_call_dgemv = &__libasr_created__subroutine_call_dgemv_value;
    struct r64___ __libasr_created__subroutine_call_dgemv1_value;
    struct r64___* __libasr_created__subroutine_call_dgemv1 = &__libasr_created__subroutine_call_dgemv1_value;
    struct r64___ __libasr_created__subroutine_call_dgemv2_value;
    struct r64___* __libasr_created__subroutine_call_dgemv2 = &__libasr_created__subroutine_call_dgemv2_value;
    double a;
    double b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (double)(0);
    }
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    m = ((int32_t)amat->dims[1-1].length);
    n = ((int32_t)amat->dims[2-1].length);
    if (!(!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level2__wrap_dgemv____libasr_created__subroutine_call_dgemv, );
        __libasr_created__subroutine_call_dgemv->n_dims = 2;
        __libasr_created__subroutine_call_dgemv->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_dgemv->dims[1].length = ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemv->dims[1].stride = 1;
        __libasr_created__subroutine_call_dgemv->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_dgemv->dims[0].length = ((int32_t) amat->dims[1-1].length + amat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemv->dims[0].stride = (1 * ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_dgemv->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_dgemv->dims[1].length*__libasr_created__subroutine_call_dgemv->dims[0].length*sizeof(double));
        __libasr_created__subroutine_call_dgemv->is_allocated = true;
        for (__lcompilers_i_1=((int32_t)__libasr_created__subroutine_call_dgemv->dims[2-1].lower_bound); __lcompilers_i_1<=((int32_t) __libasr_created__subroutine_call_dgemv->dims[2-1].length + __libasr_created__subroutine_call_dgemv->dims[2-1].lower_bound - 1); __lcompilers_i_1++) {
            for (__lcompilers_i_0=((int32_t)__libasr_created__subroutine_call_dgemv->dims[1-1].lower_bound); __lcompilers_i_0<=((int32_t) __libasr_created__subroutine_call_dgemv->dims[1-1].length + __libasr_created__subroutine_call_dgemv->dims[1-1].lower_bound - 1); __lcompilers_i_0++) {
                __libasr_created__subroutine_call_dgemv->data[(((0 + (__libasr_created__subroutine_call_dgemv->dims[0].stride * (__lcompilers_i_0 - __libasr_created__subroutine_call_dgemv->dims[0].lower_bound))) + (__libasr_created__subroutine_call_dgemv->dims[1].stride * (__lcompilers_i_1 - __libasr_created__subroutine_call_dgemv->dims[1].lower_bound))) + __libasr_created__subroutine_call_dgemv->offset)] = amat->data[(((0 + (amat->dims[0].stride * (__lcompilers_i_0 - amat->dims[0].lower_bound))) + (amat->dims[1].stride * (__lcompilers_i_1 - amat->dims[1].lower_bound))) + amat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_dgemv = amat;
    }
    if (!(!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level2__wrap_dgemv____libasr_created__subroutine_call_dgemv1, );
        __libasr_created__subroutine_call_dgemv1->n_dims = 1;
        __libasr_created__subroutine_call_dgemv1->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_dgemv1->dims[0].length = ((int32_t) xvec->dims[1-1].length + xvec->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemv1->dims[0].stride = 1;
        __libasr_created__subroutine_call_dgemv1->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_dgemv1->dims[0].length*sizeof(double));
        __libasr_created__subroutine_call_dgemv1->is_allocated = true;
        for (__lcompilers_i_01=((int32_t)__libasr_created__subroutine_call_dgemv1->dims[1-1].lower_bound); __lcompilers_i_01<=((int32_t) __libasr_created__subroutine_call_dgemv1->dims[1-1].length + __libasr_created__subroutine_call_dgemv1->dims[1-1].lower_bound - 1); __lcompilers_i_01++) {
            __libasr_created__subroutine_call_dgemv1->data[((0 + (__libasr_created__subroutine_call_dgemv1->dims[0].stride * (__lcompilers_i_01 - __libasr_created__subroutine_call_dgemv1->dims[0].lower_bound))) + __libasr_created__subroutine_call_dgemv1->offset)] = xvec->data[((0 + (xvec->dims[0].stride * (__lcompilers_i_01 - xvec->dims[0].lower_bound))) + xvec->offset)];
        }
    } else {
        __libasr_created__subroutine_call_dgemv1 = xvec;
    }
    if (!(!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level2__wrap_dgemv____libasr_created__subroutine_call_dgemv2, );
        __libasr_created__subroutine_call_dgemv2->n_dims = 1;
        __libasr_created__subroutine_call_dgemv2->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_dgemv2->dims[0].length = ((int32_t) yvec->dims[1-1].length + yvec->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemv2->dims[0].stride = 1;
        __libasr_created__subroutine_call_dgemv2->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_dgemv2->dims[0].length*sizeof(double));
        __libasr_created__subroutine_call_dgemv2->is_allocated = true;
        for (__lcompilers_i_02=((int32_t)__libasr_created__subroutine_call_dgemv2->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) __libasr_created__subroutine_call_dgemv2->dims[1-1].length + __libasr_created__subroutine_call_dgemv2->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
            __libasr_created__subroutine_call_dgemv2->data[((0 + (__libasr_created__subroutine_call_dgemv2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_dgemv2->dims[0].lower_bound))) + __libasr_created__subroutine_call_dgemv2->offset)] = yvec->data[((0 + (yvec->dims[0].stride * (__lcompilers_i_02 - yvec->dims[0].lower_bound))) + yvec->offset)];
        }
    } else {
        __libasr_created__subroutine_call_dgemv2 = yvec;
    }
    dgemv(tra, m, n, a, __libasr_created__subroutine_call_dgemv, lda, __libasr_created__subroutine_call_dgemv1, incx, b, __libasr_created__subroutine_call_dgemv2, incy);
    if ((!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        __libasr_created__subroutine_call_dgemv = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level2__wrap_dgemv____libasr_created__subroutine_call_dgemv, );
    }
    if ((!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        __libasr_created__subroutine_call_dgemv1 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level2__wrap_dgemv____libasr_created__subroutine_call_dgemv1, );
    }
    if (!(!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        for (__lcompilers_i_02=((int32_t)yvec->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) yvec->dims[1-1].length + yvec->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
            yvec->data[((0 + (yvec->dims[0].stride * (__lcompilers_i_02 - yvec->dims[0].lower_bound))) + yvec->offset)] = __libasr_created__subroutine_call_dgemv2->data[((0 + (__libasr_created__subroutine_call_dgemv2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_dgemv2->dims[0].lower_bound))) + __libasr_created__subroutine_call_dgemv2->offset)];
        }
    }
    if ((!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        __libasr_created__subroutine_call_dgemv2 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level2__wrap_dgemv____libasr_created__subroutine_call_dgemv2, );
    }
}

void __lfortran_tblite_blas_level2__wrap_dgemv312_real____1(struct r64_______* amat, struct r64___* xvec, int32_t __1xvec, struct r64_____* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r64___ yptr_value;
    struct r64___* yptr = &yptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_dgemv(aptr, xvec, yptr, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_dgemv321_real____2(struct r64_______* amat, struct r64_____* xvec, struct r64___* yvec, int32_t __1yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r64___ xptr_value;
    struct r64___* xptr = &xptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_dgemv(aptr, xptr, yvec, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_dgemv422(struct r64_________* amat, struct r64_____* xvec, struct r64_____* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r64___ xptr_value;
    struct r64___* xptr = &xptr_value;
    struct r64___ yptr_value;
    struct r64___* yptr = &yptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    aptr->data = amat->data;
    aptr->offset = amat->offset;
    aptr->is_allocated = amat->is_allocated;
    aptr->dims[0].lower_bound = 1;
    aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
    aptr->dims[0].stride = (1);
    aptr->dims[1].lower_bound = 1;
    aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)*((int32_t)amat->dims[4-1].length)) - (1))/(1) + 1);
    aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
    aptr->n_dims = 2;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    __lfortran_tblite_blas_level2__wrap_dgemv(aptr, xptr, yptr, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_dgemv_real_______0_real____1_real____2(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64___* xvec, int32_t __1xvec, struct r64___* yvec, int32_t __1yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    double a;
    double b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (double)(0);
    }
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    m = __1amat;
    n = __2amat;
    dgemv(tra, m, n, a, amat, lda, xvec, incx, b, yvec, incy);
}

void __lfortran_tblite_blas_level2__wrap_dsymv_real_______0_real____1_real____2(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64___* xvec, int32_t __1xvec, struct r64___* yvec, int32_t __1yvec, char * uplo, bool __libasr_is_present_uplo, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    double a;
    double b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t n;
    char * ula = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (double)(0);
    }
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    n = __2amat;
    dsymv(ula, n, a, amat, lda, xvec, incx, b, yvec, incy);
}

void __lfortran_tblite_blas_level2__wrap_sgemv(struct r32_____* amat, struct r32___* xvec, struct r32___* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    int32_t __lcompilers_i_0;
    int32_t __lcompilers_i_01;
    int32_t __lcompilers_i_02;
    int32_t __lcompilers_i_1;
    struct r32_____ __libasr_created__subroutine_call_sgemv_value;
    struct r32_____* __libasr_created__subroutine_call_sgemv = &__libasr_created__subroutine_call_sgemv_value;
    struct r32___ __libasr_created__subroutine_call_sgemv1_value;
    struct r32___* __libasr_created__subroutine_call_sgemv1 = &__libasr_created__subroutine_call_sgemv1_value;
    struct r32___ __libasr_created__subroutine_call_sgemv2_value;
    struct r32___* __libasr_created__subroutine_call_sgemv2 = &__libasr_created__subroutine_call_sgemv2_value;
    float a;
    float b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (float)(0);
    }
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    m = ((int32_t)amat->dims[1-1].length);
    n = ((int32_t)amat->dims[2-1].length);
    if (!(!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level2__wrap_sgemv____libasr_created__subroutine_call_sgemv, );
        __libasr_created__subroutine_call_sgemv->n_dims = 2;
        __libasr_created__subroutine_call_sgemv->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_sgemv->dims[1].length = ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemv->dims[1].stride = 1;
        __libasr_created__subroutine_call_sgemv->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_sgemv->dims[0].length = ((int32_t) amat->dims[1-1].length + amat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemv->dims[0].stride = (1 * ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_sgemv->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_sgemv->dims[1].length*__libasr_created__subroutine_call_sgemv->dims[0].length*sizeof(float));
        __libasr_created__subroutine_call_sgemv->is_allocated = true;
        for (__lcompilers_i_1=((int32_t)__libasr_created__subroutine_call_sgemv->dims[2-1].lower_bound); __lcompilers_i_1<=((int32_t) __libasr_created__subroutine_call_sgemv->dims[2-1].length + __libasr_created__subroutine_call_sgemv->dims[2-1].lower_bound - 1); __lcompilers_i_1++) {
            for (__lcompilers_i_0=((int32_t)__libasr_created__subroutine_call_sgemv->dims[1-1].lower_bound); __lcompilers_i_0<=((int32_t) __libasr_created__subroutine_call_sgemv->dims[1-1].length + __libasr_created__subroutine_call_sgemv->dims[1-1].lower_bound - 1); __lcompilers_i_0++) {
                __libasr_created__subroutine_call_sgemv->data[(((0 + (__libasr_created__subroutine_call_sgemv->dims[0].stride * (__lcompilers_i_0 - __libasr_created__subroutine_call_sgemv->dims[0].lower_bound))) + (__libasr_created__subroutine_call_sgemv->dims[1].stride * (__lcompilers_i_1 - __libasr_created__subroutine_call_sgemv->dims[1].lower_bound))) + __libasr_created__subroutine_call_sgemv->offset)] = amat->data[(((0 + (amat->dims[0].stride * (__lcompilers_i_0 - amat->dims[0].lower_bound))) + (amat->dims[1].stride * (__lcompilers_i_1 - amat->dims[1].lower_bound))) + amat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_sgemv = amat;
    }
    if (!(!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level2__wrap_sgemv____libasr_created__subroutine_call_sgemv1, );
        __libasr_created__subroutine_call_sgemv1->n_dims = 1;
        __libasr_created__subroutine_call_sgemv1->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_sgemv1->dims[0].length = ((int32_t) xvec->dims[1-1].length + xvec->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemv1->dims[0].stride = 1;
        __libasr_created__subroutine_call_sgemv1->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_sgemv1->dims[0].length*sizeof(float));
        __libasr_created__subroutine_call_sgemv1->is_allocated = true;
        for (__lcompilers_i_01=((int32_t)__libasr_created__subroutine_call_sgemv1->dims[1-1].lower_bound); __lcompilers_i_01<=((int32_t) __libasr_created__subroutine_call_sgemv1->dims[1-1].length + __libasr_created__subroutine_call_sgemv1->dims[1-1].lower_bound - 1); __lcompilers_i_01++) {
            __libasr_created__subroutine_call_sgemv1->data[((0 + (__libasr_created__subroutine_call_sgemv1->dims[0].stride * (__lcompilers_i_01 - __libasr_created__subroutine_call_sgemv1->dims[0].lower_bound))) + __libasr_created__subroutine_call_sgemv1->offset)] = xvec->data[((0 + (xvec->dims[0].stride * (__lcompilers_i_01 - xvec->dims[0].lower_bound))) + xvec->offset)];
        }
    } else {
        __libasr_created__subroutine_call_sgemv1 = xvec;
    }
    if (!(!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        // FIXME: deallocate(tblite_blas_level2__wrap_sgemv____libasr_created__subroutine_call_sgemv2, );
        __libasr_created__subroutine_call_sgemv2->n_dims = 1;
        __libasr_created__subroutine_call_sgemv2->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_sgemv2->dims[0].length = ((int32_t) yvec->dims[1-1].length + yvec->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemv2->dims[0].stride = 1;
        __libasr_created__subroutine_call_sgemv2->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_sgemv2->dims[0].length*sizeof(float));
        __libasr_created__subroutine_call_sgemv2->is_allocated = true;
        for (__lcompilers_i_02=((int32_t)__libasr_created__subroutine_call_sgemv2->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) __libasr_created__subroutine_call_sgemv2->dims[1-1].length + __libasr_created__subroutine_call_sgemv2->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
            __libasr_created__subroutine_call_sgemv2->data[((0 + (__libasr_created__subroutine_call_sgemv2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_sgemv2->dims[0].lower_bound))) + __libasr_created__subroutine_call_sgemv2->offset)] = yvec->data[((0 + (yvec->dims[0].stride * (__lcompilers_i_02 - yvec->dims[0].lower_bound))) + yvec->offset)];
        }
    } else {
        __libasr_created__subroutine_call_sgemv2 = yvec;
    }
    sgemv(tra, m, n, a, __libasr_created__subroutine_call_sgemv, lda, __libasr_created__subroutine_call_sgemv1, incx, b, __libasr_created__subroutine_call_sgemv2, incy);
    if ((!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        __libasr_created__subroutine_call_sgemv = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level2__wrap_sgemv____libasr_created__subroutine_call_sgemv, );
    }
    if ((!xvec->is_allocated || (true && (xvec->dims[0].stride == 1)))) {
        __libasr_created__subroutine_call_sgemv1 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level2__wrap_sgemv____libasr_created__subroutine_call_sgemv1, );
    }
    if (!(!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        for (__lcompilers_i_02=((int32_t)yvec->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) yvec->dims[1-1].length + yvec->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
            yvec->data[((0 + (yvec->dims[0].stride * (__lcompilers_i_02 - yvec->dims[0].lower_bound))) + yvec->offset)] = __libasr_created__subroutine_call_sgemv2->data[((0 + (__libasr_created__subroutine_call_sgemv2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_sgemv2->dims[0].lower_bound))) + __libasr_created__subroutine_call_sgemv2->offset)];
        }
    }
    if ((!yvec->is_allocated || (true && (yvec->dims[0].stride == 1)))) {
        __libasr_created__subroutine_call_sgemv2 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level2__wrap_sgemv____libasr_created__subroutine_call_sgemv2, );
    }
}

void __lfortran_tblite_blas_level2__wrap_sgemv312_real____1(struct r32_______* amat, struct r32___* xvec, int32_t __1xvec, struct r32_____* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r32___ yptr_value;
    struct r32___* yptr = &yptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_sgemv(aptr, xvec, yptr, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_sgemv321_real____2(struct r32_______* amat, struct r32_____* xvec, struct r32___* yvec, int32_t __1yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r32___ xptr_value;
    struct r32___* xptr = &xptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_sgemv(aptr, xptr, yvec, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_sgemv422(struct r32_________* amat, struct r32_____* xvec, struct r32_____* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r32___ xptr_value;
    struct r32___* xptr = &xptr_value;
    struct r32___ yptr_value;
    struct r32___* yptr = &yptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    aptr->data = amat->data;
    aptr->offset = amat->offset;
    aptr->is_allocated = amat->is_allocated;
    aptr->dims[0].lower_bound = 1;
    aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
    aptr->dims[0].stride = (1);
    aptr->dims[1].lower_bound = 1;
    aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)*((int32_t)amat->dims[4-1].length)) - (1))/(1) + 1);
    aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
    aptr->n_dims = 2;
    xptr->data = xvec->data;
    xptr->offset = xvec->offset;
    xptr->is_allocated = xvec->is_allocated;
    xptr->dims[0].lower_bound = 1;
    xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
    xptr->dims[0].stride = (1);
    xptr->n_dims = 1;
    yptr->data = yvec->data;
    yptr->offset = yvec->offset;
    yptr->is_allocated = yvec->is_allocated;
    yptr->dims[0].lower_bound = 1;
    yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
    yptr->dims[0].stride = (1);
    yptr->n_dims = 1;
    __lfortran_tblite_blas_level2__wrap_sgemv(aptr, xptr, yptr, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_sgemv_real_______0_real____1_real____2(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32___* xvec, int32_t __1xvec, struct r32___* yvec, int32_t __1yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    float a;
    float b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (float)(0);
    }
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    m = __1amat;
    n = __2amat;
    sgemv(tra, m, n, a, amat, lda, xvec, incx, b, yvec, incy);
}

void __lfortran_tblite_blas_level2__wrap_ssymv_real_______0_real____1_real____2(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32___* xvec, int32_t __1xvec, struct r32___* yvec, int32_t __1yvec, char * uplo, bool __libasr_is_present_uplo, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    float a;
    float b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t n;
    char * ula = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (float)(0);
    }
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    n = __2amat;
    ssymv(ula, n, a, amat, lda, xvec, incx, b, yvec, incy);
}

void __lfortran_tblite_blas_level2__wrap_sgemv312(struct r32_______* amat, struct r32___* xvec, struct r32_____* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r32___ yptr_value;
    struct r32___* yptr = &yptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_sgemv(aptr, xvec, yptr, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_sgemv321(struct r32_______* amat, struct r32_____* xvec, struct r32___* yvec, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r32___ xptr_value;
    struct r32___* xptr = &xptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_sgemv(aptr, xptr, yvec, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_dgemv312(struct r64_______* amat, struct r64___* xvec, struct r64_____* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r64___ yptr_value;
    struct r64___* yptr = &yptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        yptr->data = yvec->data;
        yptr->offset = yvec->offset;
        yptr->is_allocated = yvec->is_allocated;
        yptr->dims[0].lower_bound = 1;
        yptr->dims[0].length = (((((int32_t)yvec->dims[1-1].length)*((int32_t)yvec->dims[2-1].length)) - (1))/(1) + 1);
        yptr->dims[0].stride = (1);
        yptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_dgemv(aptr, xvec, yptr, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_dgemv321(struct r64_______* amat, struct r64_____* xvec, struct r64___* yvec, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta, char * trans, bool __libasr_is_present_trans)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    char * tra = NULL;
    struct r64___ xptr_value;
    struct r64___* xptr = &xptr_value;
    if (__libasr_is_present_trans) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, trans, strlen(trans));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
        xptr->data = xvec->data;
        xptr->offset = xvec->offset;
        xptr->is_allocated = xvec->is_allocated;
        xptr->dims[0].lower_bound = 1;
        xptr->dims[0].length = (((((int32_t)xvec->dims[1-1].length)*((int32_t)xvec->dims[2-1].length)) - (1))/(1) + 1);
        xptr->dims[0].stride = (1);
        xptr->n_dims = 1;
    }
    __lfortran_tblite_blas_level2__wrap_dgemv(aptr, xptr, yvec, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta, tra, true);
}

void __lfortran_tblite_blas_level2__wrap_ssymv(struct r32_____* amat, struct r32___* xvec, struct r32___* yvec, char * uplo, bool __libasr_is_present_uplo, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    float a;
    float b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t n;
    char * ula = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (float)(0);
    }
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    n = ((int32_t)amat->dims[2-1].length);
    ssymv(ula, n, a, amat, lda, xvec, incx, b, yvec, incy);
}

void __lfortran_tblite_blas_level2__wrap_dsymv(struct r64_____* amat, struct r64___* xvec, struct r64___* yvec, char * uplo, bool __libasr_is_present_uplo, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    double a;
    double b;
    int32_t incx;
    int32_t incy;
    int32_t lda;
    int32_t n;
    char * ula = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b = (double)(0);
    }
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    }
    incx = 1;
    incy = 1;
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    n = ((int32_t)amat->dims[2-1].length);
    dsymv(ula, n, a, amat, lda, xvec, incx, b, yvec, incy);
}

void __lfortran_tblite_blas_level3__wrap_dgemm(struct r64_____* amat, struct r64_____* bmat, struct r64_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    int32_t __lcompilers_i_0;
    int32_t __lcompilers_i_01;
    int32_t __lcompilers_i_02;
    int32_t __lcompilers_i_1;
    int32_t __lcompilers_i_11;
    int32_t __lcompilers_i_12;
    struct r64_____ __libasr_created__subroutine_call_dgemm_value;
    struct r64_____* __libasr_created__subroutine_call_dgemm = &__libasr_created__subroutine_call_dgemm_value;
    struct r64_____ __libasr_created__subroutine_call_dgemm1_value;
    struct r64_____* __libasr_created__subroutine_call_dgemm1 = &__libasr_created__subroutine_call_dgemm1_value;
    struct r64_____ __libasr_created__subroutine_call_dgemm2_value;
    struct r64_____* __libasr_created__subroutine_call_dgemm2 = &__libasr_created__subroutine_call_dgemm2_value;
    double a;
    double b;
    int32_t k;
    int32_t lda;
    int32_t ldb;
    int32_t ldc;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b =   0.00000000000000000e+00;
    }
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (str_compare(tra, 1, (char*)"n", 1)  ==  0 || str_compare(tra, 1, (char*)"N", 1)  ==  0) {
        k = ((int32_t)amat->dims[2-1].length);
    } else {
        k = ((int32_t)amat->dims[1-1].length);
    }
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    ldb = ((1) > (((int32_t)bmat->dims[1-1].length)) ? (1) : (((int32_t)bmat->dims[1-1].length)));
    ldc = ((1) > (((int32_t)cmat->dims[1-1].length)) ? (1) : (((int32_t)cmat->dims[1-1].length)));
    m = ((int32_t)cmat->dims[1-1].length);
    n = ((int32_t)cmat->dims[2-1].length);
    if (!(!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level3__wrap_dgemm____libasr_created__subroutine_call_dgemm, );
        __libasr_created__subroutine_call_dgemm->n_dims = 2;
        __libasr_created__subroutine_call_dgemm->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_dgemm->dims[1].length = ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemm->dims[1].stride = 1;
        __libasr_created__subroutine_call_dgemm->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_dgemm->dims[0].length = ((int32_t) amat->dims[1-1].length + amat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemm->dims[0].stride = (1 * ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_dgemm->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_dgemm->dims[1].length*__libasr_created__subroutine_call_dgemm->dims[0].length*sizeof(double));
        __libasr_created__subroutine_call_dgemm->is_allocated = true;
        for (__lcompilers_i_1=((int32_t)__libasr_created__subroutine_call_dgemm->dims[2-1].lower_bound); __lcompilers_i_1<=((int32_t) __libasr_created__subroutine_call_dgemm->dims[2-1].length + __libasr_created__subroutine_call_dgemm->dims[2-1].lower_bound - 1); __lcompilers_i_1++) {
            for (__lcompilers_i_0=((int32_t)__libasr_created__subroutine_call_dgemm->dims[1-1].lower_bound); __lcompilers_i_0<=((int32_t) __libasr_created__subroutine_call_dgemm->dims[1-1].length + __libasr_created__subroutine_call_dgemm->dims[1-1].lower_bound - 1); __lcompilers_i_0++) {
                __libasr_created__subroutine_call_dgemm->data[(((0 + (__libasr_created__subroutine_call_dgemm->dims[0].stride * (__lcompilers_i_0 - __libasr_created__subroutine_call_dgemm->dims[0].lower_bound))) + (__libasr_created__subroutine_call_dgemm->dims[1].stride * (__lcompilers_i_1 - __libasr_created__subroutine_call_dgemm->dims[1].lower_bound))) + __libasr_created__subroutine_call_dgemm->offset)] = amat->data[(((0 + (amat->dims[0].stride * (__lcompilers_i_0 - amat->dims[0].lower_bound))) + (amat->dims[1].stride * (__lcompilers_i_1 - amat->dims[1].lower_bound))) + amat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_dgemm = amat;
    }
    if (!(!bmat->is_allocated || (true && (bmat->dims[0].stride == 1) && (bmat->dims[1].stride == (1 * bmat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level3__wrap_dgemm____libasr_created__subroutine_call_dgemm1, );
        __libasr_created__subroutine_call_dgemm1->n_dims = 2;
        __libasr_created__subroutine_call_dgemm1->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_dgemm1->dims[1].length = ((int32_t) bmat->dims[2-1].length + bmat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemm1->dims[1].stride = 1;
        __libasr_created__subroutine_call_dgemm1->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_dgemm1->dims[0].length = ((int32_t) bmat->dims[1-1].length + bmat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemm1->dims[0].stride = (1 * ((int32_t) bmat->dims[2-1].length + bmat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_dgemm1->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_dgemm1->dims[1].length*__libasr_created__subroutine_call_dgemm1->dims[0].length*sizeof(double));
        __libasr_created__subroutine_call_dgemm1->is_allocated = true;
        for (__lcompilers_i_11=((int32_t)__libasr_created__subroutine_call_dgemm1->dims[2-1].lower_bound); __lcompilers_i_11<=((int32_t) __libasr_created__subroutine_call_dgemm1->dims[2-1].length + __libasr_created__subroutine_call_dgemm1->dims[2-1].lower_bound - 1); __lcompilers_i_11++) {
            for (__lcompilers_i_01=((int32_t)__libasr_created__subroutine_call_dgemm1->dims[1-1].lower_bound); __lcompilers_i_01<=((int32_t) __libasr_created__subroutine_call_dgemm1->dims[1-1].length + __libasr_created__subroutine_call_dgemm1->dims[1-1].lower_bound - 1); __lcompilers_i_01++) {
                __libasr_created__subroutine_call_dgemm1->data[(((0 + (__libasr_created__subroutine_call_dgemm1->dims[0].stride * (__lcompilers_i_01 - __libasr_created__subroutine_call_dgemm1->dims[0].lower_bound))) + (__libasr_created__subroutine_call_dgemm1->dims[1].stride * (__lcompilers_i_11 - __libasr_created__subroutine_call_dgemm1->dims[1].lower_bound))) + __libasr_created__subroutine_call_dgemm1->offset)] = bmat->data[(((0 + (bmat->dims[0].stride * (__lcompilers_i_01 - bmat->dims[0].lower_bound))) + (bmat->dims[1].stride * (__lcompilers_i_11 - bmat->dims[1].lower_bound))) + bmat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_dgemm1 = bmat;
    }
    if (!(!cmat->is_allocated || (true && (cmat->dims[0].stride == 1) && (cmat->dims[1].stride == (1 * cmat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level3__wrap_dgemm____libasr_created__subroutine_call_dgemm2, );
        __libasr_created__subroutine_call_dgemm2->n_dims = 2;
        __libasr_created__subroutine_call_dgemm2->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_dgemm2->dims[1].length = ((int32_t) cmat->dims[2-1].length + cmat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemm2->dims[1].stride = 1;
        __libasr_created__subroutine_call_dgemm2->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_dgemm2->dims[0].length = ((int32_t) cmat->dims[1-1].length + cmat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_dgemm2->dims[0].stride = (1 * ((int32_t) cmat->dims[2-1].length + cmat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_dgemm2->data = (double*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_dgemm2->dims[1].length*__libasr_created__subroutine_call_dgemm2->dims[0].length*sizeof(double));
        __libasr_created__subroutine_call_dgemm2->is_allocated = true;
        for (__lcompilers_i_12=((int32_t)__libasr_created__subroutine_call_dgemm2->dims[2-1].lower_bound); __lcompilers_i_12<=((int32_t) __libasr_created__subroutine_call_dgemm2->dims[2-1].length + __libasr_created__subroutine_call_dgemm2->dims[2-1].lower_bound - 1); __lcompilers_i_12++) {
            for (__lcompilers_i_02=((int32_t)__libasr_created__subroutine_call_dgemm2->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) __libasr_created__subroutine_call_dgemm2->dims[1-1].length + __libasr_created__subroutine_call_dgemm2->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
                __libasr_created__subroutine_call_dgemm2->data[(((0 + (__libasr_created__subroutine_call_dgemm2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_dgemm2->dims[0].lower_bound))) + (__libasr_created__subroutine_call_dgemm2->dims[1].stride * (__lcompilers_i_12 - __libasr_created__subroutine_call_dgemm2->dims[1].lower_bound))) + __libasr_created__subroutine_call_dgemm2->offset)] = cmat->data[(((0 + (cmat->dims[0].stride * (__lcompilers_i_02 - cmat->dims[0].lower_bound))) + (cmat->dims[1].stride * (__lcompilers_i_12 - cmat->dims[1].lower_bound))) + cmat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_dgemm2 = cmat;
    }
    dgemm(tra, trb, m, n, k, a, __libasr_created__subroutine_call_dgemm, lda, __libasr_created__subroutine_call_dgemm1, ldb, b, __libasr_created__subroutine_call_dgemm2, ldc);
    if ((!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        __libasr_created__subroutine_call_dgemm = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level3__wrap_dgemm____libasr_created__subroutine_call_dgemm, );
    }
    if ((!bmat->is_allocated || (true && (bmat->dims[0].stride == 1) && (bmat->dims[1].stride == (1 * bmat->dims[0].length))))) {
        __libasr_created__subroutine_call_dgemm1 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level3__wrap_dgemm____libasr_created__subroutine_call_dgemm1, );
    }
    if (!(!cmat->is_allocated || (true && (cmat->dims[0].stride == 1) && (cmat->dims[1].stride == (1 * cmat->dims[0].length))))) {
        for (__lcompilers_i_12=((int32_t)cmat->dims[2-1].lower_bound); __lcompilers_i_12<=((int32_t) cmat->dims[2-1].length + cmat->dims[2-1].lower_bound - 1); __lcompilers_i_12++) {
            for (__lcompilers_i_02=((int32_t)cmat->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) cmat->dims[1-1].length + cmat->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
                cmat->data[(((0 + (cmat->dims[0].stride * (__lcompilers_i_02 - cmat->dims[0].lower_bound))) + (cmat->dims[1].stride * (__lcompilers_i_12 - cmat->dims[1].lower_bound))) + cmat->offset)] = __libasr_created__subroutine_call_dgemm2->data[(((0 + (__libasr_created__subroutine_call_dgemm2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_dgemm2->dims[0].lower_bound))) + (__libasr_created__subroutine_call_dgemm2->dims[1].stride * (__lcompilers_i_12 - __libasr_created__subroutine_call_dgemm2->dims[1].lower_bound))) + __libasr_created__subroutine_call_dgemm2->offset)];
            }
        }
    }
    if ((!cmat->is_allocated || (true && (cmat->dims[0].stride == 1) && (cmat->dims[1].stride == (1 * cmat->dims[0].length))))) {
        __libasr_created__subroutine_call_dgemm2 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level3__wrap_dgemm____libasr_created__subroutine_call_dgemm2, );
    }
}

void __lfortran_tblite_blas_level3__wrap_dgemm233_real_______0(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_______* bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ bptr_value;
    struct r64_____* bptr = &bptr_value;
    struct r64_____ cptr_value;
    struct r64_____* cptr = &cptr_value;
    char * trb = NULL;
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[2-1].length)*((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_dgemm(amat, bptr, cptr, transa, __libasr_is_present_transa, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_dgemm323_real_______1(struct r64_______* amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    struct r64_____ cptr_value;
    struct r64_____* cptr = &cptr_value;
    char * tra = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_dgemm(aptr, bmat, cptr, tra, true, transb, __libasr_is_present_transb, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_dgemm332_real_______2(struct r64_______* amat, struct r64_______* bmat, struct r64_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct str___ __libasr_created__array_constant_1_value;
    struct str___* __libasr_created__array_constant_1 = &__libasr_created__array_constant_1_value;
    char* __libasr_created__array_constant_1_data[2];
    __libasr_created__array_constant_1->data = __libasr_created__array_constant_1_data;
    __libasr_created__array_constant_1->n_dims = 1;
    __libasr_created__array_constant_1->offset = 0;
    __libasr_created__array_constant_1->dims[0].lower_bound = 1;
    __libasr_created__array_constant_1->dims[0].length = 2;
    __libasr_created__array_constant_1->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant_1__init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i1 = 0; array_init_i1 < 2; array_init_i1++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant_1_data[array_init_i1], NULL, true, true, __libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)], strlen(__libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    struct l32___ __libasr_created__intrinsic_array_function_Any1_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any1 = &__libasr_created__intrinsic_array_function_Any1_value;
    bool __libasr_created__intrinsic_array_function_Any1_data[2];
    __libasr_created__intrinsic_array_function_Any1->data = __libasr_created__intrinsic_array_function_Any1_data;
    __libasr_created__intrinsic_array_function_Any1->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any1->offset = 0;
    __libasr_created__intrinsic_array_function_Any1->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any1->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any1->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any2;
    bool __libasr_created__intrinsic_array_function_Any3;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    int32_t __libasr_index_0_2;
    int32_t __libasr_index_0_3;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    struct r64_____ bptr_value;
    struct r64_____* bptr = &bptr_value;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any2 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any2) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    __libasr_index_0_3 = ((int32_t)__libasr_created__array_constant_1->dims[1-1].lower_bound);
    for (__libasr_index_0_2=((int32_t)__libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound); __libasr_index_0_2<=((int32_t) __libasr_created__intrinsic_array_function_Any1->dims[1-1].length + __libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound - 1); __libasr_index_0_2++) {
        __libasr_created__intrinsic_array_function_Any1->data[(0 + (1 * (__libasr_index_0_2 - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_1->data[(0 + (1 * (__libasr_index_0_3 - 1)))], 1)  ==  0;
        __libasr_index_0_3 = __libasr_index_0_3 + 1;
    }
    __libasr_created__intrinsic_array_function_Any3 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any1, 2);
    if (__libasr_created__intrinsic_array_function_Any3) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    __lfortran_tblite_blas_level3__wrap_dgemm(aptr, bptr, cmat, tra, true, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_dgemm_real_______0_real_______1_real_______2(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r64_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    double a;
    double b;
    int32_t k;
    int32_t lda;
    int32_t ldb;
    int32_t ldc;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b =   0.00000000000000000e+00;
    }
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (str_compare(tra, 1, (char*)"n", 1)  ==  0 || str_compare(tra, 1, (char*)"N", 1)  ==  0) {
        k = __2amat;
    } else {
        k = __1amat;
    }
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    ldb = ((1) > (__1bmat) ? (1) : (__1bmat));
    ldc = ((1) > (__1cmat) ? (1) : (__1cmat));
    m = __1cmat;
    n = __2cmat;
    dgemm(tra, trb, m, n, k, a, amat, lda, bmat, ldb, b, cmat, ldc);
}

void __lfortran_tblite_blas_level3__wrap_dtrsm_real_______0_real_______1(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, double alpha, bool __libasr_is_present_alpha)
{
    double a;
    char * dga = NULL;
    int32_t lda;
    int32_t ldb;
    int32_t m;
    int32_t n;
    char * sda = NULL;
    char * tra = NULL;
    char * ula = NULL;
    a =   1.00000000000000000e+00;
    if (__libasr_is_present_alpha) {
        a = alpha;
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_diag) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, diag, strlen(diag));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, (char*)"l", strlen((char*)"l"));
    if (__libasr_is_present_side) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, side, strlen(side));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    }
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    ldb = ((1) > (__1bmat) ? (1) : (__1bmat));
    m = __1bmat;
    n = __2bmat;
    dtrsm(sda, ula, tra, dga, m, n, a, amat, lda, bmat, ldb);
}

void __lfortran_tblite_blas_level3__wrap_sgemm(struct r32_____* amat, struct r32_____* bmat, struct r32_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    int32_t __lcompilers_i_0;
    int32_t __lcompilers_i_01;
    int32_t __lcompilers_i_02;
    int32_t __lcompilers_i_1;
    int32_t __lcompilers_i_11;
    int32_t __lcompilers_i_12;
    struct r32_____ __libasr_created__subroutine_call_sgemm_value;
    struct r32_____* __libasr_created__subroutine_call_sgemm = &__libasr_created__subroutine_call_sgemm_value;
    struct r32_____ __libasr_created__subroutine_call_sgemm1_value;
    struct r32_____* __libasr_created__subroutine_call_sgemm1 = &__libasr_created__subroutine_call_sgemm1_value;
    struct r32_____ __libasr_created__subroutine_call_sgemm2_value;
    struct r32_____* __libasr_created__subroutine_call_sgemm2 = &__libasr_created__subroutine_call_sgemm2_value;
    float a;
    float b;
    int32_t k;
    int32_t lda;
    int32_t ldb;
    int32_t ldc;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b =   0.00000000000000000e+00;
    }
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (str_compare(tra, 1, (char*)"n", 1)  ==  0 || str_compare(tra, 1, (char*)"N", 1)  ==  0) {
        k = ((int32_t)amat->dims[2-1].length);
    } else {
        k = ((int32_t)amat->dims[1-1].length);
    }
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    ldb = ((1) > (((int32_t)bmat->dims[1-1].length)) ? (1) : (((int32_t)bmat->dims[1-1].length)));
    ldc = ((1) > (((int32_t)cmat->dims[1-1].length)) ? (1) : (((int32_t)cmat->dims[1-1].length)));
    m = ((int32_t)cmat->dims[1-1].length);
    n = ((int32_t)cmat->dims[2-1].length);
    if (!(!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level3__wrap_sgemm____libasr_created__subroutine_call_sgemm, );
        __libasr_created__subroutine_call_sgemm->n_dims = 2;
        __libasr_created__subroutine_call_sgemm->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_sgemm->dims[1].length = ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemm->dims[1].stride = 1;
        __libasr_created__subroutine_call_sgemm->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_sgemm->dims[0].length = ((int32_t) amat->dims[1-1].length + amat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemm->dims[0].stride = (1 * ((int32_t) amat->dims[2-1].length + amat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_sgemm->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_sgemm->dims[1].length*__libasr_created__subroutine_call_sgemm->dims[0].length*sizeof(float));
        __libasr_created__subroutine_call_sgemm->is_allocated = true;
        for (__lcompilers_i_1=((int32_t)__libasr_created__subroutine_call_sgemm->dims[2-1].lower_bound); __lcompilers_i_1<=((int32_t) __libasr_created__subroutine_call_sgemm->dims[2-1].length + __libasr_created__subroutine_call_sgemm->dims[2-1].lower_bound - 1); __lcompilers_i_1++) {
            for (__lcompilers_i_0=((int32_t)__libasr_created__subroutine_call_sgemm->dims[1-1].lower_bound); __lcompilers_i_0<=((int32_t) __libasr_created__subroutine_call_sgemm->dims[1-1].length + __libasr_created__subroutine_call_sgemm->dims[1-1].lower_bound - 1); __lcompilers_i_0++) {
                __libasr_created__subroutine_call_sgemm->data[(((0 + (__libasr_created__subroutine_call_sgemm->dims[0].stride * (__lcompilers_i_0 - __libasr_created__subroutine_call_sgemm->dims[0].lower_bound))) + (__libasr_created__subroutine_call_sgemm->dims[1].stride * (__lcompilers_i_1 - __libasr_created__subroutine_call_sgemm->dims[1].lower_bound))) + __libasr_created__subroutine_call_sgemm->offset)] = amat->data[(((0 + (amat->dims[0].stride * (__lcompilers_i_0 - amat->dims[0].lower_bound))) + (amat->dims[1].stride * (__lcompilers_i_1 - amat->dims[1].lower_bound))) + amat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_sgemm = amat;
    }
    if (!(!bmat->is_allocated || (true && (bmat->dims[0].stride == 1) && (bmat->dims[1].stride == (1 * bmat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level3__wrap_sgemm____libasr_created__subroutine_call_sgemm1, );
        __libasr_created__subroutine_call_sgemm1->n_dims = 2;
        __libasr_created__subroutine_call_sgemm1->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_sgemm1->dims[1].length = ((int32_t) bmat->dims[2-1].length + bmat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemm1->dims[1].stride = 1;
        __libasr_created__subroutine_call_sgemm1->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_sgemm1->dims[0].length = ((int32_t) bmat->dims[1-1].length + bmat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemm1->dims[0].stride = (1 * ((int32_t) bmat->dims[2-1].length + bmat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_sgemm1->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_sgemm1->dims[1].length*__libasr_created__subroutine_call_sgemm1->dims[0].length*sizeof(float));
        __libasr_created__subroutine_call_sgemm1->is_allocated = true;
        for (__lcompilers_i_11=((int32_t)__libasr_created__subroutine_call_sgemm1->dims[2-1].lower_bound); __lcompilers_i_11<=((int32_t) __libasr_created__subroutine_call_sgemm1->dims[2-1].length + __libasr_created__subroutine_call_sgemm1->dims[2-1].lower_bound - 1); __lcompilers_i_11++) {
            for (__lcompilers_i_01=((int32_t)__libasr_created__subroutine_call_sgemm1->dims[1-1].lower_bound); __lcompilers_i_01<=((int32_t) __libasr_created__subroutine_call_sgemm1->dims[1-1].length + __libasr_created__subroutine_call_sgemm1->dims[1-1].lower_bound - 1); __lcompilers_i_01++) {
                __libasr_created__subroutine_call_sgemm1->data[(((0 + (__libasr_created__subroutine_call_sgemm1->dims[0].stride * (__lcompilers_i_01 - __libasr_created__subroutine_call_sgemm1->dims[0].lower_bound))) + (__libasr_created__subroutine_call_sgemm1->dims[1].stride * (__lcompilers_i_11 - __libasr_created__subroutine_call_sgemm1->dims[1].lower_bound))) + __libasr_created__subroutine_call_sgemm1->offset)] = bmat->data[(((0 + (bmat->dims[0].stride * (__lcompilers_i_01 - bmat->dims[0].lower_bound))) + (bmat->dims[1].stride * (__lcompilers_i_11 - bmat->dims[1].lower_bound))) + bmat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_sgemm1 = bmat;
    }
    if (!(!cmat->is_allocated || (true && (cmat->dims[0].stride == 1) && (cmat->dims[1].stride == (1 * cmat->dims[0].length))))) {
        // FIXME: deallocate(tblite_blas_level3__wrap_sgemm____libasr_created__subroutine_call_sgemm2, );
        __libasr_created__subroutine_call_sgemm2->n_dims = 2;
        __libasr_created__subroutine_call_sgemm2->dims[1].lower_bound = 1;
        __libasr_created__subroutine_call_sgemm2->dims[1].length = ((int32_t) cmat->dims[2-1].length + cmat->dims[2-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemm2->dims[1].stride = 1;
        __libasr_created__subroutine_call_sgemm2->dims[0].lower_bound = 1;
        __libasr_created__subroutine_call_sgemm2->dims[0].length = ((int32_t) cmat->dims[1-1].length + cmat->dims[1-1].lower_bound - 1);
        __libasr_created__subroutine_call_sgemm2->dims[0].stride = (1 * ((int32_t) cmat->dims[2-1].length + cmat->dims[2-1].lower_bound - 1));
        __libasr_created__subroutine_call_sgemm2->data = (float*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), 1*__libasr_created__subroutine_call_sgemm2->dims[1].length*__libasr_created__subroutine_call_sgemm2->dims[0].length*sizeof(float));
        __libasr_created__subroutine_call_sgemm2->is_allocated = true;
        for (__lcompilers_i_12=((int32_t)__libasr_created__subroutine_call_sgemm2->dims[2-1].lower_bound); __lcompilers_i_12<=((int32_t) __libasr_created__subroutine_call_sgemm2->dims[2-1].length + __libasr_created__subroutine_call_sgemm2->dims[2-1].lower_bound - 1); __lcompilers_i_12++) {
            for (__lcompilers_i_02=((int32_t)__libasr_created__subroutine_call_sgemm2->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) __libasr_created__subroutine_call_sgemm2->dims[1-1].length + __libasr_created__subroutine_call_sgemm2->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
                __libasr_created__subroutine_call_sgemm2->data[(((0 + (__libasr_created__subroutine_call_sgemm2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_sgemm2->dims[0].lower_bound))) + (__libasr_created__subroutine_call_sgemm2->dims[1].stride * (__lcompilers_i_12 - __libasr_created__subroutine_call_sgemm2->dims[1].lower_bound))) + __libasr_created__subroutine_call_sgemm2->offset)] = cmat->data[(((0 + (cmat->dims[0].stride * (__lcompilers_i_02 - cmat->dims[0].lower_bound))) + (cmat->dims[1].stride * (__lcompilers_i_12 - cmat->dims[1].lower_bound))) + cmat->offset)];
            }
        }
    } else {
        __libasr_created__subroutine_call_sgemm2 = cmat;
    }
    sgemm(tra, trb, m, n, k, a, __libasr_created__subroutine_call_sgemm, lda, __libasr_created__subroutine_call_sgemm1, ldb, b, __libasr_created__subroutine_call_sgemm2, ldc);
    if ((!amat->is_allocated || (true && (amat->dims[0].stride == 1) && (amat->dims[1].stride == (1 * amat->dims[0].length))))) {
        __libasr_created__subroutine_call_sgemm = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level3__wrap_sgemm____libasr_created__subroutine_call_sgemm, );
    }
    if ((!bmat->is_allocated || (true && (bmat->dims[0].stride == 1) && (bmat->dims[1].stride == (1 * bmat->dims[0].length))))) {
        __libasr_created__subroutine_call_sgemm1 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level3__wrap_sgemm____libasr_created__subroutine_call_sgemm1, );
    }
    if (!(!cmat->is_allocated || (true && (cmat->dims[0].stride == 1) && (cmat->dims[1].stride == (1 * cmat->dims[0].length))))) {
        for (__lcompilers_i_12=((int32_t)cmat->dims[2-1].lower_bound); __lcompilers_i_12<=((int32_t) cmat->dims[2-1].length + cmat->dims[2-1].lower_bound - 1); __lcompilers_i_12++) {
            for (__lcompilers_i_02=((int32_t)cmat->dims[1-1].lower_bound); __lcompilers_i_02<=((int32_t) cmat->dims[1-1].length + cmat->dims[1-1].lower_bound - 1); __lcompilers_i_02++) {
                cmat->data[(((0 + (cmat->dims[0].stride * (__lcompilers_i_02 - cmat->dims[0].lower_bound))) + (cmat->dims[1].stride * (__lcompilers_i_12 - cmat->dims[1].lower_bound))) + cmat->offset)] = __libasr_created__subroutine_call_sgemm2->data[(((0 + (__libasr_created__subroutine_call_sgemm2->dims[0].stride * (__lcompilers_i_02 - __libasr_created__subroutine_call_sgemm2->dims[0].lower_bound))) + (__libasr_created__subroutine_call_sgemm2->dims[1].stride * (__lcompilers_i_12 - __libasr_created__subroutine_call_sgemm2->dims[1].lower_bound))) + __libasr_created__subroutine_call_sgemm2->offset)];
            }
        }
    }
    if ((!cmat->is_allocated || (true && (cmat->dims[0].stride == 1) && (cmat->dims[1].stride == (1 * cmat->dims[0].length))))) {
        __libasr_created__subroutine_call_sgemm2 = NULL;
    } else {
        // FIXME: deallocate(tblite_blas_level3__wrap_sgemm____libasr_created__subroutine_call_sgemm2, );
    }
}

void __lfortran_tblite_blas_level3__wrap_sgemm233_real_______0(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32_______* bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ bptr_value;
    struct r32_____* bptr = &bptr_value;
    struct r32_____ cptr_value;
    struct r32_____* cptr = &cptr_value;
    char * trb = NULL;
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[2-1].length)*((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_sgemm(amat, bptr, cptr, transa, __libasr_is_present_transa, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_sgemm323_real_______1(struct r32_______* amat, struct r32_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    struct r32_____ cptr_value;
    struct r32_____* cptr = &cptr_value;
    char * tra = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_sgemm(aptr, bmat, cptr, tra, true, transb, __libasr_is_present_transb, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_sgemm332_real_______2(struct r32_______* amat, struct r32_______* bmat, struct r32_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct str___ __libasr_created__array_constant_1_value;
    struct str___* __libasr_created__array_constant_1 = &__libasr_created__array_constant_1_value;
    char* __libasr_created__array_constant_1_data[2];
    __libasr_created__array_constant_1->data = __libasr_created__array_constant_1_data;
    __libasr_created__array_constant_1->n_dims = 1;
    __libasr_created__array_constant_1->offset = 0;
    __libasr_created__array_constant_1->dims[0].lower_bound = 1;
    __libasr_created__array_constant_1->dims[0].length = 2;
    __libasr_created__array_constant_1->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant_1__init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i1 = 0; array_init_i1 < 2; array_init_i1++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant_1_data[array_init_i1], NULL, true, true, __libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)], strlen(__libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    struct l32___ __libasr_created__intrinsic_array_function_Any1_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any1 = &__libasr_created__intrinsic_array_function_Any1_value;
    bool __libasr_created__intrinsic_array_function_Any1_data[2];
    __libasr_created__intrinsic_array_function_Any1->data = __libasr_created__intrinsic_array_function_Any1_data;
    __libasr_created__intrinsic_array_function_Any1->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any1->offset = 0;
    __libasr_created__intrinsic_array_function_Any1->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any1->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any1->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any2;
    bool __libasr_created__intrinsic_array_function_Any3;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    int32_t __libasr_index_0_2;
    int32_t __libasr_index_0_3;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    struct r32_____ bptr_value;
    struct r32_____* bptr = &bptr_value;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any2 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any2) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    __libasr_index_0_3 = ((int32_t)__libasr_created__array_constant_1->dims[1-1].lower_bound);
    for (__libasr_index_0_2=((int32_t)__libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound); __libasr_index_0_2<=((int32_t) __libasr_created__intrinsic_array_function_Any1->dims[1-1].length + __libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound - 1); __libasr_index_0_2++) {
        __libasr_created__intrinsic_array_function_Any1->data[(0 + (1 * (__libasr_index_0_2 - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_1->data[(0 + (1 * (__libasr_index_0_3 - 1)))], 1)  ==  0;
        __libasr_index_0_3 = __libasr_index_0_3 + 1;
    }
    __libasr_created__intrinsic_array_function_Any3 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any1, 2);
    if (__libasr_created__intrinsic_array_function_Any3) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    __lfortran_tblite_blas_level3__wrap_sgemm(aptr, bptr, cmat, tra, true, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_sgemm_real_______0_real_______1_real_______2(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r32_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    float a;
    float b;
    int32_t k;
    int32_t lda;
    int32_t ldb;
    int32_t ldc;
    int32_t m;
    int32_t n;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_alpha) {
        a = alpha;
    } else {
        a =   1.00000000000000000e+00;
    }
    if (__libasr_is_present_beta) {
        b = beta;
    } else {
        b =   0.00000000000000000e+00;
    }
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (str_compare(tra, 1, (char*)"n", 1)  ==  0 || str_compare(tra, 1, (char*)"N", 1)  ==  0) {
        k = __2amat;
    } else {
        k = __1amat;
    }
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    ldb = ((1) > (__1bmat) ? (1) : (__1bmat));
    ldc = ((1) > (__1cmat) ? (1) : (__1cmat));
    m = __1cmat;
    n = __2cmat;
    sgemm(tra, trb, m, n, k, a, amat, lda, bmat, ldb, b, cmat, ldc);
}

void __lfortran_tblite_blas_level3__wrap_strsm_real_______0_real_______1(struct r32_____* amat, int32_t __1amat, int32_t __2amat, struct r32_____* bmat, int32_t __1bmat, int32_t __2bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, float alpha, bool __libasr_is_present_alpha)
{
    float a;
    char * dga = NULL;
    int32_t lda;
    int32_t ldb;
    int32_t m;
    int32_t n;
    char * sda = NULL;
    char * tra = NULL;
    char * ula = NULL;
    a =   1.00000000000000000e+00;
    if (__libasr_is_present_alpha) {
        a = alpha;
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_diag) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, diag, strlen(diag));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, (char*)"l", strlen((char*)"l"));
    if (__libasr_is_present_side) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, side, strlen(side));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    }
    lda = ((1) > (__1amat) ? (1) : (__1amat));
    ldb = ((1) > (__1bmat) ? (1) : (__1bmat));
    m = __1bmat;
    n = __2bmat;
    strsm(sda, ula, tra, dga, m, n, a, amat, lda, bmat, ldb);
}

void __lfortran_tblite_blas_level3__wrap_sgemm323(struct r32_______* amat, struct r32_____* bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    struct r32_____ cptr_value;
    struct r32_____* cptr = &cptr_value;
    char * tra = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_sgemm(aptr, bmat, cptr, tra, true, transb, __libasr_is_present_transb, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_sgemm233(struct r32_____* amat, struct r32_______* bmat, struct r32_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r32_____ bptr_value;
    struct r32_____* bptr = &bptr_value;
    struct r32_____ cptr_value;
    struct r32_____* cptr = &cptr_value;
    char * trb = NULL;
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[2-1].length)*((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_sgemm(amat, bptr, cptr, transa, __libasr_is_present_transa, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_sgemm332(struct r32_______* amat, struct r32_______* bmat, struct r32_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, float alpha, bool __libasr_is_present_alpha, float beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct str___ __libasr_created__array_constant_1_value;
    struct str___* __libasr_created__array_constant_1 = &__libasr_created__array_constant_1_value;
    char* __libasr_created__array_constant_1_data[2];
    __libasr_created__array_constant_1->data = __libasr_created__array_constant_1_data;
    __libasr_created__array_constant_1->n_dims = 1;
    __libasr_created__array_constant_1->offset = 0;
    __libasr_created__array_constant_1->dims[0].lower_bound = 1;
    __libasr_created__array_constant_1->dims[0].length = 2;
    __libasr_created__array_constant_1->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant_1__init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i1 = 0; array_init_i1 < 2; array_init_i1++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant_1_data[array_init_i1], NULL, true, true, __libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)], strlen(__libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    struct l32___ __libasr_created__intrinsic_array_function_Any1_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any1 = &__libasr_created__intrinsic_array_function_Any1_value;
    bool __libasr_created__intrinsic_array_function_Any1_data[2];
    __libasr_created__intrinsic_array_function_Any1->data = __libasr_created__intrinsic_array_function_Any1_data;
    __libasr_created__intrinsic_array_function_Any1->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any1->offset = 0;
    __libasr_created__intrinsic_array_function_Any1->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any1->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any1->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any2;
    bool __libasr_created__intrinsic_array_function_Any3;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    int32_t __libasr_index_0_2;
    int32_t __libasr_index_0_3;
    struct r32_____ aptr_value;
    struct r32_____* aptr = &aptr_value;
    struct r32_____ bptr_value;
    struct r32_____* bptr = &bptr_value;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any2 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any2) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    __libasr_index_0_3 = ((int32_t)__libasr_created__array_constant_1->dims[1-1].lower_bound);
    for (__libasr_index_0_2=((int32_t)__libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound); __libasr_index_0_2<=((int32_t) __libasr_created__intrinsic_array_function_Any1->dims[1-1].length + __libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound - 1); __libasr_index_0_2++) {
        __libasr_created__intrinsic_array_function_Any1->data[(0 + (1 * (__libasr_index_0_2 - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_1->data[(0 + (1 * (__libasr_index_0_3 - 1)))], 1)  ==  0;
        __libasr_index_0_3 = __libasr_index_0_3 + 1;
    }
    __libasr_created__intrinsic_array_function_Any3 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any1, 2);
    if (__libasr_created__intrinsic_array_function_Any3) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    __lfortran_tblite_blas_level3__wrap_sgemm(aptr, bptr, cmat, tra, true, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_dgemm323(struct r64_______* amat, struct r64_____* bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    struct r64_____ cptr_value;
    struct r64_____* cptr = &cptr_value;
    char * tra = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)*((int32_t)cmat->dims[2-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_dgemm(aptr, bmat, cptr, tra, true, transb, __libasr_is_present_transb, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_dgemm233(struct r64_____* amat, struct r64_______* bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any1;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    struct r64_____ bptr_value;
    struct r64_____* bptr = &bptr_value;
    struct r64_____ cptr_value;
    struct r64_____* cptr = &cptr_value;
    char * trb = NULL;
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any1 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any1) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    cptr->data = cmat->data;
    cptr->offset = cmat->offset;
    cptr->is_allocated = cmat->is_allocated;
    cptr->dims[0].lower_bound = 1;
    cptr->dims[0].length = (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1);
    cptr->dims[0].stride = (1);
    cptr->dims[1].lower_bound = 1;
    cptr->dims[1].length = (((((int32_t)cmat->dims[2-1].length)*((int32_t)cmat->dims[3-1].length)) - (1))/(1) + 1);
    cptr->dims[1].stride = ((1 * (((((int32_t)cmat->dims[1-1].length)) - (1))/(1) + 1)));
    cptr->n_dims = 2;
    __lfortran_tblite_blas_level3__wrap_dgemm(amat, bptr, cptr, transa, __libasr_is_present_transa, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_dgemm332(struct r64_______* amat, struct r64_______* bmat, struct r64_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta)
{
    struct str___ __libasr_created__array_constant__value;
    struct str___* __libasr_created__array_constant_ = &__libasr_created__array_constant__value;
    char* __libasr_created__array_constant__data[2];
    __libasr_created__array_constant_->data = __libasr_created__array_constant__data;
    __libasr_created__array_constant_->n_dims = 1;
    __libasr_created__array_constant_->offset = 0;
    __libasr_created__array_constant_->dims[0].lower_bound = 1;
    __libasr_created__array_constant_->dims[0].length = 2;
    __libasr_created__array_constant_->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant___init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i = 0; array_init_i < 2; array_init_i++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant__data[array_init_i], NULL, true, true, __libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)], strlen(__libasr_created__array_constant___init->data[__libasr_created__array_constant___init->offset + (array_init_i * __libasr_created__array_constant___init->dims[0].stride)]));
    }
;
    struct str___ __libasr_created__array_constant_1_value;
    struct str___* __libasr_created__array_constant_1 = &__libasr_created__array_constant_1_value;
    char* __libasr_created__array_constant_1_data[2];
    __libasr_created__array_constant_1->data = __libasr_created__array_constant_1_data;
    __libasr_created__array_constant_1->n_dims = 1;
    __libasr_created__array_constant_1->offset = 0;
    __libasr_created__array_constant_1->dims[0].lower_bound = 1;
    __libasr_created__array_constant_1->dims[0].length = 2;
    __libasr_created__array_constant_1->dims[0].stride = 1;
    struct str___* __libasr_created__array_constant_1__init = array_constant_str___(2, "n", "N");
    for (int32_t array_init_i1 = 0; array_init_i1 < 2; array_init_i1++) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &__libasr_created__array_constant_1_data[array_init_i1], NULL, true, true, __libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)], strlen(__libasr_created__array_constant_1__init->data[__libasr_created__array_constant_1__init->offset + (array_init_i1 * __libasr_created__array_constant_1__init->dims[0].stride)]));
    }
;
    struct l32___ __libasr_created__intrinsic_array_function_Any_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any = &__libasr_created__intrinsic_array_function_Any_value;
    bool __libasr_created__intrinsic_array_function_Any_data[2];
    __libasr_created__intrinsic_array_function_Any->data = __libasr_created__intrinsic_array_function_Any_data;
    __libasr_created__intrinsic_array_function_Any->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any->offset = 0;
    __libasr_created__intrinsic_array_function_Any->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any->dims[0].stride = 1;
    struct l32___ __libasr_created__intrinsic_array_function_Any1_value;
    struct l32___* __libasr_created__intrinsic_array_function_Any1 = &__libasr_created__intrinsic_array_function_Any1_value;
    bool __libasr_created__intrinsic_array_function_Any1_data[2];
    __libasr_created__intrinsic_array_function_Any1->data = __libasr_created__intrinsic_array_function_Any1_data;
    __libasr_created__intrinsic_array_function_Any1->n_dims = 1;
    __libasr_created__intrinsic_array_function_Any1->offset = 0;
    __libasr_created__intrinsic_array_function_Any1->dims[0].lower_bound = 1;
    __libasr_created__intrinsic_array_function_Any1->dims[0].length = 2;
    __libasr_created__intrinsic_array_function_Any1->dims[0].stride = 1;
    bool __libasr_created__intrinsic_array_function_Any2;
    bool __libasr_created__intrinsic_array_function_Any3;
    int32_t __libasr_index_0_;
    int32_t __libasr_index_0_1;
    int32_t __libasr_index_0_2;
    int32_t __libasr_index_0_3;
    struct r64_____ aptr_value;
    struct r64_____* aptr = &aptr_value;
    struct r64_____ bptr_value;
    struct r64_____* bptr = &bptr_value;
    char * tra = NULL;
    char * trb = NULL;
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    if (__libasr_is_present_transb) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, transb, strlen(transb));
    } else {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &trb, NULL, true, true, (char*)"n", strlen((char*)"n"));
    }
    __libasr_index_0_1 = ((int32_t)__libasr_created__array_constant_->dims[1-1].lower_bound);
    for (__libasr_index_0_=((int32_t)__libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound); __libasr_index_0_<=((int32_t) __libasr_created__intrinsic_array_function_Any->dims[1-1].length + __libasr_created__intrinsic_array_function_Any->dims[1-1].lower_bound - 1); __libasr_index_0_++) {
        __libasr_created__intrinsic_array_function_Any->data[(0 + (1 * (__libasr_index_0_ - 1)))] = str_compare(tra, 1, __libasr_created__array_constant_->data[(0 + (1 * (__libasr_index_0_1 - 1)))], 1)  ==  0;
        __libasr_index_0_1 = __libasr_index_0_1 + 1;
    }
    __libasr_created__intrinsic_array_function_Any2 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any, 2);
    if (__libasr_created__intrinsic_array_function_Any2) {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[2-1].length)*((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    } else {
        aptr->data = amat->data;
        aptr->offset = amat->offset;
        aptr->is_allocated = amat->is_allocated;
        aptr->dims[0].lower_bound = 1;
        aptr->dims[0].length = (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1);
        aptr->dims[0].stride = (1);
        aptr->dims[1].lower_bound = 1;
        aptr->dims[1].length = (((((int32_t)amat->dims[3-1].length)) - (1))/(1) + 1);
        aptr->dims[1].stride = ((1 * (((((int32_t)amat->dims[1-1].length)*((int32_t)amat->dims[2-1].length)) - (1))/(1) + 1)));
        aptr->n_dims = 2;
    }
    __libasr_index_0_3 = ((int32_t)__libasr_created__array_constant_1->dims[1-1].lower_bound);
    for (__libasr_index_0_2=((int32_t)__libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound); __libasr_index_0_2<=((int32_t) __libasr_created__intrinsic_array_function_Any1->dims[1-1].length + __libasr_created__intrinsic_array_function_Any1->dims[1-1].lower_bound - 1); __libasr_index_0_2++) {
        __libasr_created__intrinsic_array_function_Any1->data[(0 + (1 * (__libasr_index_0_2 - 1)))] = str_compare(trb, 1, __libasr_created__array_constant_1->data[(0 + (1 * (__libasr_index_0_3 - 1)))], 1)  ==  0;
        __libasr_index_0_3 = __libasr_index_0_3 + 1;
    }
    __libasr_created__intrinsic_array_function_Any3 = __lfortran__lcompilers_Any_4_1_0_logical____0(__libasr_created__intrinsic_array_function_Any1, 2);
    if (__libasr_created__intrinsic_array_function_Any3) {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)*((int32_t)bmat->dims[2-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    } else {
        bptr->data = bmat->data;
        bptr->offset = bmat->offset;
        bptr->is_allocated = bmat->is_allocated;
        bptr->dims[0].lower_bound = 1;
        bptr->dims[0].length = (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1);
        bptr->dims[0].stride = (1);
        bptr->dims[1].lower_bound = 1;
        bptr->dims[1].length = (((((int32_t)bmat->dims[2-1].length)*((int32_t)bmat->dims[3-1].length)) - (1))/(1) + 1);
        bptr->dims[1].stride = ((1 * (((((int32_t)bmat->dims[1-1].length)) - (1))/(1) + 1)));
        bptr->n_dims = 2;
    }
    __lfortran_tblite_blas_level3__wrap_dgemm(aptr, bptr, cmat, tra, true, trb, true, alpha, __libasr_is_present_alpha, beta, __libasr_is_present_beta);
}

void __lfortran_tblite_blas_level3__wrap_strsm(struct r32_____* amat, struct r32_____* bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, float alpha, bool __libasr_is_present_alpha)
{
    float a;
    char * dga = NULL;
    int32_t lda;
    int32_t ldb;
    int32_t m;
    int32_t n;
    char * sda = NULL;
    char * tra = NULL;
    char * ula = NULL;
    a =   1.00000000000000000e+00;
    if (__libasr_is_present_alpha) {
        a = alpha;
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_diag) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, diag, strlen(diag));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, (char*)"l", strlen((char*)"l"));
    if (__libasr_is_present_side) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, side, strlen(side));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    }
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    ldb = ((1) > (((int32_t)bmat->dims[1-1].length)) ? (1) : (((int32_t)bmat->dims[1-1].length)));
    m = ((int32_t)bmat->dims[1-1].length);
    n = ((int32_t)bmat->dims[2-1].length);
    strsm(sda, ula, tra, dga, m, n, a, amat, lda, bmat, ldb);
}

void __lfortran_tblite_blas_level3__wrap_dtrsm(struct r64_____* amat, struct r64_____* bmat, char * side, bool __libasr_is_present_side, char * uplo, bool __libasr_is_present_uplo, char * transa, bool __libasr_is_present_transa, char * diag, bool __libasr_is_present_diag, double alpha, bool __libasr_is_present_alpha)
{
    double a;
    char * dga = NULL;
    int32_t lda;
    int32_t ldb;
    int32_t m;
    int32_t n;
    char * sda = NULL;
    char * tra = NULL;
    char * ula = NULL;
    a =   1.00000000000000000e+00;
    if (__libasr_is_present_alpha) {
        a = alpha;
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_diag) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &dga, NULL, true, true, diag, strlen(diag));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, (char*)"l", strlen((char*)"l"));
    if (__libasr_is_present_side) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &sda, NULL, true, true, side, strlen(side));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, (char*)"n", strlen((char*)"n"));
    if (__libasr_is_present_transa) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &tra, NULL, true, true, transa, strlen(transa));
    }
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, (char*)"u", strlen((char*)"u"));
    if (__libasr_is_present_uplo) {
        _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &ula, NULL, true, true, uplo, strlen(uplo));
    }
    lda = ((1) > (((int32_t)amat->dims[1-1].length)) ? (1) : (((int32_t)amat->dims[1-1].length)));
    ldb = ((1) > (((int32_t)bmat->dims[1-1].length)) ? (1) : (((int32_t)bmat->dims[1-1].length)));
    m = ((int32_t)bmat->dims[1-1].length);
    n = ((int32_t)bmat->dims[2-1].length);
    dtrsm(sda, ula, tra, dga, m, n, a, amat, lda, bmat, ldb);
}

void __lfortran_tblite_wavefunction_type__get_alpha_beta_occupation(double nocc, double nuhf, double *nalp, double *nbet)
{
    double diff;
    double ntmp;
    diff = fmin(nuhf, nocc);
    ntmp = nocc - diff;
    (*nalp) = ntmp/(double)(2) + diff;
    (*nbet) = ntmp/(double)(2);
}

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


int32_t array_size(struct dimension_descriptor dims[], size_t n) {
    int32_t size = 1;
    for (size_t i = 0; i < n; i++) {
        size *= dims[i].length;
    }
    return size;
}

struct str___* array_constant_str___(int32_t n, ...) {
    struct str___* const_array  = (struct str___*) malloc(sizeof(struct str___));
    va_list ap;
    va_start(ap, n);
    const_array->data = (char**) malloc(sizeof(char*)*n);
    const_array->n_dims = 1;
    const_array->dims[0].lower_bound = 0;
    const_array->dims[0].length = n;
    for (int32_t i = 0; i < n; i++) {
        const_array->data[i] = va_arg(ap, char*);
    }
    va_end(ap);
    return const_array;
}


