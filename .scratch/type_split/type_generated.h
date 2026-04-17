#ifndef TYPE_GENERATED_H
#define TYPE_GENERATED_H

#include <inttypes.h>
#include <math.h>

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


struct r64___
{
    double *data;
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


struct Allocatable_r64______
{
    double *data;
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


struct r64_______
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


struct r32_____
{
    float *data;
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

bool __lfortran__lcompilers_Any_4_1_0_logical____0(struct l32___* mask, int32_t __1mask);
void __lfortran__lcompilers_get_command_argument_(int32_t number, int32_t *length, int32_t *status);
void __lfortran__lcompilers_get_command_argument_1(int32_t number, char * *value, int32_t *status);
void __lfortran__lcompilers_get_environment_variable_(char * name, int32_t *length, int32_t *status);
void __lfortran__lcompilers_get_environment_variable_1(char * name, char * *value, int32_t *status);
int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_(char * str, char * substr, bool back, int32_t kind);
int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_1(char * str, char * substr, bool back, int32_t kind);
int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_2(char * str, char * substr, bool back, int32_t kind);
void __lfortran_tblite_wavefunction_type__get_alpha_beta_occupation(double nocc, double nuhf, double *nalp, double *nbet);
void __lfortran_tblite_wavefunction_type__get_density_matrix_real____0_real_______1_real_______2(struct r64___* focc, int32_t __1focc, struct r64_____* coeff, int32_t __1coeff, int32_t __2coeff, struct r64_____* pmat, int32_t __1pmat, int32_t __2pmat);
void __lfortran_tblite_blas_level3__wrap_dgemm_real_______0_real_______1_real_______2(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r64_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_wavefunction_type__new_wavefunction__StructType(struct tblite_wavefunction_type__wavefunction_type* self, int32_t nat, int32_t nsh, int32_t nao, int32_t nspin, double kt, bool grad, bool __libasr_is_present_grad);
void __lfortran_mctc_env_error__fatal_error__Allocatable_StructType_(struct mctc_env_error__error_type* *error, char * message, bool __libasr_is_present_message, int32_t stat, bool __libasr_is_present_stat);
void __lfortran_mctc_env_system__get_argument(int32_t idx, char * *arg);
void __lfortran_mctc_env_system__get_variable(char * var, char * *val);
bool __lfortran_mctc_env_system__is_windows();
bool __lfortran_mctc_env_system__is_unix();
double ddot(int32_t n, struct r64___* x, int32_t incx, struct r64___* y, int32_t incy);
float sdot(int32_t n, struct r32___* x, int32_t incx, struct r32___* y, int32_t incy);
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
void dgemv(char * trans, int32_t m, int32_t n, double alpha, struct r64_____* a, int32_t lda, struct r64___* x, int32_t incx, double beta, struct r64___* y, int32_t incy);
void dsymv(char * uplo, int32_t n, double alpha, struct r64_____* a, int32_t lda, struct r64___* x, int32_t incx, double beta, struct r64___* y, int32_t incy);
void sgemv(char * trans, int32_t m, int32_t n, float alpha, struct r32_____* a, int32_t lda, struct r32___* x, int32_t incx, float beta, struct r32___* y, int32_t incy);
void ssymv(char * uplo, int32_t n, float alpha, struct r32_____* a, int32_t lda, struct r32___* x, int32_t incx, float beta, struct r32___* y, int32_t incy);
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
void dgemm(char * transa, char * transb, int32_t m, int32_t n, int32_t k, double alpha, struct r64_____* a, int32_t lda, struct r64_____* b, int32_t ldb, double beta, struct r64_____* c, int32_t ldc);
void dtrsm(char * side, char * uplo, char * transa, char * diag, int32_t m, int32_t n, double alpha, struct r64_____* a, int32_t lda, struct r64_____* b, int32_t ldb);
void sgemm(char * transa, char * transb, int32_t m, int32_t n, int32_t k, float alpha, struct r32_____* a, int32_t lda, struct r32_____* b, int32_t ldb, float beta, struct r32_____* c, int32_t ldc);
void strsm(char * side, char * uplo, char * transa, char * diag, int32_t m, int32_t n, float alpha, struct r32_____* a, int32_t lda, struct r32_____* b, int32_t ldb);
void __lfortran_tblite_blas_level3__wrap_dgemm(struct r64_____* amat, struct r64_____* bmat, struct r64_____* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm233_real_______0(struct r64_____* amat, int32_t __1amat, int32_t __2amat, struct r64_______* bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm323_real_______1(struct r64_______* amat, struct r64_____* bmat, int32_t __1bmat, int32_t __2bmat, struct r64_______* cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
void __lfortran_tblite_blas_level3__wrap_dgemm332_real_______2(struct r64_______* amat, struct r64_______* bmat, struct r64_____* cmat, int32_t __1cmat, int32_t __2cmat, char * transa, bool __libasr_is_present_transa, char * transb, bool __libasr_is_present_transb, double alpha, bool __libasr_is_present_alpha, double beta, bool __libasr_is_present_beta);
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

extern const int32_t mctc_env_accuracy__dp;
extern const int32_t mctc_env_accuracy__i1;
extern const int32_t mctc_env_accuracy__i2;
extern const int32_t mctc_env_accuracy__i4;
extern const int32_t mctc_env_accuracy__i8;
extern const int32_t mctc_env_accuracy__sp;
extern const int32_t mctc_env_accuracy__wp;
extern const struct mctc_env_error__enum_stat mctc_env_error__mctc_stat;

#endif
