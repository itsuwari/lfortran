#include <tests/doctest.h>
#include <cstring>
#include <iostream>

#include <libasr/bwriter.h>
#include <libasr/serialization.h>
#include <lfortran/ast_serialization.h>
#include <libasr/modfile.h>
#include <lfortran/pickle.h>
#include <libasr/pickle.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/utils.h>

using LCompilers::TRY;
using LCompilers::string_to_uint64;
using LCompilers::uint64_to_string;
using LCompilers::string_to_uint32;
using LCompilers::uint32_to_string;

TEST_CASE("Integer conversion") {
    uint64_t i;
    i = 1;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 150;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 256;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 65537;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 16777217;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 4294967295LU;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 4294967296LU;
    CHECK(string_to_uint32(uint32_to_string(i)) != i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 18446744073709551615LLU;
    CHECK(string_to_uint32(uint32_to_string(i)) != i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);
}

void ast_ser(const std::string &src) {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* result;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions co;
    co.interactive = true;
    result = TRY(LCompilers::LFortran::parse(al, src, diagnostics, co));
    std::string ast_orig = LCompilers::LFortran::pickle(*result);
    std::string binary = LCompilers::LFortran::serialize(*result);

    LCompilers::LFortran::AST::ast_t *ast;
    ast = LCompilers::LFortran::deserialize_ast(al, binary);
    CHECK(LCompilers::LFortran::AST::is_a<LCompilers::LFortran::AST::unit_t>(*ast));

    std::string ast_new = LCompilers::LFortran::pickle(*ast);

    CHECK(ast_orig == ast_new);
}

void asr_ser(const std::string &src) {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    ast0 = TRY(LCompilers::LFortran::parse(al, src, diagnostics, compiler_options));
    LCompilers::LocationManager lm;
    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));

    std::string asr_orig = LCompilers::pickle(*asr);
    std::string binary = LCompilers::serialize(*asr);

    LCompilers::ASR::asr_t *asr_new0;
    LCompilers::SymbolTable symtab(nullptr);
    asr_new0 = LCompilers::deserialize_asr(al, binary, true, symtab, 0);
    CHECK(LCompilers::ASR::is_a<LCompilers::ASR::unit_t>(*asr_new0));
    LCompilers::ASR::TranslationUnit_t *tu
        = LCompilers::ASR::down_cast2<LCompilers::ASR::TranslationUnit_t>(asr_new0);
    fix_external_symbols(*tu, symtab);
    LCOMPILERS_ASSERT(LCompilers::asr_verify(*tu, true, diagnostics));

    std::string asr_new = LCompilers::pickle(*asr_new0);

    CHECK(asr_orig == asr_new);
}

void check_module_function_bodies(LCompilers::ASR::TranslationUnit_t *asr,
        bool expect_empty_body) {
    for (auto &module_item : asr->m_symtab->get_scope()) {
        if (!LCompilers::ASR::is_a<LCompilers::ASR::Module_t>(*module_item.second)) {
            continue;
        }
        LCompilers::ASR::Module_t *module
            = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(module_item.second);
        for (auto &function_item : module->m_symtab->get_scope()) {
            if (!LCompilers::ASR::is_a<LCompilers::ASR::Function_t>(*function_item.second)) {
                continue;
            }
            LCompilers::ASR::Function_t *function
                = LCompilers::ASR::down_cast<LCompilers::ASR::Function_t>(function_item.second);
            if (expect_empty_body) {
                CHECK(function->n_body == 0);
            } else {
                CHECK(function->n_body > 0);
            }
        }
    }
}

LCompilers::ASR::Function_t *get_module_function(
        LCompilers::ASR::TranslationUnit_t *asr,
        const std::string &module_name, const std::string &function_name) {
    LCompilers::ASR::symbol_t *module_sym = asr->m_symtab->get_symbol(module_name);
    REQUIRE(module_sym != nullptr);
    REQUIRE(LCompilers::ASR::is_a<LCompilers::ASR::Module_t>(*module_sym));
    LCompilers::ASR::Module_t *module
        = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(module_sym);
    LCompilers::ASR::symbol_t *function_sym = module->m_symtab->get_symbol(function_name);
    REQUIRE(function_sym != nullptr);
    REQUIRE(LCompilers::ASR::is_a<LCompilers::ASR::Function_t>(*function_sym));
    return LCompilers::ASR::down_cast<LCompilers::ASR::Function_t>(function_sym);
}

bool function_has_dependency(LCompilers::ASR::Function_t *function,
        const std::string &dependency) {
    for (size_t i = 0; i < function->n_dependencies; i++) {
        if (dependency == function->m_dependencies[i]) {
            return true;
        }
    }
    return false;
}

bool module_has_dependency(LCompilers::ASR::Module_t *module,
        const std::string &dependency) {
    for (size_t i = 0; i < module->n_dependencies; i++) {
        if (dependency == module->m_dependencies[i]) {
            return true;
        }
    }
    return false;
}

LCompilers::LocationManager make_test_location_manager() {
    LCompilers::LocationManager lm;
    lm.file_ends.push_back(0);
    LCompilers::LocationManager::FileLocations file;
    file.out_start.push_back(0); file.in_start.push_back(0); file.in_newlines.push_back(0);
    file.in_filename = "test"; file.current_line = 1; file.preprocessor = false; file.out_start0.push_back(0);
    file.in_start0.push_back(0); file.in_size0.push_back(0); file.interval_type0.push_back(0);
    file.in_newlines0.push_back(0);
    lm.files.push_back(file);
    return lm;
}

void asr_mod(const std::string &src) {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    ast0 = TRY(LCompilers::LFortran::parse(al, src, diagnostics, compiler_options));
    LCompilers::LocationManager lm;
    lm.file_ends.push_back(0);
    LCompilers::LocationManager::FileLocations file;
    file.out_start.push_back(0); file.in_start.push_back(0); file.in_newlines.push_back(0);
    file.in_filename = "test"; file.current_line = 1; file.preprocessor = false; file.out_start0.push_back(0);
    file.in_start0.push_back(0); file.in_size0.push_back(0); file.interval_type0.push_back(0);
    file.in_newlines0.push_back(0);
    lm.files.push_back(file);
    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));
    check_module_function_bodies(asr, false);

    std::string modfile = LCompilers::save_modfile(*asr, lm);
    check_module_function_bodies(asr, false);
    LCompilers::SymbolTable symtab(nullptr);
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> res
        = LCompilers::load_modfile(al, modfile, true, symtab, lm);
    CHECK(res.ok);
    LCompilers::ASR::TranslationUnit_t* asr2 = res.result;
    fix_external_symbols(*asr2, symtab);
    LCOMPILERS_ASSERT(LCompilers::asr_verify(*asr2, true, diagnostics));
    check_module_function_bodies(asr2, true);
}

TEST_CASE("AST Tests") {
    ast_ser("x = 2+2**2");

    ast_ser(R"""(
x = 2+2**2
)""");

    ast_ser("r = a - 2*x");

    ast_ser("r = f(a) - 2*x");

    ast_ser(R"""(
program expr2
implicit none
integer :: x
x = (2+3)*5
print *, x
end program
)""");

    ast_ser(R"""(
integer function f(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function
)""");

    ast_ser(R"""(
module modules_05_mod
implicit none
! TODO: the following line does not work yet
!private
integer, parameter, public :: a = 5
integer, parameter :: b = 6
integer, parameter :: c = 7
public :: c
end module


program modules_05
use modules_05_mod, only: a, c
print *, a, c
end program
)""");

    ast_ser(R"""(
program doconcurrentloop_01
implicit none
real, dimension(10000) :: a, b, c
real :: scalar
integer :: i, nsize
scalar = 10
nsize = size(a)
do concurrent (i = 1:nsize)
    a(i) = 5
    b(i) = 5
end do
call triad(a, b, scalar, c)
print *, "End Stream Triad"

contains

    subroutine triad(a, b, scalar, c)
    real, intent(in) :: a(:), b(:), scalar
    real, intent(out) :: c(:)
    integer :: N, i
    N = size(a)
    do concurrent (i = 1:N)
        c(i) = a(i) + scalar * b(i)
    end do
    end subroutine

end program
)""");

}

TEST_CASE("ASR Tests 1") {
    asr_ser(R"""(
program expr2
implicit none
integer :: x
x = (2+3)*5
print *, x
end program
)""");
}

TEST_CASE("ASR Tests 2") {
    asr_ser(R"""(
integer function f(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function
)""");
}

TEST_CASE("ASR Tests 3") {
    asr_ser(R"""(
program doconcurrentloop_01
implicit none
real, dimension(10000) :: a, b, c
real :: scalar
integer :: i, nsize
scalar = 10
nsize = size(a)
do concurrent (i = 1:nsize)
    a(i) = 5
    b(i) = 5
end do
call triad(a, b, scalar, c)
print *, "End Stream Triad"

contains

    subroutine triad(a, b, scalar, c)
    real, intent(in) :: a(:), b(:), scalar
    real, intent(out) :: c(:)
    integer :: N, i
    N = size(a)
    do concurrent (i = 1:N)
        c(i) = a(i) + scalar * b(i)
    end do
    end subroutine

end program
)""");
}

TEST_CASE("ASR Tests 4") {
    asr_ser(R"""(
module a
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module

program modules_01
use a, only: b
implicit none

call b()

end
)""");
}

TEST_CASE("ASR Tests 5") {
    asr_ser(R"""(
program derived_types_03
implicit none

type :: X
    integer :: i
end type

type(X) :: b

contains

    subroutine Y()
    type :: A
        integer :: i
    end type
    type(A) :: b
    end subroutine

    integer function Z()
    type :: A
        integer :: i
    end type
    type(A) :: b
    Z = 5
    end function

end
)""");

}

TEST_CASE("ASR modfile handling") {
    asr_mod(R"""(
module a
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module
)""");

}

TEST_CASE("ASR modfile preserves procedure-local parameter values") {
    asr_mod(R"""(
module local_params_mod
implicit none

contains

subroutine mindless01()
    integer, parameter :: nat = 3
    integer, parameter :: sym(nat) = [1, 8, 6]
    real(8), parameter :: xyz(3, nat) = reshape([ &
        1.0d0, 2.0d0, 3.0d0, &
        4.0d0, 5.0d0, 6.0d0, &
        7.0d0, 8.0d0, 9.0d0 ], [3, nat])
    print *, nat, sym(1), xyz(1, 1)
end subroutine

end module
)""");
}

TEST_CASE("ASR modfile strips only implementation payload") {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    const std::string src = R"""(
module modfile_interface_dependencies
implicit none
private
public :: payload, public_unused, api

type :: payload
    integer :: i
end type

type, public :: attr_public_payload
    integer :: k
end type

type :: public_unused
    integer :: j
end type

contains

pure integer function iface_len()
    iface_len = 3
end function

integer function body_only()
    body_only = 4
end function

subroutine api(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    block
        call worker(x, arr)
    end block
end subroutine

subroutine worker(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    integer :: body_local
    body_local = body_only()
    if (body_local /= 4) error stop
end subroutine

end module
)""";
    ast0 = TRY(LCompilers::LFortran::parse(al, src, diagnostics, compiler_options));
    LCompilers::LocationManager lm;
    lm.file_ends.push_back(0);
    LCompilers::LocationManager::FileLocations file;
    file.out_start.push_back(0); file.in_start.push_back(0); file.in_newlines.push_back(0);
    file.in_filename = "test"; file.current_line = 1; file.preprocessor = false; file.out_start0.push_back(0);
    file.in_start0.push_back(0); file.in_size0.push_back(0); file.interval_type0.push_back(0);
    file.in_newlines0.push_back(0);
    lm.files.push_back(file);
    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));

    LCompilers::ASR::Function_t *worker = get_module_function(asr,
        "modfile_interface_dependencies", "worker");
    CHECK(worker->n_body > 0);
    CHECK(worker->m_symtab->get_symbol("body_local") != nullptr);
    CHECK(function_has_dependency(worker, "iface_len"));
    CHECK(function_has_dependency(worker, "body_only"));

    std::string modfile = LCompilers::save_modfile(*asr, lm);
    CHECK(worker->n_body > 0);
    CHECK(worker->m_symtab->get_symbol("body_local") != nullptr);
    CHECK(function_has_dependency(worker, "iface_len"));
    CHECK(function_has_dependency(worker, "body_only"));

    LCompilers::SymbolTable symtab(nullptr);
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> res
        = LCompilers::load_modfile(al, modfile, true, symtab, lm);
    CHECK(res.ok);
    LCompilers::ASR::TranslationUnit_t* asr2 = res.result;
    fix_external_symbols(*asr2, symtab);
    LCOMPILERS_ASSERT(LCompilers::asr_verify(*asr2, true, diagnostics));

    LCompilers::ASR::Function_t *api2 = get_module_function(asr2,
        "modfile_interface_dependencies", "api");
    CHECK(api2->n_body == 0);
    CHECK(function_has_dependency(api2, "iface_len"));
    CHECK(!function_has_dependency(api2, "worker"));
    LCompilers::ASR::Function_t *iface_len2 = get_module_function(asr2,
        "modfile_interface_dependencies", "iface_len");
    CHECK(iface_len2->n_body == 0);

    LCompilers::ASR::symbol_t *module_sym = asr2->m_symtab->get_symbol(
        "modfile_interface_dependencies");
    REQUIRE(module_sym != nullptr);
    LCompilers::ASR::Module_t *module
        = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(module_sym);
    CHECK(module->m_symtab->get_symbol("payload") != nullptr);
    CHECK(module->m_symtab->get_symbol("attr_public_payload") != nullptr);
    CHECK(module->m_symtab->get_symbol("public_unused") != nullptr);
    CHECK(module->m_symtab->get_symbol("worker") == nullptr);
    CHECK(module->m_symtab->get_symbol("body_only") == nullptr);
}

TEST_CASE("ASR modfile filters body-only module dependencies") {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    const std::string src = R"""(
module modfile_sig_dep
implicit none

type :: payload
    integer :: i
end type

end module

module modfile_body_dep
implicit none

contains

integer function body_value()
    body_value = 4
end function

end module

module modfile_dependency_api
use modfile_sig_dep, only: payload
use modfile_body_dep, only: body_value
implicit none
private
public :: payload, api

contains

pure integer function iface_len()
    iface_len = 3
end function

subroutine api(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    call worker(x, arr)
end subroutine

subroutine worker(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    if (body_value() /= 4) error stop
end subroutine

end module
)""";
    ast0 = TRY(LCompilers::LFortran::parse(al, src, diagnostics, compiler_options));
    LCompilers::LocationManager lm;
    lm.file_ends.push_back(0);
    LCompilers::LocationManager::FileLocations file;
    file.out_start.push_back(0); file.in_start.push_back(0); file.in_newlines.push_back(0);
    file.in_filename = "test"; file.current_line = 1; file.preprocessor = false; file.out_start0.push_back(0);
    file.in_start0.push_back(0); file.in_size0.push_back(0); file.interval_type0.push_back(0);
    file.in_newlines0.push_back(0);
    lm.files.push_back(file);
    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));

    LCompilers::ASR::symbol_t *api_module_sym = asr->m_symtab->get_symbol(
        "modfile_dependency_api");
    REQUIRE(api_module_sym != nullptr);
    REQUIRE(LCompilers::ASR::is_a<LCompilers::ASR::Module_t>(*api_module_sym));
    LCompilers::ASR::Module_t *api_module
        = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(api_module_sym);
    CHECK(module_has_dependency(api_module, "modfile_sig_dep"));
    CHECK(module_has_dependency(api_module, "modfile_body_dep"));

    LCompilers::SymbolTable save_symtab(nullptr);
    save_symtab.add_symbol("modfile_dependency_api", api_module_sym);
    LCompilers::ASR::TranslationUnit_t *save_tu
        = LCompilers::ASR::down_cast2<LCompilers::ASR::TranslationUnit_t>(
            LCompilers::ASR::make_TranslationUnit_t(al, api_module->base.base.loc,
                &save_symtab, nullptr, 0));

    std::string modfile = LCompilers::save_modfile(*save_tu, lm);
    CHECK(module_has_dependency(api_module, "modfile_sig_dep"));
    CHECK(module_has_dependency(api_module, "modfile_body_dep"));

    LCompilers::SymbolTable symtab(nullptr);
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> res
        = LCompilers::load_modfile(al, modfile, true, symtab, lm);
    CHECK(res.ok);
    LCompilers::ASR::TranslationUnit_t* asr2 = res.result;

    LCompilers::ASR::symbol_t *module_sym = asr2->m_symtab->get_symbol(
        "modfile_dependency_api");
    REQUIRE(module_sym != nullptr);
    LCompilers::ASR::Module_t *module
        = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(module_sym);
    CHECK(module_has_dependency(module, "modfile_sig_dep"));
    CHECK(!module_has_dependency(module, "modfile_body_dep"));
    CHECK(module->m_symtab->get_symbol("payload") != nullptr);
    CHECK(module->m_symtab->get_symbol("api") != nullptr);
    CHECK(module->m_symtab->get_symbol("worker") == nullptr);
    CHECK(module->m_symtab->get_symbol("body_value") == nullptr);
}

TEST_CASE("ASR modfile downstream compile does not require stripped body dependencies") {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    const std::string src = R"""(
module modfile_downstream_sig_dep
implicit none

type :: payload
    integer :: i
end type

end module

module modfile_downstream_body_dep
implicit none

contains

integer function body_value()
    body_value = 4
end function

end module

module modfile_downstream_api
use modfile_downstream_sig_dep, only: payload
use modfile_downstream_body_dep, only: body_value
implicit none
private
public :: payload, api

contains

pure integer function iface_len()
    iface_len = 3
end function

subroutine api(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    call worker(x, arr)
end subroutine

subroutine worker(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    if (body_value() /= 4) error stop
end subroutine

end module
)""";
    ast0 = TRY(LCompilers::LFortran::parse(al, src, diagnostics, compiler_options));
    LCompilers::LocationManager lm = make_test_location_manager();
    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));

    LCompilers::ASR::symbol_t *sig_module_sym = asr->m_symtab->get_symbol(
        "modfile_downstream_sig_dep");
    LCompilers::ASR::symbol_t *api_module_sym = asr->m_symtab->get_symbol(
        "modfile_downstream_api");
    REQUIRE(sig_module_sym != nullptr);
    REQUIRE(api_module_sym != nullptr);

    LCompilers::SymbolTable sig_save_symtab(nullptr);
    sig_save_symtab.add_symbol("modfile_downstream_sig_dep", sig_module_sym);
    LCompilers::ASR::TranslationUnit_t *sig_save_tu
        = LCompilers::ASR::down_cast2<LCompilers::ASR::TranslationUnit_t>(
            LCompilers::ASR::make_TranslationUnit_t(al, sig_module_sym->base.loc,
                &sig_save_symtab, nullptr, 0));

    LCompilers::SymbolTable api_save_symtab(nullptr);
    api_save_symtab.add_symbol("modfile_downstream_api", api_module_sym);
    LCompilers::ASR::TranslationUnit_t *api_save_tu
        = LCompilers::ASR::down_cast2<LCompilers::ASR::TranslationUnit_t>(
            LCompilers::ASR::make_TranslationUnit_t(al, api_module_sym->base.loc,
                &api_save_symtab, nullptr, 0));

    std::string sig_modfile = LCompilers::save_modfile(*sig_save_tu, lm);
    std::string api_modfile = LCompilers::save_modfile(*api_save_tu, lm);

    LCompilers::SymbolTable downstream_symtab(nullptr);
    LCompilers::LocationManager downstream_lm = make_test_location_manager();
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> sig_res
        = LCompilers::load_modfile(al, sig_modfile, true, downstream_symtab, downstream_lm);
    CHECK(sig_res.ok);
    for (auto &item : sig_res.result->m_symtab->get_scope()) {
        if (LCompilers::ASR::is_a<LCompilers::ASR::Module_t>(*item.second)) {
            LCompilers::ASR::Module_t *module
                = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(item.second);
            module->m_symtab->parent = &downstream_symtab;
        }
        downstream_symtab.add_symbol(item.first, item.second);
    }
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> api_res
        = LCompilers::load_modfile(al, api_modfile, true, downstream_symtab, downstream_lm);
    CHECK(api_res.ok);
    for (auto &item : api_res.result->m_symtab->get_scope()) {
        if (LCompilers::ASR::is_a<LCompilers::ASR::Module_t>(*item.second)) {
            LCompilers::ASR::Module_t *module
                = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(item.second);
            module->m_symtab->parent = &downstream_symtab;
        }
        downstream_symtab.add_symbol(item.first, item.second);
    }
    CHECK(downstream_symtab.get_symbol("modfile_downstream_sig_dep") != nullptr);
    CHECK(downstream_symtab.get_symbol("modfile_downstream_api") != nullptr);
    CHECK(downstream_symtab.get_symbol("modfile_downstream_body_dep") == nullptr);

    const std::string downstream_src = R"""(
program modfile_downstream_user
use modfile_downstream_api, only: payload, api
implicit none
type(payload) :: x
integer :: arr(3, 7)

x%i = 7
arr = 8
call api(x, arr)
end program
)""";
    LCompilers::LFortran::AST::TranslationUnit_t* downstream_ast0;
    downstream_ast0 = TRY(LCompilers::LFortran::parse(al, downstream_src,
        diagnostics, compiler_options));
    LCompilers::ASR::TranslationUnit_t* downstream_asr = TRY(
        LCompilers::LFortran::ast_to_asr(al, *downstream_ast0, diagnostics,
            &downstream_symtab, false, compiler_options, downstream_lm));
    fix_external_symbols(*downstream_asr, downstream_symtab);
    LCOMPILERS_ASSERT(LCompilers::asr_verify(*downstream_asr, true, diagnostics));
    CHECK(downstream_symtab.get_symbol("modfile_downstream_body_dep") == nullptr);
}

TEST_CASE("ASR modfile derives dependencies from surviving external symbols") {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    const std::string src = R"""(
module modfile_surviving_dep
implicit none

type :: payload
    integer :: i
end type

end module

module modfile_surviving_api
use modfile_surviving_dep, only: payload
implicit none
private
public :: api_payload

type :: api_payload
    type(payload) :: value
end type

end module
)""";
    ast0 = TRY(LCompilers::LFortran::parse(al, src, diagnostics, compiler_options));
    LCompilers::LocationManager lm;
    lm.file_ends.push_back(0);
    LCompilers::LocationManager::FileLocations file;
    file.out_start.push_back(0); file.in_start.push_back(0); file.in_newlines.push_back(0);
    file.in_filename = "test"; file.current_line = 1; file.preprocessor = false; file.out_start0.push_back(0);
    file.in_start0.push_back(0); file.in_size0.push_back(0); file.interval_type0.push_back(0);
    file.in_newlines0.push_back(0);
    lm.files.push_back(file);
    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));

    LCompilers::ASR::symbol_t *api_module_sym = asr->m_symtab->get_symbol(
        "modfile_surviving_api");
    REQUIRE(api_module_sym != nullptr);
    REQUIRE(LCompilers::ASR::is_a<LCompilers::ASR::Module_t>(*api_module_sym));
    LCompilers::ASR::Module_t *api_module
        = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(api_module_sym);
    CHECK(module_has_dependency(api_module, "modfile_surviving_dep"));

    api_module->m_dependencies = nullptr;
    api_module->n_dependencies = 0;

    LCompilers::SymbolTable save_symtab(nullptr);
    save_symtab.add_symbol("modfile_surviving_api", api_module_sym);
    LCompilers::ASR::TranslationUnit_t *save_tu
        = LCompilers::ASR::down_cast2<LCompilers::ASR::TranslationUnit_t>(
            LCompilers::ASR::make_TranslationUnit_t(al, api_module->base.base.loc,
                &save_symtab, nullptr, 0));

    std::string modfile = LCompilers::save_modfile(*save_tu, lm);

    LCompilers::SymbolTable symtab(nullptr);
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> res
        = LCompilers::load_modfile(al, modfile, true, symtab, lm);
    CHECK(res.ok);
    LCompilers::ASR::TranslationUnit_t* asr2 = res.result;

    LCompilers::ASR::symbol_t *module_sym = asr2->m_symtab->get_symbol(
        "modfile_surviving_api");
    REQUIRE(module_sym != nullptr);
    LCompilers::ASR::Module_t *module
        = LCompilers::ASR::down_cast<LCompilers::ASR::Module_t>(module_sym);
    CHECK(module_has_dependency(module, "modfile_surviving_dep"));
    CHECK(module->m_symtab->get_symbol("api_payload") != nullptr);
    CHECK(module->m_symtab->get_symbol("payload") != nullptr);
}

TEST_CASE("ASR modfile preserves intrinsic helper bodies") {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    std::string src = R"""(
module lfortran_intrinsic_custom
implicit none

interface newunit
    procedure :: newunit_int_4
end interface

contains

integer function get_valid_newunit()
    get_valid_newunit = 9
end function

subroutine newunit_int_4(unit)
    integer(4), intent(out) :: unit
    unit = get_valid_newunit()
end subroutine

end module
)""";
    ast0 = TRY(LCompilers::LFortran::parse(al, src, diagnostics, compiler_options));
    LCompilers::LocationManager lm;
    lm.file_ends.push_back(0);
    LCompilers::LocationManager::FileLocations file;
    file.out_start.push_back(0); file.in_start.push_back(0); file.in_newlines.push_back(0);
    file.in_filename = "test"; file.current_line = 1; file.preprocessor = false; file.out_start0.push_back(0);
    file.in_start0.push_back(0); file.in_size0.push_back(0); file.interval_type0.push_back(0);
    file.in_newlines0.push_back(0);
    lm.files.push_back(file);
    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));

    std::string modfile = LCompilers::save_modfile(*asr, lm);

    LCompilers::SymbolTable symtab(nullptr);
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> res
        = LCompilers::load_modfile(al, modfile, true, symtab, lm);
    CHECK(res.ok);
    LCompilers::ASR::TranslationUnit_t* asr2 = res.result;
    fix_external_symbols(*asr2, symtab);
    LCOMPILERS_ASSERT(LCompilers::asr_verify(*asr2, true, diagnostics));

    LCompilers::ASR::Function_t *helper = get_module_function(asr2,
        "lfortran_intrinsic_custom", "get_valid_newunit");
    LCompilers::ASR::Function_t *newunit = get_module_function(asr2,
        "lfortran_intrinsic_custom", "newunit_int_4");
    CHECK(helper->n_body > 0);
    CHECK(newunit->n_body > 0);
    CHECK(function_has_dependency(newunit, "get_valid_newunit"));
}

TEST_CASE("ASR modfile accepts compiler version drift when schema matches") {
    Allocator al(4*1024);

    LCompilers::LFortran::AST::TranslationUnit_t* ast0;
    LCompilers::diag::Diagnostics diagnostics;
    LCompilers::CompilerOptions compiler_options;
    ast0 = TRY(LCompilers::LFortran::parse(al, R"""(
module a
implicit none
contains
subroutine b()
print *, "b()"
end subroutine
end module
)""", diagnostics, compiler_options));

    LCompilers::LocationManager lm;
    lm.file_ends.push_back(0);
    LCompilers::LocationManager::FileLocations file;
    file.out_start.push_back(0); file.in_start.push_back(0); file.in_newlines.push_back(0);
    file.in_filename = "test"; file.current_line = 1; file.preprocessor = false; file.out_start0.push_back(0);
    file.in_start0.push_back(0); file.in_size0.push_back(0); file.interval_type0.push_back(0);
    file.in_newlines0.push_back(0);
    lm.files.push_back(file);

    LCompilers::ASR::TranslationUnit_t* asr = TRY(LCompilers::LFortran::ast_to_asr(al, *ast0,
        diagnostics, nullptr, false, compiler_options, lm));

    std::string modfile = LCompilers::save_modfile(*asr, lm);
    std::string fake_version(std::strlen(LFORTRAN_VERSION), 'x');
    size_t version_pos = modfile.find(LFORTRAN_VERSION);
    REQUIRE(version_pos != std::string::npos);
    modfile.replace(version_pos, std::strlen(LFORTRAN_VERSION), fake_version);

    LCompilers::SymbolTable symtab(nullptr);
    LCompilers::Result<LCompilers::ASR::TranslationUnit_t*, LCompilers::ErrorMessage> res
        = LCompilers::load_modfile(al, modfile, true, symtab, lm);
    CHECK(res.ok);
}

TEST_CASE("Topological sorting mod_int") {
    std::map<std::string, std::vector<std::string>> deps;
    // 1 depends on 2
    deps["mod_1"].push_back("mod_2");
    // 3 depends on 1, etc.
    deps["mod_3"].push_back("mod_1");
    deps["mod_2"].push_back("mod_4");
    deps["mod_3"].push_back("mod_4");
    CHECK(LCompilers::ASRUtils::order_deps(deps) == std::vector<std::string>({"mod_4", "mod_2", "mod_1", "mod_3"}));

    deps.clear();
    deps["mod_1"].push_back("mod_2");
    deps["mod_1"].push_back("mod_3");
    deps["mod_2"].push_back("mod_4");
    deps["mod_3"].push_back("mod_4");
    CHECK(LCompilers::ASRUtils::order_deps(deps) == std::vector<std::string>({ "mod_4", "mod_2", "mod_3", "mod_1" }));

    deps.clear();
    deps["mod_1"].push_back("mod_2");
    deps["mod_3"].push_back("mod_1");
    deps["mod_3"].push_back("mod_4");
    deps["mod_4"].push_back("mod_1");
    CHECK(LCompilers::ASRUtils::order_deps(deps) == std::vector<std::string>({ "mod_2", "mod_1", "mod_4", "mod_3" }));
}

TEST_CASE("Topological sorting string") {
    std::map<std::string, std::vector<std::string>> deps;
    // A depends on B
    deps["A"].push_back("B");
    // C depends on A, etc.
    deps["C"].push_back("A");
    deps["B"].push_back("D");
    deps["C"].push_back("D");
    CHECK(LCompilers::ASRUtils::order_deps(deps) == std::vector<std::string>(
                {"D", "B", "A", "C"}));

    deps.clear();
    deps["module_a"].push_back("module_b");
    deps["module_c"].push_back("module_a");
    deps["module_c"].push_back("module_d");
    deps["module_d"].push_back("module_a");
    CHECK(LCompilers::ASRUtils::order_deps(deps) == std::vector<std::string>(
                {"module_b", "module_a", "module_d", "module_c"}));
}
