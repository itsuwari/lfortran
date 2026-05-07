#ifndef LFORTRAN_ASR_TO_C_CPP_H
#define LFORTRAN_ASR_TO_C_CPP_H

/*
 * Common code to be used in both of:
 *
 * * asr_to_cpp.cpp
 * * asr_to_c.cpp
 *
 * In particular, a common base class visitor with visitors that are identical
 * for both C and C++ code generation.
 */

#include <array>
#include <memory>
#include <cctype>
#include <cmath>
#include <numeric>
#include <set>
#include <vector>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_c.h>
#include <libasr/codegen/c_utils.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/pass/unused_functions.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_subroutine_registry.h>


#include <map>

#define CHECK_FAST_C_CPP(compiler_options, x)                   \
        if (compiler_options.po.fast && x.m_value != nullptr) { \
            self().visit_expr(*x.m_value);                      \
            return;                                             \
        }                                                       \


namespace LCompilers {


// Platform dependent fast unique hash:
static inline uint64_t get_hash(ASR::asr_t *node)
{
    return (uint64_t)node;
}

static inline uint64_t get_stable_string_hash(const std::string &value)
{
    uint64_t hash = 1469598103934665603ULL;
    for (unsigned char ch : value) {
        hash ^= static_cast<uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

static inline std::string replace_all_substrings(std::string value,
        const std::string &from, const std::string &to)
{
    if (from.empty()) {
        return value;
    }
    size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
    return value;
}

struct SymbolInfo
{
    bool needs_declaration = true;
    bool intrinsic_function = false;
};

struct DeclarationOptions {
};

struct CDeclarationOptions: public DeclarationOptions {
    bool pre_initialise_derived_type;
    bool use_ptr_for_derived_type;
    bool use_static;
    bool force_declare;
    std::string force_declare_name;
    bool declare_as_constant;
    std::string const_name;
    bool do_not_initialize;

    CDeclarationOptions() :
    pre_initialise_derived_type{true},
    use_ptr_for_derived_type{true},
    use_static{true},
    force_declare{false},
    force_declare_name{""},
    declare_as_constant{false},
    const_name{""},
    do_not_initialize{false} {
    }
};

struct CPPDeclarationOptions: public DeclarationOptions {
    bool use_static;
    bool use_templates_for_arrays;

    CPPDeclarationOptions() :
    use_static{true},
    use_templates_for_arrays{false} {
    }
};

enum class CArrayExprLoweringKind {
    FallbackRuntime,
    NoCopyDescriptorView,
    ScalarizedLoop,
    Rank2ScalarizedLoop,
    Rank2FullSelfUpdateLoop,
    FusedLoop,
    MaterializedTemporary,
    CompactConstantCopy
};

struct CArrayExprLoweringPlan {
    CArrayExprLoweringKind kind = CArrayExprLoweringKind::FallbackRuntime;
    ASR::expr_t *target_expr = nullptr;
    ASR::expr_t *target_base_expr = nullptr;
    ASR::expr_t *value_expr = nullptr;
    ASR::ArraySection_t *target_section = nullptr;
    ASR::ArrayConstant_t *constant_value = nullptr;
    ASR::ttype_t *constant_array_type = nullptr;
    ASR::ttype_t *constant_element_type = nullptr;
    size_t constant_size = 0;
};

struct CLocalScalarStructCleanup {
    std::string target;
    ASR::Struct_t *struct_t;
    bool is_polymorphic;
    bool shallow_copy;
};

struct CLocalStructCleanup {
    std::string target;
    ASR::Struct_t *struct_t;
    bool free_allocatable_array_descriptors;
    bool free_scalar_member_array_descriptors;
};

struct CLocalDescriptorCleanup {
    std::string target;
    std::string descriptor;
};

struct CLocalArrayStructCleanup {
    std::string descriptor;
    ASR::Struct_t *struct_t;
};

struct CLocalArrayStringCleanup {
    std::string descriptor;
};

struct CArrayDescriptorCache {
    std::string data;
    std::string offset;
    std::vector<std::string> lower_bounds;
    std::vector<std::string> strides;
};

struct CScalarExprCacheEntry {
    std::string name;
    std::set<ASR::symbol_t*> deps;
};

struct CLazyAutomaticArrayStorage {
    const ASR::Variable_t *var;
    std::string element_type;
    std::string heap_flag;
    std::string size_expr;
};

template <class StructType>
class BaseCCPPVisitor : public ASR::BaseVisitor<StructType>
{
private:
    StructType& self() { return static_cast<StructType&>(*this); }

    void emit_function_arg_initialization(const ASR::Function_t &,
            std::string &, const std::string &) {
    }
public:
    diag::Diagnostics &diag;
    Platform platform;
    // `src` acts as a buffer that accumulates the generated C/C++ source code
    // as the visitor traverses all the ASR nodes of a program. Each visitor method
    // uses `src` to return the result, and the caller visitor uses `src` as the
    // value of the callee visitors it calls. The C/C++ complete source code
    // is then recursively constructed using `src`.
    std::string src;
    std::string current_body;
    CompilerOptions &compiler_options;
    int indentation_level;
    int indentation_spaces;
    // The precedence of the last expression, using the table:
    // https://en.cppreference.com/w/cpp/language/operator_precedence
    int last_expr_precedence;
    bool intrinsic_module = false;
    const ASR::Function_t *current_function = nullptr;
    std::string current_return_var_name;
    bool current_function_has_explicit_return = false;
    std::vector<std::string> current_function_heap_array_data;
    std::vector<std::pair<std::string, std::string>> current_function_conditional_heap_array_data;
    std::vector<std::string> current_function_local_allocatable_arrays;
    std::vector<CLocalArrayStructCleanup> current_function_local_allocatable_array_structs;
    std::vector<CLocalArrayStringCleanup> current_function_local_allocatable_array_strings;
    std::vector<std::string> current_function_local_allocatable_strings;
    std::vector<std::string> current_function_local_allocatable_scalars;
    std::vector<std::string> current_function_local_character_strings;
    std::vector<CLocalScalarStructCleanup> current_function_local_allocatable_structs;
    std::vector<CLocalStructCleanup> current_function_local_structs;
    std::vector<CLocalDescriptorCleanup> current_function_local_descriptors;
    std::set<uint64_t> current_c_struct_cleanup_stack;
    std::map<std::string, CArrayDescriptorCache> current_function_array_descriptor_cache;
    std::map<std::string, CScalarExprCacheEntry> current_function_pow_cache;
    std::map<std::string, CLazyAutomaticArrayStorage>
        current_function_lazy_automatic_array_storage;
    int c_pow_cache_safe_expr_depth = 0;
    int c_pow_cache_suppression_depth = 0;
    std::map<uint64_t, SymbolInfo> sym_info;
    std::map<uint64_t, std::string> const_var_names;
    std::map<int32_t, std::string> gotoid2name;
    std::map<std::string, std::string> emit_headers;
    std::map<SymbolTable*, std::set<std::string>> emitted_local_names;
    std::string array_types_decls;
    std::string forward_decl_functions;
    std::vector<ASR::Function_t*> pending_function_definitions;
    std::set<uint64_t> pending_function_definition_hashes;
    bool emit_compact_constant_data_units = false;
    size_t compact_constant_data_count = 0;
    std::string compact_constant_data_body;
    std::string compact_constant_data_decls;

    // Output configuration:
    // Use std::string or char*
    bool gen_stdstring;
    // Use std::complex<float/double> or float/double complex
    bool gen_stdcomplex;
    bool is_c;
    std::set<std::string> headers, user_headers, user_defines;
    std::set<std::string> emitted_pointer_backed_struct_names;
    std::vector<std::string> tmp_buffer_src;
    SymbolTable* global_scope;
    int64_t lower_bound;

    std::string template_for_Kokkos;
    size_t template_number;
    std::string from_std_vector_helper;
    bool force_storage_expr_in_call_args = false;
    bool reuse_array_compare_temps_in_call_args = false;
    std::map<std::string, std::string> array_compare_temp_cache;
    std::set<uint64_t> c_array_section_association_temps;

    std::unique_ptr<CCPPDSUtils> c_ds_api;
    std::unique_ptr<CUtils::CUtilFunctions> c_utils_functions;
    std::unique_ptr<BindPyUtils::BindPyUtilFunctions> bind_py_utils_functions;
    std::string const_name;
    size_t const_vars_count;
    size_t loop_end_count;

    // This is used to track if during the codegeneration whether or not
    // the source is inside any bracket. bracket_open is always >= 0. We
    // increment when we come-across a open bracket and decrement when we
    // come-across a closing bracket.
    // This helps in putting the extra code-generation (mainly of Constants)
    // in the right place and avoid producing syntax errors.
    // For example:
    // In FunctionCall node: we do `some_fun(` -> bracket_open++
    // and when we close the bracket `...)` -> bracket_open--

    int bracket_open;

    SymbolTable* current_scope;
    bool is_string_concat_present;

    BaseCCPPVisitor(diag::Diagnostics &diag, Platform &platform,
            CompilerOptions &_compiler_options, bool gen_stdstring, bool gen_stdcomplex, bool is_c,
            int64_t default_lower_bound) : diag{diag},
            platform{platform}, compiler_options{_compiler_options}, array_types_decls{std::string("")},
        gen_stdstring{gen_stdstring}, gen_stdcomplex{gen_stdcomplex},
        is_c{is_c}, global_scope{nullptr}, lower_bound{default_lower_bound},
        template_number{0}, c_ds_api{std::make_unique<CCPPDSUtils>(is_c, platform)},
        c_utils_functions{std::make_unique<CUtils::CUtilFunctions>()},
        bind_py_utils_functions{std::make_unique<BindPyUtils::BindPyUtilFunctions>()},
        const_name{"constname"},
        const_vars_count{0}, loop_end_count{0}, bracket_open{0},
        is_string_concat_present{false} {
        }

    std::string emit_c_character_array_element_cleanup(
            const std::string &descriptor, const std::string &indent) {
        std::string idx = "__lfortran_cleanup_i_"
            + CUtils::sanitize_c_identifier(descriptor);
        std::string size = "__lfortran_cleanup_size_"
            + CUtils::sanitize_c_identifier(descriptor);
        std::string dim = "__lfortran_cleanup_dim_"
            + CUtils::sanitize_c_identifier(descriptor);
        return indent + "if ((" + descriptor + ") != NULL && (" + descriptor
            + ")->is_allocated && (" + descriptor + ")->data != NULL) {\n"
            + indent + "    int64_t " + size + " = 1;\n"
            + indent + "    for (int32_t " + dim + " = 0; "
            + dim + " < (" + descriptor + ")->n_dims; " + dim + "++) {\n"
            + indent + "        " + size + " *= (" + descriptor
            + ")->dims[" + dim + "].length;\n"
            + indent + "    }\n"
            + indent + "    for (int64_t " + idx + " = 0; " + idx
            + " < " + size + "; " + idx + "++) {\n"
            + indent + "        if ((" + descriptor + ")->data[" + idx
            + "] != NULL) {\n"
            + indent + "            _lfortran_free_alloc(_lfortran_get_default_allocator(), "
            + "(" + descriptor + ")->data[" + idx + "]);\n"
            + indent + "            (" + descriptor + ")->data[" + idx
            + "] = NULL;\n"
            + indent + "        }\n"
            + indent + "    }\n"
            + indent + "}\n";
    }

    std::string emit_current_function_heap_array_cleanup(const std::string &indent) {
        std::string cleanup;
        for (auto it = current_function_local_structs.rbegin();
                it != current_function_local_structs.rend(); ++it) {
            if (it->free_allocatable_array_descriptors
                    && it->free_scalar_member_array_descriptors
                    && c_struct_has_member_cleanup(it->struct_t)) {
                cleanup += emit_c_registered_struct_cleanup(
                    it->struct_t, indent, it->target);
            } else {
                cleanup += emit_c_struct_member_cleanup(it->struct_t, indent, it->target,
                    true, true, it->free_allocatable_array_descriptors,
                    it->free_scalar_member_array_descriptors);
            }
        }
        for (auto it = current_function_local_descriptors.rbegin();
                it != current_function_local_descriptors.rend(); ++it) {
            cleanup += indent + "if ((" + it->target + ") == " + it->descriptor + ") {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + "(char*) " + it->descriptor + ");\n"
                + indent + "    " + it->target + " = NULL;\n"
                + indent + "    " + it->descriptor + " = NULL;\n"
                + indent + "}\n";
        }
        for (auto it = current_function_local_allocatable_structs.rbegin();
                it != current_function_local_allocatable_structs.rend(); ++it) {
            if (it->shallow_copy) {
                cleanup += emit_c_shallow_copied_struct_root_cleanup(
                    it->struct_t, indent, it->target, it->is_polymorphic);
            } else {
                cleanup += emit_c_scalar_allocatable_struct_cleanup(
                    it->struct_t, indent, it->target, it->is_polymorphic);
            }
        }
        for (auto it = current_function_local_allocatable_arrays.rbegin();
                it != current_function_local_allocatable_arrays.rend(); ++it) {
            for (auto string_it = current_function_local_allocatable_array_strings.rbegin();
                    string_it != current_function_local_allocatable_array_strings.rend();
                    ++string_it) {
                if (string_it->descriptor != *it) {
                    continue;
                }
                cleanup += emit_c_character_array_element_cleanup(*it, indent);
                break;
            }
            for (auto struct_it = current_function_local_allocatable_array_structs.rbegin();
                    struct_it != current_function_local_allocatable_array_structs.rend();
                    ++struct_it) {
                if (struct_it->descriptor != *it) {
                    continue;
                }
                std::string idx = "__lfortran_cleanup_i_"
                    + CUtils::sanitize_c_identifier(*it);
                cleanup += indent + "if ((" + *it + ") != NULL && (" + *it
                    + ")->is_allocated && (" + *it + ")->data != NULL) {\n"
                    + indent + "    for (int64_t " + idx + " = 0; " + idx
                    + " < (" + *it + ")->dims[0].length; " + idx + "++) {\n";
                cleanup += emit_c_struct_member_cleanup(struct_it->struct_t,
                    indent + "        ", "(&((" + *it + ")->data[" + idx + "]))",
                    true, true, true, false);
                cleanup += indent + "    }\n"
                    + indent + "}\n";
                break;
            }
            cleanup += indent + "if ((" + *it + ") != NULL && (" + *it
                + ")->is_allocated && (" + *it + ")->data != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + "(char*) (" + *it + ")->data);\n"
                + indent + "    (" + *it + ")->data = NULL;\n"
                + indent + "}\n"
                + indent + "if ((" + *it + ") != NULL) {\n"
                + indent + "    (" + *it + ")->offset = 0;\n"
                + indent + "    (" + *it + ")->is_allocated = false;\n"
                + indent + "}\n";
        }
        for (auto it = current_function_local_allocatable_strings.rbegin();
                it != current_function_local_allocatable_strings.rend(); ++it) {
            cleanup += indent + "if (" + *it + " != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + *it + ");\n"
                + indent + "    " + *it + " = NULL;\n"
                + indent + "}\n";
        }
        for (auto it = current_function_local_allocatable_scalars.rbegin();
                it != current_function_local_allocatable_scalars.rend(); ++it) {
            cleanup += indent + "if (" + *it + " != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + "(char*) " + *it + ");\n"
                + indent + "    " + *it + " = NULL;\n"
                + indent + "}\n";
        }
        for (auto it = current_function_local_character_strings.rbegin();
                it != current_function_local_character_strings.rend(); ++it) {
            cleanup += indent + "if (" + *it + " != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + *it + ");\n"
                + indent + "    " + *it + " = NULL;\n"
                + indent + "}\n";
        }
        for (auto it = current_function_heap_array_data.rbegin();
                it != current_function_heap_array_data.rend(); ++it) {
            cleanup += indent + "free(" + *it + ");\n";
        }
        for (auto it = current_function_conditional_heap_array_data.rbegin();
                it != current_function_conditional_heap_array_data.rend(); ++it) {
            cleanup += indent + "if (" + it->first + ") {\n";
            cleanup += indent + "    free(" + it->second + ");\n";
            cleanup += indent + "}\n";
        }
        return cleanup;
    }

    void clear_current_function_cleanup_state() {
        current_function_heap_array_data.clear();
        current_function_conditional_heap_array_data.clear();
        current_function_local_allocatable_arrays.clear();
        current_function_local_allocatable_array_structs.clear();
        current_function_local_allocatable_array_strings.clear();
        current_function_local_allocatable_strings.clear();
        current_function_local_allocatable_scalars.clear();
        current_function_local_character_strings.clear();
        current_function_local_allocatable_structs.clear();
        current_function_local_structs.clear();
        current_function_local_descriptors.clear();
    }

    std::string emit_c_lazy_automatic_array_temp_allocation(
            ASR::expr_t*, const std::string&) {
        return "";
    }

    void register_current_function_local_allocatable_array_cleanup(
            const std::string &descriptor) {
        for (const std::string &existing: current_function_local_allocatable_arrays) {
            if (existing == descriptor) {
                return;
            }
        }
        current_function_local_allocatable_arrays.push_back(descriptor);
    }

    void register_current_function_local_allocatable_array_struct_cleanup(
            const std::string &descriptor, ASR::Struct_t *struct_t) {
        for (const auto &existing: current_function_local_allocatable_array_structs) {
            if (existing.descriptor == descriptor) {
                return;
            }
        }
        current_function_local_allocatable_array_structs.push_back({descriptor, struct_t});
    }

    void register_current_function_local_allocatable_array_string_cleanup(
            const std::string &descriptor) {
        for (const auto &existing: current_function_local_allocatable_array_strings) {
            if (existing.descriptor == descriptor) {
                return;
            }
        }
        current_function_local_allocatable_array_strings.push_back({descriptor});
    }

    void register_current_function_local_allocatable_string_cleanup(
            const std::string &target) {
        for (const std::string &existing: current_function_local_allocatable_strings) {
            if (existing == target) {
                return;
            }
        }
        current_function_local_allocatable_strings.push_back(target);
    }

    void register_current_function_local_allocatable_scalar_cleanup(
            const std::string &target) {
        for (const std::string &existing: current_function_local_allocatable_scalars) {
            if (existing == target) {
                return;
            }
        }
        current_function_local_allocatable_scalars.push_back(target);
    }

    void register_current_function_local_character_string_cleanup(
            const std::string &target) {
        for (const std::string &existing: current_function_local_character_strings) {
            if (existing == target) {
                return;
            }
        }
        current_function_local_character_strings.push_back(target);
    }

    void register_current_function_local_allocatable_struct_cleanup(
            const std::string &target, ASR::Struct_t *struct_t, bool is_polymorphic,
            bool shallow_copy=false) {
        for (const auto &existing: current_function_local_allocatable_structs) {
            if (existing.target == target) {
                return;
            }
        }
        current_function_local_allocatable_structs.push_back(
            {target, struct_t, is_polymorphic, shallow_copy});
    }

    std::string emit_c_tagged_struct_member_cleanup_dispatch(
            const std::string &indent, const std::string &target) {
        std::string cleanup;
        ensure_runtime_type_tag_header_decl();
        cleanup += indent + "if ((" + target + ") != NULL) {\n";
        std::string inner_indent = indent + "    ";
        cleanup += inner_indent + "_lfortran_cleanup_c_struct(((struct "
            + get_runtime_type_tag_header_struct_name() + "*)(" + target
            + "))->" + get_runtime_type_tag_member_name() + ", (void*) "
            + target + ");\n";
        cleanup += inner_indent
            + "_lfortran_free_alloc(_lfortran_get_default_allocator(), (char*) "
            + target + ");\n";
        cleanup += inner_indent + target + " = NULL;\n";
        cleanup += indent + "}\n";
        return cleanup;
    }

    std::string emit_c_registered_struct_cleanup(
            ASR::Struct_t *struct_t, const std::string &indent,
            const std::string &target) {
        if (struct_t == nullptr) {
            return "";
        }
        ASR::symbol_t *struct_sym = reinterpret_cast<ASR::symbol_t*>(struct_t);
        int64_t type_id = get_struct_runtime_type_id(struct_sym);
        return indent + "_lfortran_cleanup_c_struct("
            + std::to_string(type_id) + ", (void*) " + target + ");\n";
    }

    std::string emit_c_struct_member_cleanup(ASR::Struct_t *struct_t,
            const std::string &indent, const std::string &target,
            bool clean_polymorphic_scalars=true,
            bool clean_scalar_allocatable_structs=true,
            bool free_allocatable_array_descriptors=true,
            bool free_scalar_member_array_descriptors=true) {
        std::string cleanup;
        if (struct_t == nullptr) {
            return cleanup;
        }
        uint64_t cleanup_key = get_hash(reinterpret_cast<ASR::asr_t*>(struct_t));
        if (!current_c_struct_cleanup_stack.insert(cleanup_key).second) {
            return cleanup;
        }
        if (struct_t->m_parent != nullptr) {
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(
                struct_t->m_parent);
            if (parent_sym != nullptr && ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                cleanup += emit_c_struct_member_cleanup(
                    ASR::down_cast<ASR::Struct_t>(parent_sym), indent, target,
                    clean_polymorphic_scalars, clean_scalar_allocatable_structs,
                    free_allocatable_array_descriptors,
                    free_scalar_member_array_descriptors);
            }
        }
        for (size_t i = 0; i < struct_t->n_members; i++) {
            if (struct_t->m_members[i] == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym0 = struct_t->m_symtab->get_symbol(
                struct_t->m_members[i]);
            if (member_sym0 == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(
                member_sym0);
            if (member_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*member_sym)) {
                continue;
            }
            ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            std::string member = target + "->" + CUtils::get_c_member_name(member_sym);
            ASR::ttype_t *member_type = ASRUtils::type_get_past_allocatable_pointer(
                member_var->m_type);
            ASR::ttype_t *member_element_type = ASRUtils::extract_type(member_type);
            if (ASRUtils::is_allocatable(member_var->m_type)
                    && ASRUtils::is_array(member_var->m_type)
                    && !ASRUtils::is_class_type(member_element_type)
                    && !(ASR::is_a<ASR::StructType_t>(*member_element_type)
                        && ASR::down_cast<ASR::StructType_t>(
                            member_element_type)->m_is_unlimited_polymorphic)) {
                if (member_element_type != nullptr
                        && ASRUtils::is_character(*member_element_type)) {
                    cleanup += emit_c_character_array_element_cleanup(member, indent);
                } else if (ASR::is_a<ASR::StructType_t>(*member_element_type)
                        && member_var->m_type_declaration != nullptr) {
                    ASR::symbol_t *member_struct_sym =
                        ASRUtils::symbol_get_past_external(member_var->m_type_declaration);
                    if (member_struct_sym != nullptr
                            && ASR::is_a<ASR::Struct_t>(*member_struct_sym)
                            && c_struct_has_member_cleanup(
                                ASR::down_cast<ASR::Struct_t>(member_struct_sym))) {
                        std::string idx = "__lfortran_cleanup_i_"
                            + CUtils::sanitize_c_identifier(member);
                        std::string size = "__lfortran_cleanup_size_"
                            + CUtils::sanitize_c_identifier(member);
                        std::string dim = "__lfortran_cleanup_dim_"
                            + CUtils::sanitize_c_identifier(member);
                        cleanup += indent + "if ((" + member + ") != NULL && ("
                            + member + ")->is_allocated && (" + member
                            + ")->data != NULL) {\n"
                            + indent + "    int64_t " + size + " = 1;\n"
                            + indent + "    for (int32_t " + dim + " = 0; "
                            + dim + " < (" + member + ")->n_dims; " + dim
                            + "++) {\n"
                            + indent + "        " + size + " *= (" + member
                            + ")->dims[" + dim + "].length;\n"
                            + indent + "    }\n"
                            + indent + "    for (int64_t " + idx + " = 0; "
                            + idx + " < " + size + "; " + idx + "++) {\n";
                        cleanup += emit_c_struct_member_cleanup(
                            ASR::down_cast<ASR::Struct_t>(member_struct_sym),
                            indent + "        ", "(&((" + member + ")->data[" + idx + "]))",
                            clean_polymorphic_scalars, clean_scalar_allocatable_structs,
                            true, false);
                        cleanup += indent + "    }\n"
                            + indent + "}\n";
                    }
                }
                std::string descriptor_owned = "__lfortran_cleanup_desc_owned_"
                    + CUtils::sanitize_c_identifier(member);
                cleanup += indent + "bool " + descriptor_owned + " = ("
                    + member + ") != NULL && (" + member + ")->is_allocated;\n";
                cleanup += indent + "if ((" + member + ") != NULL && (" + member
                    + ")->is_allocated && (" + member + ")->data != NULL) {\n"
                    + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                    + "(char*) (" + member + ")->data);\n"
                    + indent + "    (" + member + ")->data = NULL;\n"
                    + indent + "}\n";
                if (free_allocatable_array_descriptors) {
                    cleanup += indent + "if ((" + member + ") != NULL) {\n"
                        + indent + "    (" + member + ")->offset = 0;\n"
                        + indent + "    (" + member + ")->is_allocated = false;\n"
                        + indent + "    if (" + descriptor_owned + ") {\n"
                        + indent + "        _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                        + "(char*) " + member + ");\n"
                        + indent + "        " + member + " = NULL;\n"
                        + indent + "    }\n"
                        + indent + "}\n";
                } else {
                    cleanup += indent + "if ((" + member + ") != NULL) {\n"
                    + indent + "    (" + member + ")->offset = 0;\n"
                    + indent + "    (" + member + ")->is_allocated = false;\n"
                    + indent + "}\n";
                }
            } else if (ASRUtils::is_allocatable(member_var->m_type)
                    && !ASRUtils::is_array(member_var->m_type)
                    && ASRUtils::is_character(*member_type)) {
                cleanup += indent + "if (" + member + " != NULL) {\n"
                    + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                    + member + ");\n"
                    + indent + "    " + member + " = NULL;\n"
                    + indent + "}\n";
            } else if (ASRUtils::is_allocatable(member_var->m_type)
                    && !ASRUtils::is_array(member_var->m_type)
                    && ASR::is_a<ASR::StructType_t>(*member_type)) {
                ASR::StructType_t *struct_member_type =
                    ASR::down_cast<ASR::StructType_t>(member_type);
                bool is_polymorphic = struct_member_type->m_is_unlimited_polymorphic
                    || ASRUtils::is_class_type(member_var->m_type)
                    || ASRUtils::is_class_type(member_type);
                ASR::Struct_t *member_struct = nullptr;
                if (!is_polymorphic && member_var->m_type_declaration != nullptr) {
                    ASR::symbol_t *member_struct_sym =
                        ASRUtils::symbol_get_past_external(member_var->m_type_declaration);
                    if (member_struct_sym != nullptr
                            && ASR::is_a<ASR::Struct_t>(*member_struct_sym)) {
                        member_struct = ASR::down_cast<ASR::Struct_t>(member_struct_sym);
                    }
                }
                if ((is_polymorphic && clean_polymorphic_scalars)
                        || (clean_scalar_allocatable_structs
                            && member_struct != nullptr)) {
                    cleanup += emit_c_scalar_allocatable_struct_cleanup(
                        member_struct, indent, member, is_polymorphic,
                        clean_polymorphic_scalars,
                        free_scalar_member_array_descriptors);
                }
            } else if (ASRUtils::is_allocatable(member_var->m_type)
                    && !ASRUtils::is_array(member_var->m_type)) {
                cleanup += indent + "if (" + member + " != NULL) {\n"
                    + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                    + "(char*) " + member + ");\n"
                    + indent + "    " + member + " = NULL;\n"
                    + indent + "}\n";
            } else {
                bool is_plain_struct_member = !ASRUtils::is_array(member_var->m_type)
                    && !ASRUtils::is_allocatable(member_var->m_type)
                    && !ASRUtils::is_pointer(member_var->m_type)
                    && ASR::is_a<ASR::StructType_t>(*member_type)
                    && !ASRUtils::is_class_type(member_type);
                if (is_plain_struct_member && member_var->m_type_declaration != nullptr) {
                    ASR::symbol_t *member_struct_sym =
                        ASRUtils::symbol_get_past_external(member_var->m_type_declaration);
                    if (member_struct_sym != nullptr
                            && ASR::is_a<ASR::Struct_t>(*member_struct_sym)) {
                        cleanup += emit_c_struct_member_cleanup(
                            ASR::down_cast<ASR::Struct_t>(member_struct_sym),
                            indent, "(&(" + member + "))",
                            clean_polymorphic_scalars,
                            clean_scalar_allocatable_structs,
                            free_allocatable_array_descriptors,
                            free_scalar_member_array_descriptors);
                    }
                }
            }
        }
        current_c_struct_cleanup_stack.erase(cleanup_key);
        return cleanup;
    }

    std::string emit_c_shallow_copied_struct_member_root_cleanup(
            ASR::Struct_t *struct_t, const std::string &indent,
            const std::string &target) {
        std::string cleanup;
        if (struct_t == nullptr) {
            return cleanup;
        }
        uint64_t cleanup_key = get_hash(reinterpret_cast<ASR::asr_t*>(struct_t));
        if (!current_c_struct_cleanup_stack.insert(cleanup_key).second) {
            return cleanup;
        }
        if (struct_t->m_parent != nullptr) {
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(
                struct_t->m_parent);
            if (parent_sym != nullptr && ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                cleanup += emit_c_shallow_copied_struct_member_root_cleanup(
                    ASR::down_cast<ASR::Struct_t>(parent_sym), indent, target);
            }
        }
        for (size_t i = 0; i < struct_t->n_members; i++) {
            if (struct_t->m_members[i] == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym0 = struct_t->m_symtab->get_symbol(
                struct_t->m_members[i]);
            if (member_sym0 == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(
                member_sym0);
            if (member_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*member_sym)) {
                continue;
            }
            ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            if (!ASRUtils::is_allocatable(member_var->m_type)
                    || ASRUtils::is_array(member_var->m_type)) {
                continue;
            }
            ASR::ttype_t *member_type =
                ASRUtils::type_get_past_allocatable_pointer(member_var->m_type);
            if (member_type == nullptr || !ASR::is_a<ASR::StructType_t>(*member_type)
                    || member_var->m_type_declaration == nullptr) {
                continue;
            }
            ASR::StructType_t *struct_member_type =
                ASR::down_cast<ASR::StructType_t>(member_type);
            bool is_polymorphic = struct_member_type->m_is_unlimited_polymorphic
                || ASRUtils::is_class_type(member_var->m_type)
                || ASRUtils::is_class_type(member_type);
            if (is_polymorphic) {
                continue;
            }
            ASR::symbol_t *member_struct_sym =
                ASRUtils::symbol_get_past_external(member_var->m_type_declaration);
            if (member_struct_sym == nullptr
                    || !ASR::is_a<ASR::Struct_t>(*member_struct_sym)) {
                continue;
            }
            std::string member = target + "->" + CUtils::get_c_member_name(member_sym);
            cleanup += indent + "if ((" + member + ") != NULL) {\n";
            cleanup += emit_c_shallow_copied_struct_member_root_cleanup(
                ASR::down_cast<ASR::Struct_t>(member_struct_sym),
                indent + "    ", member);
            cleanup += indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + "(char*) " + member + ");\n"
                + indent + "    " + member + " = NULL;\n"
                + indent + "}\n";
        }
        current_c_struct_cleanup_stack.erase(cleanup_key);
        return cleanup;
    }

    std::string emit_c_shallow_copied_struct_root_cleanup(ASR::Struct_t *struct_t,
            const std::string &indent, const std::string &target,
            bool is_polymorphic) {
        std::string cleanup;
        cleanup += indent + "if ((" + target + ") != NULL) {\n";
        if (!is_polymorphic) {
            cleanup += emit_c_shallow_copied_struct_member_root_cleanup(
                struct_t, indent + "    ", target);
        }
        cleanup += indent
            + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), (char*) "
            + target + ");\n"
            + indent + "    " + target + " = NULL;\n"
            + indent + "}\n";
        return cleanup;
    }

    std::string emit_c_scalar_allocatable_struct_cleanup(ASR::Struct_t *struct_t,
            const std::string &indent, const std::string &target,
            bool is_polymorphic, bool clean_polymorphic_scalars=true,
            bool free_member_array_descriptors=true) {
        std::string cleanup;
        if (struct_t == nullptr && !is_polymorphic) {
            return cleanup;
        }
        cleanup += indent + "if ((" + target + ") != NULL) {\n";
        std::string inner_indent = indent + "    ";
        if (is_polymorphic) {
            ensure_runtime_type_tag_header_decl();
            cleanup += inner_indent + "_lfortran_cleanup_c_struct(((struct "
                + get_runtime_type_tag_header_struct_name() + "*)(" + target
                + "))->" + get_runtime_type_tag_member_name() + ", (void*) "
                + target + ");\n";
        } else {
            cleanup += emit_c_struct_member_cleanup(struct_t, inner_indent, target,
                clean_polymorphic_scalars, true, free_member_array_descriptors);
        }
        cleanup += inner_indent
            + "_lfortran_free_alloc(_lfortran_get_default_allocator(), (char*) "
            + target + ");\n";
        cleanup += inner_indent + target + " = NULL;\n";
        cleanup += indent + "}\n";
        return cleanup;
    }

    bool c_struct_has_member_cleanup(ASR::Struct_t *struct_t,
            std::set<uint64_t> &seen) {
        if (struct_t == nullptr) {
            return false;
        }
        uint64_t cleanup_key = get_hash(reinterpret_cast<ASR::asr_t*>(struct_t));
        if (!seen.insert(cleanup_key).second) {
            return false;
        }
        if (struct_t->m_parent != nullptr) {
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(
                struct_t->m_parent);
            if (parent_sym != nullptr && ASR::is_a<ASR::Struct_t>(*parent_sym)
                    && c_struct_has_member_cleanup(
                        ASR::down_cast<ASR::Struct_t>(parent_sym), seen)) {
                return true;
            }
        }
        for (size_t i = 0; i < struct_t->n_members; i++) {
            if (struct_t->m_members[i] == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym0 = struct_t->m_symtab->get_symbol(
                struct_t->m_members[i]);
            if (member_sym0 == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(
                member_sym0);
            if (member_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*member_sym)) {
                continue;
            }
            ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            ASR::ttype_t *member_type = ASRUtils::type_get_past_allocatable_pointer(
                member_var->m_type);
            if (member_type == nullptr) {
                continue;
            }
            ASR::ttype_t *member_element_type = ASRUtils::extract_type(member_type);
            if (ASRUtils::is_allocatable(member_var->m_type)
                    && ASRUtils::is_array(member_var->m_type)
                    && !ASRUtils::is_class_type(member_element_type)
                    && !(ASR::is_a<ASR::StructType_t>(*member_element_type)
                        && ASR::down_cast<ASR::StructType_t>(
                            member_element_type)->m_is_unlimited_polymorphic)) {
                return true;
            }
            if (ASRUtils::is_allocatable(member_var->m_type)
                    && !ASRUtils::is_array(member_var->m_type)
                    && ASRUtils::is_character(*member_type)) {
                return true;
            }
            if (ASRUtils::is_allocatable(member_var->m_type)
                    && !ASRUtils::is_array(member_var->m_type)
                    && ASR::is_a<ASR::StructType_t>(*member_type)
                    && (ASR::down_cast<ASR::StructType_t>(
                            member_type)->m_is_unlimited_polymorphic
                        || member_var->m_type_declaration != nullptr)) {
                return true;
            }
            if (ASRUtils::is_allocatable(member_var->m_type)
                    && !ASRUtils::is_array(member_var->m_type)) {
                return true;
            }
            bool is_plain_struct_member = !ASRUtils::is_array(member_var->m_type)
                && !ASRUtils::is_allocatable(member_var->m_type)
                && !ASRUtils::is_pointer(member_var->m_type)
                && ASR::is_a<ASR::StructType_t>(*member_type)
                && !ASRUtils::is_class_type(member_type);
            if (is_plain_struct_member && member_var->m_type_declaration != nullptr) {
                ASR::symbol_t *member_struct_sym =
                    ASRUtils::symbol_get_past_external(member_var->m_type_declaration);
                if (member_struct_sym != nullptr
                        && ASR::is_a<ASR::Struct_t>(*member_struct_sym)
                        && c_struct_has_member_cleanup(
                            ASR::down_cast<ASR::Struct_t>(member_struct_sym),
                            seen)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool c_struct_has_member_cleanup(ASR::Struct_t *struct_t) {
        std::set<uint64_t> seen;
        return c_struct_has_member_cleanup(struct_t, seen);
    }

    void register_current_function_local_struct_cleanup(
            const std::string &target, ASR::Struct_t *struct_t,
            bool free_allocatable_array_descriptors=true,
            bool free_scalar_member_array_descriptors=true) {
        for (auto &existing: current_function_local_structs) {
            if (existing.target == target) {
                existing.free_allocatable_array_descriptors =
                    existing.free_allocatable_array_descriptors
                    || free_allocatable_array_descriptors;
                existing.free_scalar_member_array_descriptors =
                    existing.free_scalar_member_array_descriptors
                    || free_scalar_member_array_descriptors;
                return;
            }
        }
        current_function_local_structs.push_back(
            {target, struct_t, free_allocatable_array_descriptors,
             free_scalar_member_array_descriptors});
    }

    void register_current_function_local_descriptor_cleanup(
            const std::string &target, const std::string &descriptor) {
        for (const auto &existing: current_function_local_descriptors) {
            if (existing.target == target && existing.descriptor == descriptor) {
                return;
            }
        }
        current_function_local_descriptors.push_back({target, descriptor});
    }

    void register_c_local_variable_cleanup(const ASR::Variable_t &v,
            const std::string &emitted_name, bool require_local_intent=true) {
        if (!is_c || (require_local_intent
                && v.m_intent != ASRUtils::intent_local)) {
            return;
        }
        if (ASRUtils::is_allocatable(v.m_type)
                && ASRUtils::is_array(v.m_type)) {
            register_current_function_local_allocatable_array_cleanup(
                emitted_name);
            ASR::ttype_t *v_type_unwrapped =
                ASRUtils::type_get_past_allocatable_pointer(v.m_type);
            ASR::ttype_t *element_type =
                ASRUtils::extract_type(v_type_unwrapped);
            if (element_type != nullptr
                    && ASRUtils::is_character(*element_type)) {
                register_current_function_local_allocatable_array_string_cleanup(
                    emitted_name);
            } else if (element_type != nullptr
                    && ASR::is_a<ASR::StructType_t>(*element_type)
                    && v.m_type_declaration != nullptr) {
                ASR::symbol_t *struct_sym =
                    ASRUtils::symbol_get_past_external(v.m_type_declaration);
                if (struct_sym != nullptr
                        && ASR::is_a<ASR::Struct_t>(*struct_sym)
                        && c_struct_has_member_cleanup(
                            ASR::down_cast<ASR::Struct_t>(struct_sym))) {
                    register_current_function_local_allocatable_array_struct_cleanup(
                        emitted_name,
                        ASR::down_cast<ASR::Struct_t>(struct_sym));
                }
            }
        } else if (ASRUtils::is_allocatable(v.m_type)
                && !ASRUtils::is_array(v.m_type)) {
            ASR::ttype_t *v_type_unwrapped =
                ASRUtils::type_get_past_allocatable_pointer(v.m_type);
            if (ASRUtils::is_character(*v_type_unwrapped)) {
                register_current_function_local_allocatable_string_cleanup(
                    emitted_name);
            } else if ((!is_c_compiler_generated_temporary_name(emitted_name)
                        || is_c_owned_function_call_struct_temp_name(emitted_name))
                    && ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)
                    && v.m_type_declaration != nullptr) {
                ASR::symbol_t *struct_sym =
                    ASRUtils::symbol_get_past_external(v.m_type_declaration);
                if (struct_sym != nullptr
                        && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                    register_current_function_local_allocatable_struct_cleanup(
                        emitted_name,
                        ASR::down_cast<ASR::Struct_t>(struct_sym),
                        ASRUtils::is_class_type(v_type_unwrapped),
                        is_c_owned_function_call_struct_temp_name(
                            emitted_name));
                }
            } else if (!is_c_compiler_generated_temporary_name(emitted_name)) {
                register_current_function_local_allocatable_scalar_cleanup(
                    emitted_name);
            }
        } else if (!ASRUtils::is_allocatable(v.m_type)
                && !ASRUtils::is_pointer(v.m_type)
                && !ASRUtils::is_array(v.m_type)
                && v.m_storage != ASR::storage_typeType::Parameter
                && v.m_symbolic_value == nullptr) {
            ASR::ttype_t *v_type_unwrapped =
                ASRUtils::type_get_past_allocatable_pointer(v.m_type);
            if (ASRUtils::is_character(*v_type_unwrapped)) {
                register_current_function_local_character_string_cleanup(
                    emitted_name);
            } else if (ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)
                    && !ASRUtils::is_class_type(v_type_unwrapped)
                    && v.m_type_declaration != nullptr) {
                ASR::symbol_t *struct_sym =
                    ASRUtils::symbol_get_past_external(v.m_type_declaration);
                if (struct_sym != nullptr
                        && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                    bool free_member_array_descriptors =
                        should_free_c_local_struct_member_array_descriptors(
                            v, emitted_name);
                    bool free_scalar_member_array_descriptors =
                        should_free_c_local_struct_scalar_member_array_descriptors(
                            v, emitted_name);
                    register_current_function_local_struct_cleanup(
                        get_c_local_struct_cleanup_target(v, emitted_name),
                        ASR::down_cast<ASR::Struct_t>(struct_sym),
                        free_member_array_descriptors,
                        free_scalar_member_array_descriptors);
                }
            }
        }
    }

    ASR::Variable_t *get_c_component_cache_root_var(ASR::expr_t *expr) {
        if (expr == nullptr) {
            return nullptr;
        }
        switch (expr->type) {
            case ASR::exprType::Var: {
                ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(expr)->m_v);
                if (sym != nullptr && ASR::is_a<ASR::Variable_t>(*sym)) {
                    return ASR::down_cast<ASR::Variable_t>(sym);
                }
                return nullptr;
            }
            case ASR::exprType::StructInstanceMember:
                return get_c_component_cache_root_var(
                    ASR::down_cast<ASR::StructInstanceMember_t>(expr)->m_v);
            case ASR::exprType::ArrayPhysicalCast:
                return get_c_component_cache_root_var(
                    ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr)->m_arg);
            case ASR::exprType::GetPointer:
                return get_c_component_cache_root_var(
                    ASR::down_cast<ASR::GetPointer_t>(expr)->m_arg);
            case ASR::exprType::Cast:
                return get_c_component_cache_root_var(
                    ASR::down_cast<ASR::Cast_t>(expr)->m_arg);
            default:
                return nullptr;
        }
    }

    bool is_c_intent_in_component_array_cache_candidate(
            const ASR::StructInstanceMember_t &x) {
        if (!is_c || !ASRUtils::is_array(x.m_type)) {
            return false;
        }
        ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(x.m_m);
        if (member_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*member_sym)) {
            return false;
        }
        ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
        if (!ASRUtils::is_array(member_var->m_type)
                || ASRUtils::is_pointer(member_var->m_type)
                || !is_c_scalarizable_element_type(member_var->m_type)) {
            return false;
        }
        ASR::ttype_t *array_type = ASRUtils::type_get_past_allocatable_pointer(
            member_var->m_type);
        if (array_type == nullptr || !ASR::is_a<ASR::Array_t>(*array_type)) {
            return false;
        }
        ASR::Array_t *array = ASR::down_cast<ASR::Array_t>(array_type);
        if (array->m_physical_type == ASR::array_physical_typeType::FixedSizeArray) {
            return false;
        }
        ASR::Variable_t *root_var = get_c_component_cache_root_var(x.m_v);
        return root_var != nullptr && root_var->m_intent == ASRUtils::intent_in;
    }

    std::string emit_current_function_array_descriptor_cache_decls(
            const ASR::Function_t &x, const std::string &indent) {
        current_function_array_descriptor_cache.clear();
        if (!is_c) {
            return "";
        }
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type && f_type->m_abi == ASR::abiType::BindC) {
            return "";
        }
        std::string decl;
        auto emit_cache_for_array_expr = [&](const std::string &array_expr,
                const std::string &cache_name_base, int n_dims,
                ASR::ttype_t *array_type, bool use_restrict=true) {
            if (n_dims <= 0
                    || current_function_array_descriptor_cache.find(array_expr)
                        != current_function_array_descriptor_cache.end()) {
                return;
            }
            CArrayDescriptorCache cache;
            if (is_c_scalarizable_element_type(array_type)) {
                cache.data = get_unique_local_name(
                    "__lfortran_" + cache_name_base + "_data");
                decl += indent + CUtils::get_c_array_element_type_from_ttype_t(array_type)
                    + " *" + (use_restrict ? " restrict " : " ") + cache.data + " = "
                    + array_expr + "->data;\n";
            }
            cache.offset = get_unique_local_name(
                "__lfortran_" + cache_name_base + "_offset");
            decl += indent + "const int32_t " + cache.offset + " = "
                + array_expr + "->offset;\n";
            for (int j = 0; j < n_dims; j++) {
                std::string dim_prefix = "__lfortran_" + cache_name_base + "_dim"
                    + std::to_string(j + 1);
                std::string lower_bound =
                    get_unique_local_name(dim_prefix + "_lower_bound");
                std::string stride =
                    get_unique_local_name(dim_prefix + "_stride");
                decl += indent + "const int32_t " + lower_bound + " = "
                    + array_expr + "->dims[" + std::to_string(j) + "].lower_bound;\n";
                decl += indent + "const int32_t " + stride + " = "
                    + array_expr + "->dims[" + std::to_string(j) + "].stride;\n";
                cache.lower_bounds.push_back(lower_bound);
                cache.strides.push_back(stride);
            }
            current_function_array_descriptor_cache[array_expr] = cache;
        };
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::expr_t *arg = x.m_args[i];
            if (arg == nullptr || !ASR::is_a<ASR::Var_t>(*arg)) {
                continue;
            }
            ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(arg)->m_v);
            if (arg_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                continue;
            }
            ASR::Variable_t *arg_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
            std::string arg_name = CUtils::get_c_variable_name(*arg_var);
            if (ASRUtils::is_array(arg_var->m_type)
                    && !ASRUtils::is_allocatable(arg_var->m_type)
                    && !ASRUtils::is_pointer(arg_var->m_type)) {
                ASR::dimension_t *dims = nullptr;
                int n_dims = ASRUtils::extract_dimensions_from_ttype(
                    arg_var->m_type, dims);
                emit_cache_for_array_expr(arg_name, arg_name, n_dims, arg_var->m_type);
            }
        }
        struct IntentInComponentArrayCollector:
                public ASR::BaseWalkVisitor<IntentInComponentArrayCollector> {
            BaseCCPPVisitor<StructType> &parent;
            std::vector<ASR::StructInstanceMember_t*> members;
            std::set<ASR::StructInstanceMember_t*> seen;
            IntentInComponentArrayCollector(BaseCCPPVisitor<StructType> &parent):
                parent(parent) {}
            void visit_StructInstanceMember(const ASR::StructInstanceMember_t &x) {
                if (parent.is_c_intent_in_component_array_cache_candidate(x)
                        && seen.insert(const_cast<ASR::StructInstanceMember_t*>(&x)).second) {
                    members.push_back(const_cast<ASR::StructInstanceMember_t*>(&x));
                }
                ASR::BaseWalkVisitor<IntentInComponentArrayCollector>::
                    visit_StructInstanceMember(x);
            }
        };
        IntentInComponentArrayCollector collector(*this);
        for (size_t i = 0; i < x.n_body; i++) {
            collector.visit_stmt(*x.m_body[i]);
        }
        std::string saved_src = src;
        for (ASR::StructInstanceMember_t *member: collector.members) {
            src.clear();
            std::string array_expr = get_struct_instance_member_expr(*member, true);
            std::string cache_name_base = "component_array_"
                + std::to_string(current_function_array_descriptor_cache.size());
            ASR::dimension_t *dims = nullptr;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(member->m_type, dims);
            emit_cache_for_array_expr(array_expr, cache_name_base, n_dims, member->m_type, false);
        }
        src = saved_src;
        return decl;
    }

    const CArrayDescriptorCache *get_current_function_array_descriptor_cache(
            const std::string &arr, int n_args) const {
        if (!is_c) {
            return nullptr;
        }
        auto it = current_function_array_descriptor_cache.find(arr);
        if (it == current_function_array_descriptor_cache.end()
                || static_cast<int>(it->second.lower_bounds.size()) < n_args
                || static_cast<int>(it->second.strides.size()) < n_args) {
            return nullptr;
        }
        return &it->second;
    }

    void clear_c_pow_cache() {
        current_function_pow_cache.clear();
    }

    void visit_expr_without_c_pow_cache(ASR::expr_t &expr) {
        c_pow_cache_suppression_depth++;
        self().visit_expr(expr);
        c_pow_cache_suppression_depth--;
    }

    bool is_c_pow_cache_safe_expr(ASR::expr_t *expr) {
        if (expr == nullptr) {
            return false;
        }
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::IntegerConstant:
            case ASR::exprType::UnsignedIntegerConstant:
            case ASR::exprType::RealConstant:
            case ASR::exprType::LogicalConstant: {
                return true;
            }
            case ASR::exprType::StructInstanceMember: {
                return is_c_pow_cache_safe_expr(
                    ASR::down_cast<ASR::StructInstanceMember_t>(expr)->m_v);
            }
            case ASR::exprType::ArrayItem: {
                ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
                if (!is_c_pow_cache_safe_expr(item->m_v)) {
                    return false;
                }
                for (size_t i = 0; i < item->n_args; i++) {
                    ASR::expr_t *idx_expr = get_array_index_expr(item->m_args[i]);
                    if (idx_expr == nullptr || is_vector_subscript_expr(idx_expr)
                            || !is_c_pow_cache_safe_expr(idx_expr)) {
                        return false;
                    }
                }
                return true;
            }
            case ASR::exprType::Cast: {
                return is_c_pow_cache_safe_expr(ASR::down_cast<ASR::Cast_t>(expr)->m_arg);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_pow_cache_safe_expr(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_pow_cache_safe_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::UnsignedIntegerUnaryMinus: {
                return is_c_pow_cache_safe_expr(
                    ASR::down_cast<ASR::UnsignedIntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return is_c_pow_cache_safe_expr(binop->m_left)
                    && is_c_pow_cache_safe_expr(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return is_c_pow_cache_safe_expr(binop->m_left)
                    && is_c_pow_cache_safe_expr(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return is_c_pow_cache_safe_expr(binop->m_left)
                    && is_c_pow_cache_safe_expr(binop->m_right);
            }
            default: {
                return false;
            }
        }
    }

    bool collect_c_pow_cache_dependencies(ASR::expr_t *expr,
            std::set<ASR::symbol_t*> &deps) {
        if (expr == nullptr) {
            return false;
        }
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var: {
                deps.insert(ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(expr)->m_v));
                return true;
            }
            case ASR::exprType::IntegerConstant:
            case ASR::exprType::UnsignedIntegerConstant:
            case ASR::exprType::RealConstant:
            case ASR::exprType::LogicalConstant: {
                return true;
            }
            case ASR::exprType::StructInstanceMember: {
                return collect_c_pow_cache_dependencies(
                    ASR::down_cast<ASR::StructInstanceMember_t>(expr)->m_v, deps);
            }
            case ASR::exprType::ArrayItem: {
                ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
                if (!collect_c_pow_cache_dependencies(item->m_v, deps)) {
                    return false;
                }
                for (size_t i = 0; i < item->n_args; i++) {
                    ASR::expr_t *idx_expr = get_array_index_expr(item->m_args[i]);
                    if (idx_expr == nullptr || is_vector_subscript_expr(idx_expr)
                            || !collect_c_pow_cache_dependencies(idx_expr, deps)) {
                        return false;
                    }
                }
                return true;
            }
            case ASR::exprType::Cast: {
                return collect_c_pow_cache_dependencies(
                    ASR::down_cast<ASR::Cast_t>(expr)->m_arg, deps);
            }
            case ASR::exprType::RealUnaryMinus: {
                return collect_c_pow_cache_dependencies(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg, deps);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return collect_c_pow_cache_dependencies(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg, deps);
            }
            case ASR::exprType::UnsignedIntegerUnaryMinus: {
                return collect_c_pow_cache_dependencies(
                    ASR::down_cast<ASR::UnsignedIntegerUnaryMinus_t>(expr)->m_arg, deps);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && collect_c_pow_cache_dependencies(binop->m_left, deps)
                    && collect_c_pow_cache_dependencies(binop->m_right, deps);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && collect_c_pow_cache_dependencies(binop->m_left, deps)
                    && collect_c_pow_cache_dependencies(binop->m_right, deps);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && collect_c_pow_cache_dependencies(binop->m_left, deps)
                    && collect_c_pow_cache_dependencies(binop->m_right, deps);
            }
            default: {
                return false;
            }
        }
    }

    void invalidate_c_pow_cache_for_expr(ASR::expr_t *expr) {
        if (!is_c || expr == nullptr || current_function_pow_cache.empty()) {
            return;
        }
        std::set<ASR::symbol_t*> deps;
        if (!collect_c_pow_cache_dependencies(expr, deps) || deps.empty()) {
            clear_c_pow_cache();
            return;
        }
        for (auto it = current_function_pow_cache.begin();
                it != current_function_pow_cache.end();) {
            bool erase = false;
            for (ASR::symbol_t *dep: deps) {
                if (it->second.deps.find(dep) != it->second.deps.end()) {
                    erase = true;
                    break;
                }
            }
            if (erase) {
                it = current_function_pow_cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::string get_c_local_struct_cleanup_target(const ASR::Variable_t &v,
            const std::string &emitted_name) const {
        bool force_value_struct_temp =
            std::string(v.m_name).find("__libasr_created__struct_constructor_") != std::string::npos ||
            std::string(v.m_name).find("temp_struct_var__") != std::string::npos;
        if (force_value_struct_temp) {
            return "(&(" + emitted_name + "))";
        }
        std::string value_var_name = v.m_parent_symtab->get_unique_name(
            emitted_name + "_value");
        return "(&(" + value_var_name + "))";
    }

    bool is_c_compiler_generated_temporary_name(const std::string &name) const {
        return name.rfind("__libasr_created", 0) == 0
            || name.rfind("__libasr__created", 0) == 0
            || name.find("__libasr_created") != std::string::npos
            || name.find("__libasr__created") != std::string::npos;
    }

    bool is_c_owned_function_call_struct_temp_name(const std::string &name) const {
        return name.rfind("__libasr_created__function_call_1_", 0) == 0;
    }

    bool should_register_c_local_struct_descriptor_cleanup(
            const ASR::Variable_t &v, const std::string &emitted_name,
            const std::string &backing_name="") const {
        return !is_c_compiler_generated_temporary_name(std::string(v.m_name))
            && !is_c_compiler_generated_temporary_name(emitted_name)
            && (backing_name.empty()
                || !is_c_compiler_generated_temporary_name(backing_name));
    }

    bool should_free_c_local_struct_member_array_descriptors(
            const ASR::Variable_t &v, const std::string &emitted_name) const {
        std::string variable_name = v.m_name;
        if (is_c_compiler_generated_temporary_name(emitted_name)) {
            return variable_name.rfind("__libasr_created_dummy_variable", 0) == 0
                || variable_name.rfind("__libasr_created_variable", 0) == 0
                || variable_name.rfind("__libasr_created__subroutine_call_", 0) == 0;
        }
        return variable_name.rfind("temp_struct_var__", 0) == 0
            || variable_name == "calc";
    }

    bool should_free_c_local_struct_scalar_member_array_descriptors(
            const ASR::Variable_t &v, const std::string &emitted_name) const {
        return !is_c_compiler_generated_temporary_name(emitted_name)
            || should_free_c_local_struct_member_array_descriptors(v, emitted_name);
    }

    void register_c_intent_out_local_struct_descriptor_cleanup(
            const ASR::Variable_t &actual_var) {
        if (!is_c || actual_var.m_intent != ASRUtils::intent_local
                || ASRUtils::is_allocatable(actual_var.m_type)
                || ASRUtils::is_pointer(actual_var.m_type)
                || ASRUtils::is_array(actual_var.m_type)
                || actual_var.m_storage == ASR::storage_typeType::Parameter
                || actual_var.m_symbolic_value != nullptr
                || actual_var.m_type_declaration == nullptr) {
            return;
        }
        ASR::ttype_t *actual_type_unwrapped =
            ASRUtils::type_get_past_allocatable_pointer(actual_var.m_type);
        if (!ASR::is_a<ASR::StructType_t>(*actual_type_unwrapped)
                || ASRUtils::is_class_type(actual_type_unwrapped)) {
            return;
        }
        std::string emitted_name = CUtils::get_c_variable_name(actual_var);
        if (!should_register_c_local_struct_descriptor_cleanup(
                    actual_var, emitted_name)) {
            return;
        }
        ASR::symbol_t *struct_sym =
            ASRUtils::symbol_get_past_external(actual_var.m_type_declaration);
        if (struct_sym == nullptr || !ASR::is_a<ASR::Struct_t>(*struct_sym)) {
            return;
        }
        register_current_function_local_struct_cleanup(
            get_c_local_struct_cleanup_target(actual_var, emitted_name),
            ASR::down_cast<ASR::Struct_t>(struct_sym), true, false);
    }

    void cache_c_default_allocator(std::string &decl, std::string &body,
            const std::string &indent) {
        if (!is_c) {
            return;
        }
        const std::string allocator_call = "_lfortran_get_default_allocator()";
        if (decl.find(allocator_call) == std::string::npos
                && body.find(allocator_call) == std::string::npos) {
            return;
        }
        const std::string allocator_name = "__lfortran_default_allocator";
        decl = indent + "lfortran_allocator_t* " + allocator_name
            + " = " + allocator_call + ";\n"
            + replace_all_substrings(decl, allocator_call, allocator_name);
        body = replace_all_substrings(body, allocator_call, allocator_name);
    }

    std::string get_final_combined_src(std::string head, std::string unit_src) {
        std::string to_include = "";
        for (auto &s: user_defines) {
            to_include += "#define " + s + "\n";
        }
        for (auto &s: headers) {
            to_include += "#include <" + s + ">\n";
        }
        for (auto &s: user_headers) {
            to_include += "#include \"" + s + "\"\n";
        }
        if( c_ds_api->get_func_decls().size() > 0 ) {
            array_types_decls += "\n" + c_ds_api->get_func_decls() + "\n";
        }
        if( c_utils_functions->get_util_func_decls().size() > 0 ) {
            array_types_decls += "\n" + c_utils_functions->get_util_func_decls() + "\n";
        }
        std::string ds_funcs_defined = "";
        if( c_ds_api->get_generated_code().size() > 0 ) {
            ds_funcs_defined =  "\n" + c_ds_api->get_generated_code() + "\n";
        }
        std::string util_funcs_defined = "";
        if( c_utils_functions->get_generated_code().size() > 0 ) {
            util_funcs_defined =  "\n" + c_utils_functions->get_generated_code() + "\n";
        }
        if( bind_py_utils_functions->get_util_func_decls().size() > 0 ) {
            array_types_decls += "\n" + bind_py_utils_functions->get_util_func_decls() + "\n";
        }
        if( bind_py_utils_functions->get_generated_code().size() > 0 ) {
            util_funcs_defined =  "\n" + bind_py_utils_functions->get_generated_code() + "\n";
        }
        if (is_c) {
            head += "\n#ifndef LFORTRAN_C_BACKEND_CONSTRUCTOR\n"
                "#if defined(__GNUC__) || defined(__clang__)\n"
                "#define LFORTRAN_C_BACKEND_CONSTRUCTOR __attribute__((constructor))\n"
                "#else\n"
                "#define LFORTRAN_C_BACKEND_CONSTRUCTOR\n"
                "#endif\n"
                "#endif\n"
                "#ifndef LFORTRAN_C_BACKEND_WEAK\n"
                "#if defined(__GNUC__) || defined(__clang__)\n"
                "#define LFORTRAN_C_BACKEND_WEAK __attribute__((weak))\n"
                "#else\n"
                "#define LFORTRAN_C_BACKEND_WEAK static\n"
                "#endif\n"
                "#endif\n\n";
        }
        if( is_string_concat_present ) {
            std::string strcat_def = "";
            strcat_def += "    static inline char* strcat_(const char* x, const char* y) {\n";
            strcat_def += "        char* str_tmp = (char*) malloc((strlen(x) + strlen(y) + 2) * sizeof(char));\n";
            strcat_def += "        strcpy(str_tmp, x);\n";
            strcat_def += "        return strcat(str_tmp, y);\n";
            strcat_def += "    }\n\n";
            head += strcat_def;
        }

        // Include dimension_descriptor definition that is used by array types
        if (array_types_decls.size() != 0) {
            array_types_decls = "\nstruct dimension_descriptor\n"
                "{\n    int32_t lower_bound, length, stride;\n};\n" + array_types_decls;
        }

        return to_include + head + array_types_decls + forward_decl_functions + unit_src +
              ds_funcs_defined + util_funcs_defined;
    }

    std::string get_runtime_type_tag_member_name() const {
        return "__lfortran_type_tag";
    }

    std::string get_runtime_type_tag_header_struct_name() const {
        return "__lfortran_runtime_type_header";
    }

    void ensure_runtime_type_tag_header_decl() {
        std::string header_decl = "struct " + get_runtime_type_tag_header_struct_name()
            + "\n{\n    int64_t " + get_runtime_type_tag_member_name() + ";\n};\n";
        if (array_types_decls.find(header_decl) == std::string::npos) {
            array_types_decls = header_decl + array_types_decls;
        }
    }

    void ensure_c_backend_constructor_macro_decl() {
        if (!is_c) {
            return;
        }
        std::string macro_decl = "#ifndef LFORTRAN_C_BACKEND_CONSTRUCTOR\n"
            "#if defined(__GNUC__) || defined(__clang__)\n"
            "#define LFORTRAN_C_BACKEND_CONSTRUCTOR __attribute__((constructor))\n"
            "#else\n"
            "#define LFORTRAN_C_BACKEND_CONSTRUCTOR\n"
            "#endif\n"
            "#endif\n"
            "#ifndef LFORTRAN_C_BACKEND_WEAK\n"
            "#if defined(__GNUC__) || defined(__clang__)\n"
            "#define LFORTRAN_C_BACKEND_WEAK __attribute__((weak))\n"
            "#else\n"
            "#define LFORTRAN_C_BACKEND_WEAK static\n"
            "#endif\n"
            "#endif\n\n";
        if (array_types_decls.find("#ifndef LFORTRAN_C_BACKEND_CONSTRUCTOR\n")
                == std::string::npos) {
            array_types_decls = macro_decl + array_types_decls;
        }
    }

    std::string get_runtime_type_tag_expr(const std::string &expr,
            bool is_pointer_backed) {
        if (is_pointer_backed) {
            ensure_runtime_type_tag_header_decl();
            return "((struct " + get_runtime_type_tag_header_struct_name()
                + "*)(" + expr + "))->" + get_runtime_type_tag_member_name();
        }
        return "(" + expr + ")." + get_runtime_type_tag_member_name();
    }

    std::string get_unique_local_name(const std::string &name,
            bool use_unique_id=true) {
        SymbolTable *scope = current_scope ? current_scope : global_scope;
        LCOMPILERS_ASSERT(scope != nullptr);
        std::set<std::string> &used_names = emitted_local_names[scope];
        std::string unique_name = name;
        if (use_unique_id && !lcompilers_unique_ID_separate_compilation.empty()) {
            unique_name += "_" + lcompilers_unique_ID_separate_compilation;
        }
        int counter = 1;
        while (scope->get_symbol(unique_name) != nullptr
                || used_names.find(unique_name) != used_names.end()) {
            unique_name = name + std::to_string(counter);
            if (use_unique_id && !lcompilers_unique_ID_separate_compilation.empty()) {
                unique_name += "_" + lcompilers_unique_ID_separate_compilation;
            }
            counter++;
        }
        used_names.insert(unique_name);
        return unique_name;
    }

    std::string get_c_array_constant_init_element(ASR::ArrayConstant_t *arr,
            ASR::ttype_t *element_type, size_t index) {
        std::string value = ASRUtils::fetch_ArrayConstant_value(arr, index);
        ASR::ttype_t *element_type_unwrapped = ASRUtils::type_get_past_pointer(
            ASRUtils::type_get_past_allocatable(element_type));
        if (is_c && element_type_unwrapped != nullptr
                && ASR::is_a<ASR::Logical_t>(*element_type_unwrapped)) {
            if (value == ".true.") {
                return "true";
            }
            if (value == ".false.") {
                return "false";
            }
        }
        if (is_c && element_type_unwrapped != nullptr
                && ASRUtils::is_character(*element_type_unwrapped)) {
            ASR::String_t *string_type = ASRUtils::get_string_type(element_type_unwrapped);
            int64_t len = 0;
            if (ASRUtils::extract_value(string_type->m_len, len)) {
                char *data_char = static_cast<char*>(arr->m_data) + index * len;
                std::string raw_value(data_char, len);
                return "\"" + CUtils::escape_c_string_literal(raw_value) + "\"";
            }
        }
        return value;
    }

    size_t get_c_array_constant_storage_index(ASR::ArrayConstant_t *arr,
            ASR::ttype_t *array_type, size_t c_index) {
        ASR::dimension_t *m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(array_type, m_dims);
        if (n_dims <= 1 || arr->m_storage_format != ASR::arraystorageType::RowMajor) {
            return c_index;
        }

        std::vector<int64_t> lengths(n_dims);
        for (size_t i = 0; i < n_dims; i++) {
            int64_t length = 0;
            if (m_dims[i].m_length == nullptr
                    || !ASRUtils::extract_value(
                        ASRUtils::expr_value(m_dims[i].m_length), length)
                    || length <= 0) {
                return c_index;
            }
            lengths[i] = length;
        }

        std::vector<int64_t> subscripts(n_dims);
        size_t remaining = c_index;
        for (size_t i = 0; i < n_dims; i++) {
            subscripts[i] = static_cast<int64_t>(
                remaining % static_cast<size_t>(lengths[i]));
            remaining /= static_cast<size_t>(lengths[i]);
        }

        size_t storage_index = 0;
        for (size_t i = 0; i < n_dims; i++) {
            size_t storage_stride = 1;
            for (size_t j = i + 1; j < n_dims; j++) {
                storage_stride *= static_cast<size_t>(lengths[j]);
            }
            storage_index += static_cast<size_t>(subscripts[i]) * storage_stride;
        }
        return storage_index;
    }

    std::string get_c_array_constant_init_element_for_c_index(
            ASR::ArrayConstant_t *arr, ASR::ttype_t *array_type,
            ASR::ttype_t *element_type, size_t c_index) {
        return get_c_array_constant_init_element(arr, element_type,
            get_c_array_constant_storage_index(arr, array_type, c_index));
    }

    int64_t get_struct_runtime_type_id(ASR::symbol_t *struct_sym) {
        struct_sym = ASRUtils::symbol_get_past_external(struct_sym);
        if (struct_sym == nullptr) {
            return 1;
        }
        uint64_t hash = get_stable_string_hash(CUtils::get_c_symbol_name(struct_sym));
        int64_t type_id = static_cast<int64_t>(hash & 0x7fffffffffffffffULL);
        return type_id == 0 ? 1 : type_id;
    }

    std::string escape_c_string_literal(const std::string &value) const {
        std::string escaped;
        escaped.reserve(value.size());
        for (char ch : value) {
            switch (ch) {
                case '\\': escaped += "\\\\"; break;
                case '\"': escaped += "\\\""; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += ch; break;
            }
        }
        return escaped;
    }

    std::string get_current_indent() const {
        return std::string(indentation_level * indentation_spaces, ' ');
    }

    std::string get_function_pointer_type(const ASR::Function_t &x, bool &has_typevar,
            bool use_void_self_for_tbp_dispatch=false) {
        template_for_Kokkos.clear();
        template_number = 0;
        std::string type_sig;
        has_typevar = false;
        if (x.m_return_var) {
            ASR::Variable_t *return_var = ASRUtils::EXPR2VAR(x.m_return_var);
            has_typevar = ASR::is_a<ASR::TypeParameter_t>(*return_var->m_type);
            type_sig = get_return_var_type(return_var);
        } else {
            type_sig = "void ";
        }
        type_sig += "(*)(";
        bracket_open++;
        for (size_t i = 0; i < x.n_args; i++) {
            if (i == 0 && use_void_self_for_tbp_dispatch) {
                type_sig += "void*";
            } else {
                ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v);
                if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                    has_typevar = true;
                    bracket_open--;
                    return "";
                }
                ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
                if (is_c) {
                    CDeclarationOptions c_decl_options;
                    c_decl_options.pre_initialise_derived_type = false;
                    type_sig += self().convert_variable_decl(*arg, &c_decl_options);
                } else {
                    CPPDeclarationOptions cpp_decl_options;
                    cpp_decl_options.use_static = false;
                    cpp_decl_options.use_templates_for_arrays = true;
                    type_sig += self().convert_variable_decl(*arg, &cpp_decl_options);
                }
                if (ASR::is_a<ASR::TypeParameter_t>(*arg->m_type)) {
                    has_typevar = true;
                    bracket_open--;
                    return "";
                }
            }
            if (i < x.n_args - 1) {
                type_sig += ", ";
            }
        }
        bracket_open--;
        type_sig += ")";
        return type_sig;
    }

    ASR::symbol_t *get_expr_type_declaration_symbol(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return nullptr;
        }
        if (ASR::is_a<ASR::Var_t>(*expr)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(expr)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                return ASR::down_cast<ASR::Variable_t>(sym)->m_type_declaration;
            }
        } else if (ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::StructInstanceMember_t>(expr)->m_m);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                return ASR::down_cast<ASR::Variable_t>(sym)->m_type_declaration;
            }
        } else if (ASR::is_a<ASR::UnionInstanceMember_t>(*expr)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::UnionInstanceMember_t>(expr)->m_m);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                return ASR::down_cast<ASR::Variable_t>(sym)->m_type_declaration;
            }
        }
        return nullptr;
    }

    std::string get_c_concrete_type_from_ttype_t(ASR::ttype_t *type,
            ASR::symbol_t *type_decl=nullptr) {
        if (type == nullptr) {
            return "";
        }
        switch (type->type) {
            case ASR::ttypeType::Pointer: {
                ASR::Pointer_t *ptr_type = ASR::down_cast<ASR::Pointer_t>(type);
                return get_c_concrete_type_from_ttype_t(ptr_type->m_type, type_decl) + "*";
            }
            case ASR::ttypeType::Allocatable: {
                ASR::Allocatable_t *alloc_type = ASR::down_cast<ASR::Allocatable_t>(type);
                return get_c_concrete_type_from_ttype_t(alloc_type->m_type, type_decl);
            }
            case ASR::ttypeType::Array: {
                ASR::Array_t *array_type = ASR::down_cast<ASR::Array_t>(type);
                return get_c_concrete_type_from_ttype_t(array_type->m_type, type_decl);
            }
            case ASR::ttypeType::StructType: {
                ASR::StructType_t *struct_type = ASR::down_cast<ASR::StructType_t>(type);
                if (struct_type->m_is_unlimited_polymorphic) {
                    return "void*";
                }
                type_decl = ASRUtils::symbol_get_past_external(type_decl);
                if (type_decl != nullptr && ASR::is_a<ASR::Struct_t>(*type_decl)) {
                    return "struct " + CUtils::get_c_symbol_name(type_decl);
                }
                return "void*";
            }
            default: {
                return CUtils::get_c_type_from_ttype_t(type, is_c);
            }
        }
    }

    bool struct_derives_from(ASR::Struct_t *struct_t, ASR::symbol_t *base_sym) {
        base_sym = ASRUtils::symbol_get_past_external(base_sym);
        while (struct_t != nullptr) {
            if (reinterpret_cast<ASR::symbol_t*>(struct_t) == base_sym) {
                return true;
            }
            if (struct_t->m_parent == nullptr) {
                break;
            }
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(struct_t->m_parent);
            if (!ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                break;
            }
            struct_t = ASR::down_cast<ASR::Struct_t>(parent_sym);
        }
        return false;
    }

    void collect_descendant_structs(SymbolTable *scope, ASR::symbol_t *base_sym,
            std::vector<ASR::Struct_t*> &derived_structs, std::set<uint64_t> &seen) {
        if (scope == nullptr) {
            return;
        }
        for (auto &item: scope->get_scope()) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(item.second);
            uint64_t key = get_hash(reinterpret_cast<ASR::asr_t*>(sym));
            if (!seen.insert(key).second) {
                continue;
            }
            if (ASR::is_a<ASR::Struct_t>(*sym)) {
                ASR::Struct_t *struct_t = ASR::down_cast<ASR::Struct_t>(sym);
                if (struct_t->m_parent != nullptr && struct_derives_from(struct_t, base_sym)) {
                    derived_structs.push_back(struct_t);
                }
                collect_descendant_structs(struct_t->m_symtab, base_sym, derived_structs, seen);
            } else if (ASR::is_a<ASR::Module_t>(*sym)) {
                collect_descendant_structs(ASR::down_cast<ASR::Module_t>(sym)->m_symtab,
                    base_sym, derived_structs, seen);
            } else if (ASR::is_a<ASR::Function_t>(*sym)) {
                collect_descendant_structs(ASR::down_cast<ASR::Function_t>(sym)->m_symtab,
                    base_sym, derived_structs, seen);
            } else if (ASR::is_a<ASR::Program_t>(*sym)) {
                collect_descendant_structs(ASR::down_cast<ASR::Program_t>(sym)->m_symtab,
                    base_sym, derived_structs, seen);
            } else if (ASR::is_a<ASR::AssociateBlock_t>(*sym)) {
                collect_descendant_structs(ASR::down_cast<ASR::AssociateBlock_t>(sym)->m_symtab,
                    base_sym, derived_structs, seen);
            } else if (ASR::is_a<ASR::Block_t>(*sym)) {
                collect_descendant_structs(ASR::down_cast<ASR::Block_t>(sym)->m_symtab,
                    base_sym, derived_structs, seen);
            }
        }
    }

    ASR::StructMethodDeclaration_t* find_concrete_struct_method(ASR::Struct_t *struct_t,
            const std::string &method_name) {
        while (struct_t != nullptr) {
            ASR::symbol_t *sym = struct_t->m_symtab->resolve_symbol(method_name);
            if (sym != nullptr) {
                sym = ASRUtils::symbol_get_past_external(sym);
                if (ASR::is_a<ASR::StructMethodDeclaration_t>(*sym)) {
                    ASR::StructMethodDeclaration_t *method =
                        ASR::down_cast<ASR::StructMethodDeclaration_t>(sym);
                    if (!method->m_is_deferred) {
                        return method;
                    }
                }
            }
            if (struct_t->m_parent == nullptr) {
                break;
            }
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(struct_t->m_parent);
            if (!ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                break;
            }
            struct_t = ASR::down_cast<ASR::Struct_t>(parent_sym);
        }
        return nullptr;
    }

    ASR::StructMethodDeclaration_t* find_struct_method_by_proc_or_name(
            ASR::Struct_t *struct_t, ASR::symbol_t *proc_sym, const std::string &proc_name) {
        proc_sym = ASRUtils::symbol_get_past_external(proc_sym);
        while (struct_t != nullptr) {
            for (auto &item: struct_t->m_symtab->get_scope()) {
                ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(item.second);
                if (!ASR::is_a<ASR::StructMethodDeclaration_t>(*sym)) {
                    continue;
                }
                ASR::StructMethodDeclaration_t *method =
                    ASR::down_cast<ASR::StructMethodDeclaration_t>(sym);
                ASR::symbol_t *method_proc = ASRUtils::symbol_get_past_external(method->m_proc);
                if (method_proc == proc_sym || std::string(method->m_name) == proc_name) {
                    return method;
                }
            }
            if (struct_t->m_parent == nullptr) {
                break;
            }
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(struct_t->m_parent);
            if (!ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                break;
            }
            struct_t = ASR::down_cast<ASR::Struct_t>(parent_sym);
        }
        return nullptr;
    }

    bool find_struct_method_for_proc(SymbolTable *scope, ASR::symbol_t *proc_sym,
            ASR::Struct_t *&owner_struct, ASR::StructMethodDeclaration_t *&method,
            std::set<uint64_t> &seen) {
        if (scope == nullptr || proc_sym == nullptr) {
            return false;
        }
        proc_sym = ASRUtils::symbol_get_past_external(proc_sym);
        for (auto &item: scope->get_scope()) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(item.second);
            uint64_t key = get_hash(reinterpret_cast<ASR::asr_t*>(sym));
            if (!seen.insert(key).second) {
                continue;
            }
            if (ASR::is_a<ASR::Struct_t>(*sym)) {
                ASR::Struct_t *struct_t = ASR::down_cast<ASR::Struct_t>(sym);
                for (auto &member_item : struct_t->m_symtab->get_scope()) {
                    ASR::symbol_t *member_sym =
                        ASRUtils::symbol_get_past_external(member_item.second);
                    if (!ASR::is_a<ASR::StructMethodDeclaration_t>(*member_sym)) {
                        continue;
                    }
                    ASR::StructMethodDeclaration_t *candidate =
                        ASR::down_cast<ASR::StructMethodDeclaration_t>(member_sym);
                    ASR::symbol_t *candidate_proc =
                        ASRUtils::symbol_get_past_external(candidate->m_proc);
                    if (candidate_proc == proc_sym) {
                        owner_struct = struct_t;
                        method = candidate;
                        return true;
                    }
                }
                if (find_struct_method_for_proc(struct_t->m_symtab, proc_sym,
                        owner_struct, method, seen)) {
                    return true;
                }
            } else if (ASR::is_a<ASR::Module_t>(*sym)) {
                if (find_struct_method_for_proc(
                        ASR::down_cast<ASR::Module_t>(sym)->m_symtab,
                        proc_sym, owner_struct, method, seen)) {
                    return true;
                }
            } else if (ASR::is_a<ASR::Function_t>(*sym)) {
                if (find_struct_method_for_proc(
                        ASR::down_cast<ASR::Function_t>(sym)->m_symtab,
                        proc_sym, owner_struct, method, seen)) {
                    return true;
                }
            } else if (ASR::is_a<ASR::Program_t>(*sym)) {
                if (find_struct_method_for_proc(
                        ASR::down_cast<ASR::Program_t>(sym)->m_symtab,
                        proc_sym, owner_struct, method, seen)) {
                    return true;
                }
            } else if (ASR::is_a<ASR::AssociateBlock_t>(*sym)) {
                if (find_struct_method_for_proc(
                        ASR::down_cast<ASR::AssociateBlock_t>(sym)->m_symtab,
                        proc_sym, owner_struct, method, seen)) {
                    return true;
                }
            } else if (ASR::is_a<ASR::Block_t>(*sym)) {
                if (find_struct_method_for_proc(
                        ASR::down_cast<ASR::Block_t>(sym)->m_symtab,
                        proc_sym, owner_struct, method, seen)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool get_concrete_struct_method_binding(ASR::symbol_t *proc_sym,
            ASR::Struct_t *&owner_struct, ASR::StructMethodDeclaration_t *&method) {
        std::set<uint64_t> seen;
        return find_struct_method_for_proc(global_scope, proc_sym, owner_struct, method, seen);
    }

    std::string get_c_tbp_force_link_anchor_name(ASR::Struct_t *owner_struct,
            ASR::StructMethodDeclaration_t *method) {
        return "__lfortran_force_link_tbp_"
            + CUtils::get_c_symbol_name(reinterpret_cast<ASR::symbol_t*>(owner_struct))
            + "__" + CUtils::sanitize_c_identifier(method->m_name);
    }

    bool can_emit_c_tbp_registration_wrapper(ASR::Function_t *proc) {
        if (proc == nullptr || proc->n_args == 0) {
            return false;
        }
        bool has_typevar = false;
        return !get_function_pointer_type(*proc, has_typevar, true).empty() && !has_typevar;
    }

    void collect_direct_concrete_struct_methods(ASR::Struct_t *struct_t,
            std::vector<ASR::StructMethodDeclaration_t*> &methods) {
        if (struct_t == nullptr || struct_t->m_symtab == nullptr) {
            return;
        }
        std::set<std::string> seen_names;
        for (auto &item: struct_t->m_symtab->get_scope()) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(item.second);
            if (!ASR::is_a<ASR::StructMethodDeclaration_t>(*sym)) {
                continue;
            }
            ASR::StructMethodDeclaration_t *method =
                ASR::down_cast<ASR::StructMethodDeclaration_t>(sym);
            if (method->m_is_deferred || method->m_proc == nullptr) {
                continue;
            }
            ASR::symbol_t *proc = ASRUtils::symbol_get_past_external(method->m_proc);
            if (!ASR::is_a<ASR::Function_t>(*proc)) {
                continue;
            }
            if (!can_emit_c_tbp_registration_wrapper(ASR::down_cast<ASR::Function_t>(proc))) {
                continue;
            }
            if (!seen_names.insert(std::string(method->m_name)).second) {
                continue;
            }
            methods.push_back(method);
        }
    }

    std::string emit_c_tbp_force_link_calls(ASR::Struct_t *owner_struct,
            ASR::StructMethodDeclaration_t *base_method) {
        if (owner_struct == nullptr || base_method == nullptr) {
            return "";
        }
        std::vector<ASR::Struct_t*> candidate_structs;
        candidate_structs.push_back(owner_struct);
        std::set<uint64_t> seen_descendants;
        collect_descendant_structs(global_scope,
            reinterpret_cast<ASR::symbol_t*>(owner_struct),
            candidate_structs, seen_descendants);
        std::set<std::string> seen_anchors;
        std::string force_link_src;
        for (ASR::Struct_t *candidate_struct: candidate_structs) {
            ASR::StructMethodDeclaration_t *concrete_method =
                find_concrete_struct_method(candidate_struct, base_method->m_name);
            if (concrete_method == nullptr || concrete_method->m_is_deferred
                    || concrete_method->m_proc == nullptr) {
                continue;
            }
            ASR::symbol_t *proc_sym = ASRUtils::symbol_get_past_external(
                concrete_method->m_proc);
            if (!ASR::is_a<ASR::Function_t>(*proc_sym)) {
                continue;
            }
            if (!can_emit_c_tbp_registration_wrapper(
                    ASR::down_cast<ASR::Function_t>(proc_sym))) {
                continue;
            }
            ASR::Struct_t *method_owner = candidate_struct;
            ASR::StructMethodDeclaration_t *method_for_anchor = concrete_method;
            if (!get_concrete_struct_method_binding(proc_sym, method_owner,
                    method_for_anchor)) {
                method_owner = candidate_struct;
                method_for_anchor = concrete_method;
            }
            std::string anchor_name = get_c_tbp_force_link_anchor_name(
                method_owner, method_for_anchor);
            if (!seen_anchors.insert(anchor_name).second) {
                continue;
            }
            force_link_src += get_current_indent() + "extern void "
                + anchor_name + "(void);\n"
                + get_current_indent() + anchor_name + "();\n";
        }
        return force_link_src;
    }

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        global_scope = x.m_symtab;
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LCOMPILERS_ASSERT(x.n_items == 0);
        std::string unit_src = "";
        indentation_level = 0;
        indentation_spaces = 4;
        c_ds_api->set_indentation(indentation_level + 1, indentation_spaces);
        c_ds_api->set_global_scope(global_scope);

        std::string headers =
R"(#include <stdio.h>
#include <assert.h>
#include <complex.h>
#include <lfortran_intrinsics.h>
)";
        unit_src += headers;

        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order
                = ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_symtab->get_scope().find(item)
                    != x.m_symtab->get_scope().end());
                if (startswith(item, "lfortran_intrinsic")) {
                    ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                    self().visit_symbol(*mod);
                    unit_src += src;
                }
            }
        }

        // Process procedures first:
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                self().visit_symbol(*item.second);
                unit_src += src;
            }
        }

        // Then do all the modules in the right order
        std::vector<std::string> build_order
            = ASRUtils::determine_module_dependencies(x);
        for (auto &item : build_order) {
            LCOMPILERS_ASSERT(x.m_symtab->get_scope().find(item)
                != x.m_symtab->get_scope().end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                self().visit_symbol(*mod);
                unit_src += src;
            }
        }

        // Then the main program:
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                self().visit_symbol(*item.second);
                unit_src += src;
            }
        }

        src = unit_src;
    }

    std::string check_tmp_buffer() {
        std::string ret = "";
        if (bracket_open == 0 && !tmp_buffer_src.empty()) {
            for (auto &s: tmp_buffer_src) ret += s;
            tmp_buffer_src.clear();
        }
        return ret;
    }

    std::string drain_tmp_buffer() {
        std::string ret = "";
        for (auto &s: tmp_buffer_src) ret += s;
        tmp_buffer_src.clear();
        return ret;
    }

    std::string extract_stmt_setup_from_expr(std::string &expr) {
        std::string setup;
        size_t expr_end = expr.find_last_not_of(" \t\r\n");
        if (expr_end == std::string::npos) {
            return setup;
        }
        size_t split_at = expr.rfind('\n', expr_end);
        if (split_at == std::string::npos) {
            return setup;
        }
        std::string trailing_expr = expr.substr(split_at + 1, expr_end - split_at);
        size_t trailing_begin = trailing_expr.find_first_not_of(" \t\r\n");
        if (trailing_begin == std::string::npos) {
            return setup;
        }
        size_t trailing_end = trailing_expr.find_last_not_of(" \t\r\n");
        trailing_expr = trailing_expr.substr(
            trailing_begin, trailing_end - trailing_begin + 1);
        if (trailing_expr.empty()) {
            return setup;
        }
        setup = expr.substr(0, split_at + 1);
        expr = trailing_expr;
        return setup;
    }

    bool is_allocating_string_expr(ASR::expr_t *expr) {
        if (expr && ASR::is_a<ASR::BitCast_t>(*expr)) {
            ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(expr);
            ASR::expr_t *source = bitcast->m_value ? bitcast->m_value : bitcast->m_source;
            ASR::ttype_t *target_type = bitcast->m_type;
            ASR::ttype_t *source_type = source ? ASRUtils::expr_type(source) : nullptr;
            if (source && ASR::is_a<ASR::ArrayItem_t>(*source)) {
                ASR::ArrayItem_t *array_item = ASR::down_cast<ASR::ArrayItem_t>(source);
                source_type = ASRUtils::type_get_past_array(
                    ASRUtils::expr_type(array_item->m_v));
            }
            if (target_type && source_type
                    && ASRUtils::is_character(*target_type)
                    && (ASRUtils::is_integer(*source_type)
                        || ASRUtils::is_unsigned_integer(*source_type))) {
                return true;
            }
        }
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::StringSection:
            case ASR::exprType::StringItem:
            case ASR::exprType::StringRepeat:
            case ASR::exprType::StringConcat:
            case ASR::exprType::StringFormat:
            case ASR::exprType::StringChr:
                return true;
            case ASR::exprType::IntrinsicElementalFunction: {
                ASR::IntrinsicElementalFunction_t *ief =
                    ASR::down_cast<ASR::IntrinsicElementalFunction_t>(expr);
                return ief->m_intrinsic_id == static_cast<int64_t>(
                        ASRUtils::IntrinsicElementalFunctions::Char)
                    || ief->m_intrinsic_id == static_cast<int64_t>(
                        ASRUtils::IntrinsicElementalFunctions::Repeat);
            }
            case ASR::exprType::StringPhysicalCast:
                return is_allocating_string_expr(
                    ASR::down_cast<ASR::StringPhysicalCast_t>(expr)->m_arg);
            default:
                return false;
        }
    }

    std::string get_string_length_expr(ASR::expr_t *expr, const std::string &expr_src,
            std::string &setup) {
        ASR::String_t *str_type = ASRUtils::get_string_type(expr);
        if (str_type && str_type->m_len) {
            self().visit_expr(*str_type->m_len);
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(src);
            return src;
        }
        return get_c_string_runtime_length_expr(expr_src);
    }

    std::string get_c_string_runtime_length_expr(const std::string &expr_src) {
        return "((" + expr_src + ") != NULL ? _lfortran_str_len(" + expr_src + ") : 0)";
    }

    bool try_materialize_c_intrinsic_string_expr(ASR::expr_t *expr,
            std::string &expr_src, std::string &expr_len,
            std::string &setup, std::string &cleanup) {
        if (!is_c) {
            return false;
        }
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::IntrinsicElementalFunction_t *ief =
            ASR::is_a<ASR::IntrinsicElementalFunction_t>(*expr)
                ? ASR::down_cast<ASR::IntrinsicElementalFunction_t>(expr) : nullptr;
        ASR::StringChr_t *string_chr =
            ASR::is_a<ASR::StringChr_t>(*expr)
                ? ASR::down_cast<ASR::StringChr_t>(expr) : nullptr;
        ASR::StringRepeat_t *string_repeat =
            ASR::is_a<ASR::StringRepeat_t>(*expr)
                ? ASR::down_cast<ASR::StringRepeat_t>(expr) : nullptr;
        std::string indent(indentation_level * indentation_spaces, ' ');
        if (string_chr || (ief && ief->m_intrinsic_id == static_cast<int64_t>(
                ASRUtils::IntrinsicElementalFunctions::Char))) {
            ASR::expr_t *char_arg = string_chr ? string_chr->m_arg : ief->m_args[0];
            visit_expr_without_c_pow_cache(*char_arg);
            std::string char_value = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(char_value);
            std::string tmp_name = get_unique_local_name("__lfortran_string_tmp");
            setup += indent + "char* " + tmp_name
                + " = _lfortran_str_chr_alloc(_lfortran_get_default_allocator(), "
                + char_value + ");\n";
            expr_src = tmp_name;
            expr_len = "1";
            cleanup += indent + "_lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + tmp_name + ");\n";
            return true;
        }
        if (string_repeat || (ief && ief->m_intrinsic_id == static_cast<int64_t>(
                ASRUtils::IntrinsicElementalFunctions::Repeat))) {
            ASR::expr_t *repeat_left = string_repeat ? string_repeat->m_left : ief->m_args[0];
            ASR::expr_t *repeat_right = string_repeat ? string_repeat->m_right : ief->m_args[1];
            std::string left_src;
            std::string left_len;
            std::string left_setup;
            std::string left_cleanup;
            if (!try_materialize_c_intrinsic_string_expr(repeat_left,
                    left_src, left_len, left_setup, left_cleanup)) {
                visit_expr_without_c_pow_cache(*repeat_left);
                left_src = src;
                left_setup += drain_tmp_buffer();
                left_setup += extract_stmt_setup_from_expr(left_src);
                left_len = get_string_length_expr(repeat_left, left_src, left_setup);
            }
            visit_expr_without_c_pow_cache(*repeat_right);
            std::string repeat_count = src;
            std::string repeat_setup = drain_tmp_buffer();
            repeat_setup += extract_stmt_setup_from_expr(repeat_count);
            std::string tmp_name = get_unique_local_name("__lfortran_string_tmp");
            setup += left_setup;
            setup += repeat_setup;
            setup += indent + "char* " + tmp_name
                + " = _lfortran_strrepeat_c_len_alloc(_lfortran_get_default_allocator(), "
                + left_src + ", " + left_len + ", " + repeat_count + ");\n";
            setup += left_cleanup;
            expr_src = tmp_name;
            expr_len = "((" + left_len + ") * (" + repeat_count + "))";
            cleanup += indent + "_lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + tmp_name + ");\n";
            return true;
        }
        return false;
    }

    void materialize_allocating_string_expr(ASR::expr_t *expr, std::string &expr_src,
            std::string &expr_len, std::string &setup, std::string &cleanup) {
        if (!is_c || !is_allocating_string_expr(expr)) {
            expr_len = get_string_length_expr(expr, expr_src, setup);
            return;
        }
        if (try_materialize_c_intrinsic_string_expr(
                expr, expr_src, expr_len, setup, cleanup)) {
            return;
        }
        if (ASR::is_a<ASR::BitCast_t>(*expr)) {
            ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(expr);
            ASR::expr_t *source = bitcast->m_value ? bitcast->m_value : bitcast->m_source;
            ASR::ttype_t *target_type = bitcast->m_type;
            ASR::ttype_t *source_type = source ? ASRUtils::expr_type(source) : nullptr;
            if (source && ASR::is_a<ASR::ArrayItem_t>(*source)) {
                ASR::ArrayItem_t *array_item = ASR::down_cast<ASR::ArrayItem_t>(source);
                source_type = ASRUtils::type_get_past_array(
                    ASRUtils::expr_type(array_item->m_v));
            }
            if (target_type && source_type
                    && ASRUtils::is_character(*target_type)
                    && (ASRUtils::is_integer(*source_type)
                        || ASRUtils::is_unsigned_integer(*source_type))) {
                int64_t fixed_len = -1;
                ASR::String_t *str_type = ASRUtils::get_string_type(target_type);
                if (str_type && str_type->m_len
                        && ASRUtils::extract_value(str_type->m_len, fixed_len)) {
                    expr_len = std::to_string(fixed_len);
                } else {
                    expr_len = "sizeof(" + CUtils::get_c_type_from_ttype_t(source_type)
                        + ")";
                }
                std::string tmp_name = get_unique_local_name("__lfortran_string_tmp");
                setup += std::string(indentation_level * indentation_spaces, ' ')
                    + "char* " + tmp_name + " = " + expr_src + ";\n";
                expr_src = tmp_name;
                cleanup += std::string(indentation_level * indentation_spaces, ' ')
                    + "_lfortran_free_alloc(_lfortran_get_default_allocator(), "
                    + tmp_name + ");\n";
                return;
            }
        }
        std::string tmp_name = get_unique_local_name("__lfortran_string_tmp");
        setup += std::string(indentation_level * indentation_spaces, ' ')
            + "char* " + tmp_name + " = " + expr_src + ";\n";
        expr_src = tmp_name;
        expr_len = get_c_string_runtime_length_expr(expr_src);
        cleanup += std::string(indentation_level * indentation_spaces, ' ')
            + "_lfortran_free_alloc(_lfortran_get_default_allocator(), "
            + tmp_name + ");\n";
    }

    bool try_get_unit_step_string_section_view(ASR::expr_t *expr,
            std::string &view_src, std::string &view_len, std::string &setup) {
        expr = unwrap_c_lvalue_expr(expr);
        if (!is_c || expr == nullptr) {
            return false;
        }
        if (ASR::is_a<ASR::StringItem_t>(*expr)) {
            ASR::StringItem_t *si = ASR::down_cast<ASR::StringItem_t>(expr);
            self().visit_expr(*si->m_arg);
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(src);
            std::string base = src;
            self().visit_expr(*si->m_idx);
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(src);
            view_src = "(" + base + " + ((" + src + ") - 1))";
            view_len = "1";
            return true;
        }
        if (!ASR::is_a<ASR::StringSection_t>(*expr)) {
            return false;
        }
        ASR::StringSection_t *ss = ASR::down_cast<ASR::StringSection_t>(expr);
        std::string step = "1";
        if (ss->m_step) {
            self().visit_expr(*ss->m_step);
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(src);
            step = src;
        }
        if (step != "1") {
            return false;
        }

        self().visit_expr(*ss->m_arg);
        setup += drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(src);
        std::string base = src;
        std::string start0 = "0";
        if (ss->m_start) {
            self().visit_expr(*ss->m_start);
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(src);
            start0 = "((" + src + ") - 1)";
        }
        view_src = "(" + base + " + (" + start0 + "))";

        if (ss->m_end) {
            self().visit_expr(*ss->m_end);
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(src);
            view_len = "((" + src + ") - (" + start0 + "))";
        } else {
            std::string base_len = get_string_length_expr(ss->m_arg, base, setup);
            view_len = "((" + base_len + ") - (" + start0 + "))";
        }
        return true;
    }

    bool try_get_c_scalar_string_call_arg_view(ASR::expr_t *expr,
            ASR::Variable_t *param, ASR::ttype_t *param_type,
            std::string &arg_src, std::string &arg_setup) {
        if (!is_c || param == nullptr || param_type == nullptr
                || param->m_intent != ASRUtils::intent_in
                || ASRUtils::is_allocatable(param_type)
                || ASRUtils::is_pointer(param_type)
                || ASRUtils::is_array(param_type)) {
            return false;
        }
        ASR::ttype_t *param_type_unwrapped =
            ASRUtils::type_get_past_allocatable_pointer(param_type);
        if (param_type_unwrapped == nullptr
                || !ASRUtils::is_character(*param_type_unwrapped)) {
            return false;
        }
        std::string arg_len;
        return try_get_unit_step_string_section_view(
            expr, arg_src, arg_len, arg_setup);
    }

    bool looks_like_non_expression_stmt(const std::string &expr) {
        auto trim_ws = [](const std::string &text) -> std::string {
            size_t begin = text.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos) {
                return "";
            }
            size_t end = text.find_last_not_of(" \t\r\n");
            return text.substr(begin, end - begin + 1);
        };
        std::string trimmed = trim_ws(expr);
        if (trimmed.empty()) {
            return false;
        }
        if (trimmed.find('\n') != std::string::npos) {
            return true;
        }
        if (trimmed == "{" || trimmed == "}") {
            return true;
        }
        if (endswith(trimmed, ";")) {
            return true;
        }
        static const std::vector<std::string> stmt_prefixes = {
            "struct ", "for ", "if ", "while ", "switch ", "return ",
            "int ", "int32_t ", "int64_t ", "float ", "double ",
            "bool ", "char "
        };
        for (const auto &prefix : stmt_prefixes) {
            if (startswith(trimmed, prefix)) {
                return true;
            }
        }
        return false;
    }

    bool extract_trailing_stmt_expr_from_setup(std::string &setup, std::string &expr) {
        auto trim_ws = [](const std::string &text) -> std::string {
            size_t begin = text.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos) {
                return "";
            }
            size_t end = text.find_last_not_of(" \t\r\n");
            return text.substr(begin, end - begin + 1);
        };
        size_t setup_end = setup.find_last_not_of(" \t\r\n");
        if (setup_end == std::string::npos) {
            return false;
        }
        size_t line_start = setup.rfind('\n', setup_end);
        line_start = (line_start == std::string::npos) ? 0 : line_start + 1;
        std::string candidate = trim_ws(setup.substr(line_start, setup_end - line_start + 1));
        if (candidate.empty() || !endswith(candidate, ";")) {
            return false;
        }
        candidate.pop_back();
        candidate = trim_ws(candidate);
        if (candidate.empty() || looks_like_non_expression_stmt(candidate)) {
            return false;
        }
        setup.erase(line_start);
        expr = candidate;
        return true;
    }

    void visit_Module(const ASR::Module_t &x) {
        if (x.m_intrinsic) {
            intrinsic_module = true;
        } else {
            intrinsic_module = false;
        }

        std::string contains;

        // Declare the global variables that are imported from the module
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
            if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
                std::string decl = self().convert_variable_decl(*v);
                decl = check_tmp_buffer() + decl;
                bool used_define_for_const = (v->m_storage == ASR::storage_typeType::Parameter &&
                        v->m_intent == ASRUtils::intent_local);
                if (used_define_for_const) {
                    contains += decl + "\n";
                    continue;
                }
                if (v->m_value) {
                    self().visit_expr(*v->m_value);
                    decl += " = " + src;
                }
                decl += ";\n\n";
                contains += decl;
            }
        }

        // Topologically sort all module functions
        // and then define them in the right order
        std::vector<std::string> func_order = ASRUtils::determine_function_definition_order(x.m_symtab);

        // Generate the bodies of subroutines
        for (auto &item : func_order) {
            ASR::symbol_t* sym = x.m_symtab->get_symbol(item);
            if( !sym ) {
                continue ;
            }
            ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(sym);
            self().visit_Function(*s);
            contains += src;
        }

        src = contains;
        intrinsic_module = false;
    }

    void visit_Program(const ASR::Program_t &x) {
        // Generate code for nested subroutines and functions first:
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        std::string contains;
        std::map<std::string, std::vector<std::string>> struct_dep_graph;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Struct_t>(*item.second) ||
                    ASR::is_a<ASR::Enum_t>(*item.second) ||
                    ASR::is_a<ASR::Union_t>(*item.second)) {
                std::vector<std::string> struct_deps_vec;
                std::pair<char**, size_t> struct_deps_ptr = ASRUtils::symbol_dependencies(item.second);
                for (size_t i = 0; i < struct_deps_ptr.second; i++) {
                    struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                }
                struct_dep_graph[item.first] = struct_deps_vec;
            }
        }
        std::vector<std::string> struct_deps = ASRUtils::order_deps(struct_dep_graph);
        for (auto &item : struct_deps) {
            ASR::symbol_t *struct_sym = x.m_symtab->get_symbol(item);
            if (struct_sym == nullptr) {
                continue;
            }
            if (!(ASR::is_a<ASR::Struct_t>(*struct_sym) ||
                    ASR::is_a<ASR::Enum_t>(*struct_sym) ||
                    ASR::is_a<ASR::Union_t>(*struct_sym))) {
                continue;
            }
            self().visit_symbol(*struct_sym);
        }
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
                contains += src;
            }
        }

        // Generate code for the main program
        indentation_level += 1;
        std::string indent1(indentation_level*indentation_spaces, ' ');
        indentation_level += 1;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string decl;
        clear_current_function_cleanup_state();
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
            if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
                std::string emitted_name = CUtils::get_c_variable_name(*v);
                std::string decl_tmp = self().convert_variable_decl(*v);
                std::string d = decl_tmp + ";\n";
                decl += check_tmp_buffer() + d;
                if (!decl_tmp.empty()) {
                    register_c_local_variable_cleanup(*v, emitted_name, false);
                }
            }
        }

        std::string body;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            body += src;
        }

        body += emit_current_function_heap_array_cleanup(indent1);
        cache_c_default_allocator(decl, body, indent1);

        src = contains
                + "int main(int argc, char* argv[])\n{\n"
                + decl + body
                + indent1 + "return 0;\n}\n";
        clear_current_function_cleanup_state();
        indentation_level -= 2;
        current_scope = current_scope_copy;
    }

    void visit_BlockCall(const ASR::BlockCall_t &x) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Block_t>(*x.m_m));
        ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(x.m_m);
        std::string decl, body, cleanup;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string open_paranthesis = indent + "do {\n";
        std::string close_paranthesis = indent + "} while (0);\n";
        if (x.m_label != -1) {
            std::string b_name;
            if (gotoid2name.find(x.m_label) != gotoid2name.end()) {
                b_name = gotoid2name[x.m_label];
            } else {
                b_name = "__" +std::to_string(x.m_label);
            }
            open_paranthesis = indent + b_name + ": do {\n";
        }
        indent += std::string(indentation_spaces, ' ');
        indentation_level += 1;
        SymbolTable* current_scope_copy = current_scope;
        current_scope = block->m_symtab;
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(block->m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = block->m_symtab->get_symbol(item);
            if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
                std::string d = indent + self().convert_variable_decl(*v) + ";\n";
                decl += check_tmp_buffer() + d;
            }
        }
        for (size_t i=0; i<block->n_body; i++) {
            self().visit_stmt(*block->m_body[i]);
            body += src;
        }
        cleanup = emit_c_scope_compiler_return_slot_cleanup(block->m_symtab, indent)
            + emit_c_scope_allocatable_array_cleanup(block->m_symtab, indent);
        decl += check_tmp_buffer();
        src = open_paranthesis + decl + body + cleanup + close_paranthesis;
        indentation_level -= 1;
        current_scope = current_scope_copy;
    }

    void visit_AssociateBlockCall(const ASR::AssociateBlockCall_t &x) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::AssociateBlock_t>(*x.m_m));
        ASR::AssociateBlock_t* associate_block = ASR::down_cast<ASR::AssociateBlock_t>(x.m_m);
        std::string decl, body, cleanup;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string open_paranthesis = indent + "do {\n";
        std::string close_paranthesis = indent + "} while (0);\n";
        indent += std::string(indentation_spaces, ' ');
        indentation_level += 1;
        SymbolTable* current_scope_copy = current_scope;
        current_scope = associate_block->m_symtab;
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(associate_block->m_symtab);
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = associate_block->m_symtab->get_symbol(item);
            if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
                std::string d = indent + self().convert_variable_decl(*v) + ";\n";
                decl += check_tmp_buffer() + d;
            }
        }
        for (size_t i = 0; i < associate_block->n_body; i++) {
            self().visit_stmt(*associate_block->m_body[i]);
            body += src;
        }
        cleanup = emit_c_scope_compiler_return_slot_cleanup(
            associate_block->m_symtab, indent)
            + emit_c_scope_allocatable_array_cleanup(associate_block->m_symtab,
                indent);
        decl += check_tmp_buffer();
        src = open_paranthesis + decl + body + cleanup + close_paranthesis;
        indentation_level -= 1;
        current_scope = current_scope_copy;
    }

    std::string get_return_var_type(ASR::Variable_t* return_var) {
        std::string sub;
        bool is_array = ASRUtils::is_array(return_var->m_type);
        if (ASR::is_a<ASR::Pointer_t>(*return_var->m_type)) {
            ASR::Pointer_t* ptr_type = ASR::down_cast<ASR::Pointer_t>(return_var->m_type);
            ASR::ttype_t *pointee_type = ptr_type->m_type;
            if (pointee_type != nullptr
                    && !ASRUtils::is_array(pointee_type)
                    && ASRUtils::is_character(
                        *ASRUtils::type_get_past_allocatable_pointer(pointee_type))) {
                sub = "char* ";
            } else {
                std::string pointer_type_str = CUtils::get_c_type_from_ttype_t(pointee_type);
                sub = pointer_type_str + "* ";
            }
        } else if (ASRUtils::is_integer(*return_var->m_type)) {
            int kind = ASRUtils::extract_kind_from_ttype_t(return_var->m_type);
            if (is_array) {
                sub = "struct i" + std::to_string(kind * 8) + "* ";
            } else {
                sub = "int" + std::to_string(kind * 8) + "_t ";
            }
        } else if (ASRUtils::is_unsigned_integer(*return_var->m_type)) {
            int kind = ASRUtils::extract_kind_from_ttype_t(return_var->m_type);
            if (is_array) {
                sub = "struct u" + std::to_string(kind * 8) + "* ";
            } else {
                sub = "uint" + std::to_string(kind * 8) + "_t ";
            }
        } else if (ASRUtils::is_real(*return_var->m_type)) {
            int kind = ASRUtils::extract_kind_from_ttype_t(return_var->m_type);
            bool is_float = (kind == 4);
            if (is_array) {
                sub = "struct r" + std::to_string(kind * 8) + "* ";
            } else {
                if (is_float) {
                    sub = "float ";
                } else {
                    sub = "double ";
                }
            }
        } else if (ASRUtils::is_logical(*return_var->m_type)) {
            if (is_array) {
                sub = "struct i1* ";
            } else {
                sub = "bool ";
            }
        } else if (ASRUtils::is_character(*return_var->m_type)) {
            if (gen_stdstring) {
                sub = "std::string ";
            } else {
                sub = "char* ";
            }
        } else if (ASRUtils::is_complex(*return_var->m_type)) {
            int kind = ASRUtils::extract_kind_from_ttype_t(return_var->m_type);
            if (is_array) {
                sub = "struct c" + std::to_string(kind * 8) + "* ";
            } else {
                bool is_float = kind == 4;
                if (is_float) {
                    if (gen_stdcomplex) {
                        sub = "std::complex<float> ";
                    } else {
                        sub = "float_complex_t ";
                    }
                } else {
                    if (gen_stdcomplex) {
                        sub = "std::complex<double> ";
                    } else {
                        sub = "double_complex_t ";
                    }
                }
            }
        } else if (ASR::is_a<ASR::CPtr_t>(*return_var->m_type)) {
            sub = "void* ";
        } else if (ASR::is_a<ASR::List_t>(*return_var->m_type)) {
            ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(return_var->m_type);
            sub = c_ds_api->get_list_type(list_type) + " ";
        } else if (ASR::is_a<ASR::Tuple_t>(*return_var->m_type)) {
            ASR::Tuple_t* tup_type = ASR::down_cast<ASR::Tuple_t>(return_var->m_type);
            sub = c_ds_api->get_tuple_type(tup_type) + " ";
        } else if (ASR::is_a<ASR::TypeParameter_t>(*return_var->m_type)) {
            return "";
        } else if (ASR::is_a<ASR::Dict_t>(*return_var->m_type)) {
            ASR::Dict_t* dict_type = ASR::down_cast<ASR::Dict_t>(return_var->m_type);
            sub = c_ds_api->get_dict_type(dict_type) + " ";
        } else {
            throw CodeGenError("Return type not supported in function '" +
                CUtils::get_c_symbol_name(ASR::down_cast<ASR::symbol_t>(
                    return_var->m_parent_symtab->asr_owner)) +
                "'", return_var->base.base.loc);
        }

        return sub;
    }

    std::string get_c_function_symbol_name(const ASR::Function_t &x) {
        ASR::FunctionType_t *func_type =
            ASR::down_cast<ASR::FunctionType_t>(x.m_function_signature);
        SymbolTable *parent_symtab = ASRUtils::symbol_parent_symtab(
            reinterpret_cast<ASR::symbol_t*>(const_cast<ASR::Function_t*>(&x)));
        ASR::asr_t *owner = parent_symtab ? parent_symtab->asr_owner : nullptr;
        if (owner && ASR::is_a<ASR::symbol_t>(*owner)) {
            ASR::symbol_t *owner_sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::symbol_t>(owner));
            if (ASR::is_a<ASR::Module_t>(*owner_sym)) {
                ASR::Module_t *owner_mod = ASR::down_cast<ASR::Module_t>(owner_sym);
                if (owner_mod->m_parent_module && func_type->m_module) {
                    std::string root_module = std::string(owner_mod->m_parent_module);
                    SymbolTable *tu_symtab = owner_mod->m_symtab
                        ? owner_mod->m_symtab->parent : nullptr;
                    if (tu_symtab) {
                        ASR::symbol_t *parent_sym = tu_symtab->get_symbol(root_module);
                        while (parent_sym && ASR::is_a<ASR::Module_t>(*parent_sym)) {
                            ASR::Module_t *parent_mod =
                                ASR::down_cast<ASR::Module_t>(parent_sym);
                            if (!parent_mod->m_parent_module) {
                                break;
                            }
                            root_module = std::string(parent_mod->m_parent_module);
                            parent_sym = tu_symtab->get_symbol(root_module);
                        }
                    }
                    std::string name = CUtils::sanitize_c_identifier(std::string(x.m_name));
                    std::string prefix = CUtils::sanitize_c_identifier(root_module) + "__";
                    if (name.rfind(prefix, 0) != 0) {
                        name = prefix + name;
                    }
                    if (CUtils::uses_split_local_link_name(name)) {
                        name += CUtils::get_split_local_symbol_suffix();
                    }
                    return name;
                }
            }
        }
        return CUtils::get_c_symbol_name(
            reinterpret_cast<ASR::symbol_t*>(const_cast<ASR::Function_t*>(&x)));
    }

    std::string get_emitted_function_name(const ASR::Function_t &x) {
        std::string sym_name = "__lfortran_"
            + get_c_function_symbol_name(x);
        if (x.n_args > 0) {
            ASR::Variable_t *arg0 = ASRUtils::EXPR2VAR(x.m_args[0]);
            ASR::ttype_t *arg0_type = ASRUtils::type_get_past_allocatable_pointer(arg0->m_type);
            if (ASR::is_a<ASR::StructType_t>(*ASRUtils::extract_type(arg0_type))
                    || ASRUtils::is_class_type(arg0_type)) {
                sym_name += "__" + CUtils::get_c_type_code(arg0->m_type);
            }
        }
        if (sym_name == "main") {
            sym_name = "_xx_lcompilers_changed_main_xx";
        }
        if (sym_name == "exit") {
            sym_name = "_xx_lcompilers_changed_exit_xx";
        }
        if (sym_name == "getline") {
            sym_name = "_xx_lcompilers_changed_getline_xx";
        }
        return sym_name;
    }

    std::string get_emitted_function_name_with_local_name(const ASR::Function_t &x,
            const std::string &local_name) {
        std::string name = CUtils::sanitize_c_identifier(local_name);
        const SymbolTable *scope = x.m_symtab ? x.m_symtab->parent : nullptr;
        while (scope && scope->asr_owner) {
            ASR::asr_t *owner = scope->asr_owner;
            if (const ASR::symbol_t *owner_sym = CUtils::get_symbol_owner(owner)) {
                std::string owner_name =
                    CUtils::sanitize_c_identifier(ASRUtils::symbol_name(owner_sym));
                std::string prefix = owner_name + "__";
                if (name.rfind(prefix, 0) != 0) {
                    name = prefix + name;
                }
                if (ASR::is_a<ASR::Program_t>(*owner_sym)) {
                    break;
                }
            }
            scope = scope->parent;
        }
        if (CUtils::uses_split_local_link_name(
                    reinterpret_cast<const ASR::symbol_t*>(&x))
                || CUtils::uses_split_local_link_name(name)) {
            name += CUtils::get_split_local_symbol_suffix();
        }
        std::string sym_name = "__lfortran_" + name;
        if (x.n_args > 0) {
            ASR::Variable_t *arg0 = ASRUtils::EXPR2VAR(x.m_args[0]);
            ASR::ttype_t *arg0_type = ASRUtils::type_get_past_allocatable_pointer(arg0->m_type);
            if (ASR::is_a<ASR::StructType_t>(*ASRUtils::extract_type(arg0_type))
                    || ASRUtils::is_class_type(arg0_type)) {
                sym_name += "__" + CUtils::get_c_type_code(arg0->m_type);
            }
        }
        if (sym_name == "main") {
            sym_name = "_xx_lcompilers_changed_main_xx";
        }
        if (sym_name == "exit") {
            sym_name = "_xx_lcompilers_changed_exit_xx";
        }
        if (sym_name == "getline") {
            sym_name = "_xx_lcompilers_changed_getline_xx";
        }
        return sym_name;
    }

    std::string get_pass_array_by_data_suffix(const ASR::Function_t &x,
            const std::vector<size_t> &indices) {
        std::string suffix;
        for (size_t i = 0; i < x.n_args + 1; i++) {
            if (std::find(indices.begin(), indices.end(), i) == indices.end()) {
                continue;
            }
            ASR::Variable_t *arg = nullptr;
            ASR::Function_t *arg_func = nullptr;
            if (i < x.n_args) {
                if (!ASR::is_a<ASR::Var_t>(*x.m_args[i])) {
                    continue;
                }
                ASR::Var_t *x_arg = ASR::down_cast<ASR::Var_t>(x.m_args[i]);
                ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(x_arg->m_v);
                if (ASR::is_a<ASR::Function_t>(*arg_sym)) {
                    arg_func = ASR::down_cast<ASR::Function_t>(arg_sym);
                } else if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                    arg = ASR::down_cast<ASR::Variable_t>(arg_sym);
                }
            } else if (x.m_return_var) {
                arg = ASRUtils::EXPR2VAR(x.m_return_var);
            } else {
                break;
            }
            if (arg_func) {
                suffix += "_" + ASRUtils::get_type_code(
                    arg_func->m_function_signature, true);
            } else if (arg) {
                suffix += "_" + ASRUtils::type_to_str_fortran_symbol(
                    arg->m_type, arg->m_type_declaration);
            }
            suffix += "_" + std::to_string(i);
        }
        suffix = to_lower(suffix);
        for (size_t i = 0; i < suffix.size(); i++) {
            if (!((suffix[i] >= 'a' && suffix[i] <= 'z')
                    || (suffix[i] >= '0' && suffix[i] <= '9'))) {
                suffix[i] = '_';
            }
        }
        return suffix;
    }

    void record_generated_forward_decl(const std::string &decl) {
        if (decl.empty()) {
            return;
        }
        std::string decl_line = decl + ";\n";
        if (forward_decl_functions.find(decl_line) == std::string::npos) {
            forward_decl_functions += decl_line;
        }
    }

    std::string get_pass_array_by_data_method_suffix(const ASR::Function_t &x) {
        if (x.n_args == 0) {
            return "";
        }
        ASR::Variable_t *arg0 = ASRUtils::EXPR2VAR(x.m_args[0]);
        ASR::ttype_t *arg0_type =
            ASRUtils::type_get_past_allocatable_pointer(arg0->m_type);
        if (ASR::is_a<ASR::StructType_t>(*ASRUtils::extract_type(arg0_type))
                || ASRUtils::is_class_type(arg0_type)) {
            return "__" + CUtils::get_c_type_code(arg0->m_type);
        }
        return "";
    }

    void get_pass_array_by_data_local_names(const ASR::Function_t &x,
            const std::string &pass_suffix, std::string &public_local_name,
            std::string &specialized_local_name, bool &body_is_specialized) {
        std::string local_name = std::string(x.m_name);
        std::string method_suffix = get_pass_array_by_data_method_suffix(x);
        public_local_name = local_name;
        specialized_local_name = local_name;
        body_is_specialized = false;
        if (pass_suffix.empty()) {
            return;
        }

        auto try_strip_specialization = [&](const std::string &name,
                const std::string &trailing) -> bool {
            if (!trailing.empty() && !endswith(name, trailing)) {
                return false;
            }
            std::string base = trailing.empty()
                ? name
                : name.substr(0, name.size() - trailing.size());
            if (!endswith(base, pass_suffix)) {
                return false;
            }
            public_local_name = base.substr(0, base.size() - pass_suffix.size()) + trailing;
            specialized_local_name = name;
            body_is_specialized = true;
            return true;
        };

        if (!method_suffix.empty() && try_strip_specialization(local_name, method_suffix)) {
            return;
        }
        if (try_strip_specialization(local_name, "")) {
            return;
        }

        if (!method_suffix.empty() && endswith(local_name, method_suffix)) {
            std::string base = local_name.substr(0, local_name.size() - method_suffix.size());
            specialized_local_name = base + pass_suffix + method_suffix;
        } else {
            specialized_local_name = local_name + pass_suffix;
        }
    }

    bool is_pass_array_by_data_hidden_arg_name(const std::string &candidate,
            const std::string &base_name) {
        if (candidate.size() <= 2 + base_name.size()
                || candidate.rfind("__", 0) != 0) {
            return false;
        }
        size_t pos = 2;
        while (pos < candidate.size() && std::isdigit(candidate[pos])) {
            pos++;
        }
        if (pos == 2) {
            return false;
        }
        return candidate.substr(pos) == base_name;
    }

    bool get_specialized_pass_array_by_data_args(const ASR::Function_t &x,
            std::vector<size_t> &public_arg_positions,
            std::vector<size_t> &public_indices_with_hidden_dims,
            std::string &pass_suffix) {
        std::vector<std::string> public_arg_names;
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                return false;
            }
            ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
            std::string arg_name = std::string(arg->m_name);
            if (!public_arg_names.empty()
                    && is_pass_array_by_data_hidden_arg_name(
                        arg_name, public_arg_names.back())) {
                if (public_indices_with_hidden_dims.empty()
                        || public_indices_with_hidden_dims.back()
                            != public_arg_names.size() - 1) {
                    public_indices_with_hidden_dims.push_back(
                        public_arg_names.size() - 1);
                }
                continue;
            }
            public_arg_positions.push_back(i);
            public_arg_names.push_back(arg_name);
        }
        if (public_indices_with_hidden_dims.empty()) {
            return false;
        }

        for (size_t k = 0; k < public_indices_with_hidden_dims.size(); k++) {
            size_t public_i = public_indices_with_hidden_dims[k];
            size_t arg_pos = public_arg_positions[public_i];
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[arg_pos])->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                return false;
            }
            ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
            pass_suffix += "_" + ASRUtils::type_to_str_fortran_symbol(
                arg->m_type, arg->m_type_declaration);
            pass_suffix += "_" + std::to_string(public_i);
        }
        pass_suffix = to_lower(pass_suffix);
        for (size_t i = 0; i < pass_suffix.size(); i++) {
            if (!((pass_suffix[i] >= 'a' && pass_suffix[i] <= 'z')
                    || (pass_suffix[i] >= '0' && pass_suffix[i] <= '9'))) {
                pass_suffix[i] = '_';
            }
        }
        return true;
    }

    ASR::Function_t *find_other_scope_implementation_with_local_name(
            SymbolTable *scope, const std::string &local_name,
            const ASR::Function_t &current) {
        if (scope == nullptr || local_name.empty()) {
            return nullptr;
        }

        std::function<ASR::Function_t*(ASR::symbol_t*)> find_in_symbol =
            [&](ASR::symbol_t *sym) -> ASR::Function_t* {
                sym = ASRUtils::symbol_get_past_external(sym);
                if (sym == nullptr) {
                    return nullptr;
                }
                if (ASR::is_a<ASR::Function_t>(*sym)) {
                    ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(sym);
                    ASR::FunctionType_t *fn_type = ASRUtils::get_FunctionType(*fn);
                    if (fn != &current
                            && fn_type->m_deftype == ASR::deftypeType::Implementation
                            && std::string(fn->m_name) == local_name) {
                        return fn;
                    }
                    return nullptr;
                }
                auto find_in_procs = [&](ASR::symbol_t **procs,
                        size_t n_procs) -> ASR::Function_t* {
                    for (size_t i = 0; i < n_procs; i++) {
                        if (ASR::Function_t *fn = find_in_symbol(procs[i])) {
                            return fn;
                        }
                    }
                    return nullptr;
                };
                if (ASR::is_a<ASR::GenericProcedure_t>(*sym)) {
                    ASR::GenericProcedure_t *gp = ASR::down_cast<ASR::GenericProcedure_t>(sym);
                    return find_in_procs(gp->m_procs, gp->n_procs);
                }
                if (ASR::is_a<ASR::CustomOperator_t>(*sym)) {
                    ASR::CustomOperator_t *op = ASR::down_cast<ASR::CustomOperator_t>(sym);
                    return find_in_procs(op->m_procs, op->n_procs);
                }
                if (ASR::is_a<ASR::StructMethodDeclaration_t>(*sym)) {
                    ASR::StructMethodDeclaration_t *md =
                        ASR::down_cast<ASR::StructMethodDeclaration_t>(sym);
                    return find_in_symbol(md->m_proc);
                }
                return nullptr;
            };

        for (auto &item : scope->get_scope()) {
            if (ASR::Function_t *fn = find_in_symbol(item.second)) {
                return fn;
            }
        }
        return nullptr;
    }

    std::string get_pass_array_by_data_wrapper(const ASR::Function_t &x) {
        if (!is_c) {
            return "";
        }
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type->m_abi != ASR::abiType::Source
                || f_type->m_deftype == ASR::deftypeType::Interface) {
            return "";
        }

        std::vector<size_t> public_arg_positions;
        std::vector<size_t> public_indices_with_hidden_dims;
        std::string pass_suffix;
        bool uses_specialized_hidden_dims = false;
        bool body_is_specialized = false;
        std::vector<size_t> indices;
        if (ASRUtils::is_pass_array_by_data_possible(
                const_cast<ASR::Function_t*>(&x), indices)) {
            public_arg_positions.resize(x.n_args);
            std::iota(public_arg_positions.begin(), public_arg_positions.end(), 0);
            pass_suffix = get_pass_array_by_data_suffix(x, indices);
        } else if (get_specialized_pass_array_by_data_args(x, public_arg_positions,
                public_indices_with_hidden_dims, pass_suffix)) {
            uses_specialized_hidden_dims = true;
        } else {
            return "";
        }

        std::string public_local_name;
        std::string specialized_local_name;
        get_pass_array_by_data_local_names(x, pass_suffix, public_local_name,
            specialized_local_name, body_is_specialized);
        std::string wrapper_local_name = body_is_specialized
            ? public_local_name
            : specialized_local_name;
        if (wrapper_local_name.empty() || wrapper_local_name == std::string(x.m_name)) {
            return "";
        }

        SymbolTable *parent_scope = x.m_symtab ? x.m_symtab->parent : nullptr;
        if (parent_scope && !startswith(x.m_name, "_lcompilers_")) {
            if (find_other_scope_implementation_with_local_name(
                        parent_scope, wrapper_local_name, x) != nullptr) {
                return "";
            }
        }

        bool has_typevar = false;
        std::string wrapper_src;
        std::string wrapper_decl;
        if (x.m_return_var) {
            wrapper_decl += get_return_var_type(ASRUtils::EXPR2VAR(x.m_return_var));
        } else {
            wrapper_decl += "void ";
        }
        wrapper_decl += get_emitted_function_name_with_local_name(x, wrapper_local_name) + "(";

        std::vector<std::string> forwarded_args;
        std::vector<std::vector<std::string>> forwarded_hidden_args;
        for (size_t public_i = 0; public_i < public_arg_positions.size(); public_i++) {
            size_t arg_pos = public_arg_positions[public_i];
            forwarded_hidden_args.push_back({});
            if (public_i > 0) {
                wrapper_decl += ", ";
            }
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[arg_pos])->m_v);
            std::string arg_name;
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
                CDeclarationOptions c_decl_options;
                c_decl_options.pre_initialise_derived_type = false;
                wrapper_decl += self().convert_variable_decl(*arg, &c_decl_options);
                arg_name = CUtils::sanitize_c_identifier(arg->m_name);
                std::string dim_type = compiler_options.po.descriptor_index_64
                    ? "int64_t"
                    : "int32_t";
                bool arg_has_hidden_dims = uses_specialized_hidden_dims
                    ? std::find(public_indices_with_hidden_dims.begin(),
                        public_indices_with_hidden_dims.end(), public_i)
                        != public_indices_with_hidden_dims.end()
                    : std::find(indices.begin(), indices.end(), public_i)
                        != indices.end();
                if (arg_has_hidden_dims) {
                    ASR::dimension_t *dims = nullptr;
                    int n_dims = ASRUtils::extract_dimensions_from_ttype(arg->m_type, dims);
                    if (uses_specialized_hidden_dims || body_is_specialized) {
                        for (int j = 0; j < n_dims; j++) {
                            forwarded_hidden_args.back().push_back(
                                "((" + dim_type + ")" + arg_name + "->dims[" + std::to_string(j)
                                + "].length + " + arg_name + "->dims[" + std::to_string(j)
                                + "].lower_bound - 1)");
                        }
                    } else {
                        for (int j = 0; j < n_dims; j++) {
                            wrapper_decl += ", " + dim_type + " __" + std::to_string(j + 1)
                                + arg_name;
                        }
                    }
                }
            } else if (ASR::is_a<ASR::Function_t>(*sym)) {
                ASR::Function_t *fun = ASR::down_cast<ASR::Function_t>(sym);
                wrapper_decl += get_function_declaration(*fun, has_typevar, true, false);
                arg_name = get_c_function_target_name(*fun);
            } else {
                return "";
            }
            forwarded_args.push_back(arg_name);
        }
        wrapper_decl += ")";
        if (has_typevar) {
            return "";
        }
        record_generated_forward_decl(wrapper_decl);

        wrapper_src += wrapper_decl;
        wrapper_src += "\n{\n";
        if (x.m_return_var) {
            wrapper_src += "    return ";
        } else {
            wrapper_src += "    ";
        }
        wrapper_src += get_emitted_function_name(x) + "(";
        for (size_t i = 0; i < forwarded_args.size(); i++) {
            if (i > 0) {
                wrapper_src += ", ";
            }
            wrapper_src += forwarded_args[i];
            for (size_t j = 0; j < forwarded_hidden_args[i].size(); j++) {
                wrapper_src += ", ";
                wrapper_src += forwarded_hidden_args[i][j];
            }
        }
        wrapper_src += ");\n}\n";
        return wrapper_src;
    }

    std::string get_c_function_target_name(const ASR::Function_t &x) {
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type->m_abi == ASR::abiType::BindC) {
            if (f_type->m_bindc_name && std::strlen(f_type->m_bindc_name) > 0) {
                return CUtils::sanitize_c_identifier(std::string(f_type->m_bindc_name));
            } else if (!f_type->m_bindc_name) {
                return CUtils::sanitize_c_identifier(std::string(x.m_name));
            }
        } else if (f_type->m_deftype == ASR::deftypeType::Interface
                && f_type->m_abi != ASR::abiType::Intrinsic) {
            // Preserve raw names only for true external interface procedures.
            // Type-bound interface declarations with a derived/class receiver
            // otherwise collide in C as raw `destroy/get/...` prototypes.
            bool keep_external_name = true;
            if (x.n_args > 0) {
                ASR::Variable_t *arg0 = ASRUtils::EXPR2VAR(x.m_args[0]);
                ASR::ttype_t *arg0_type = ASRUtils::type_get_past_allocatable_pointer(arg0->m_type);
                if (arg0_type != nullptr) {
                    ASR::ttype_t *arg0_base = ASRUtils::extract_type(arg0_type);
                    keep_external_name = !(ASRUtils::is_class_type(arg0_type)
                        || (arg0_base != nullptr && ASR::is_a<ASR::StructType_t>(*arg0_base)));
                }
            }
            if (keep_external_name) {
                return CUtils::sanitize_c_identifier(std::string(x.m_name));
            }
        }
        return get_emitted_function_name(x);
    }

    bool is_bindc_optional_scalar_dummy(const ASR::Variable_t *v) {
        if (!is_c || v == nullptr || !ASRUtils::is_arg_dummy(v->m_intent)
                || v->m_presence != ASR::presenceType::Optional) {
            return false;
        }
        ASR::asr_t *owner = v->m_parent_symtab ? v->m_parent_symtab->asr_owner : nullptr;
        if (!(owner && ASR::is_a<ASR::symbol_t>(*owner))) {
            return false;
        }
        ASR::symbol_t *owner_sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::symbol_t>(owner));
        if (!ASR::is_a<ASR::Function_t>(*owner_sym)) {
            return false;
        }
        ASR::Function_t *owner_fn = ASR::down_cast<ASR::Function_t>(owner_sym);
        ASR::FunctionType_t *owner_fn_type = ASRUtils::get_FunctionType(*owner_fn);
        if (owner_fn_type->m_abi != ASR::abiType::BindC) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(v->m_type);
        return !ASRUtils::is_array(v->m_type)
            && !ASRUtils::is_aggregate_type(v->m_type)
            && !ASRUtils::is_character(*type)
            && !ASR::is_a<ASR::CPtr_t>(*type)
            && !ASRUtils::is_pointer(v->m_type)
            && !ASRUtils::is_allocatable(v->m_type);
    }

    bool is_raw_c_char_array_dummy(const ASR::Variable_t *v) {
        if (!is_c || v == nullptr || !ASRUtils::is_arg_dummy(v->m_intent)
                || !CUtils::is_len_one_character_array_type(v->m_type)) {
            return false;
        }
        ASR::asr_t *owner = v->m_parent_symtab ? v->m_parent_symtab->asr_owner : nullptr;
        if (!(owner && ASR::is_a<ASR::symbol_t>(*owner))) {
            return false;
        }
        ASR::symbol_t *owner_sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::symbol_t>(owner));
        if (!ASR::is_a<ASR::Function_t>(*owner_sym)) {
            return false;
        }
        return ASRUtils::get_FunctionType(*ASR::down_cast<ASR::Function_t>(owner_sym))->m_abi
            == ASR::abiType::BindC;
    }

    bool is_hidden_char_length_param(const ASR::Variable_t *v) {
        if (v == nullptr) {
            return false;
        }
        std::string name = v->m_name;
        return name.rfind("__", 0) == 0;
    }

    bool is_fortran_external_interface_function(const ASR::Function_t &x) {
        if (!is_c) {
            return false;
        }
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type->m_abi != ASR::abiType::Source
                || f_type->m_deftype != ASR::deftypeType::Interface) {
            return false;
        }
        if (x.n_args == 0) {
            return true;
        }
        ASR::Variable_t *arg0 = ASRUtils::EXPR2VAR(x.m_args[0]);
        ASR::ttype_t *arg0_type =
            ASRUtils::type_get_past_allocatable_pointer(arg0->m_type);
        if (arg0_type == nullptr) {
            return true;
        }
        ASR::ttype_t *arg0_base = ASRUtils::extract_type(arg0_type);
        return !(ASRUtils::is_class_type(arg0_type)
            || (arg0_base != nullptr && ASR::is_a<ASR::StructType_t>(*arg0_base)));
    }

    std::string get_fortran_external_value_type(ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable_pointer(type));
        if (type == nullptr) {
            return "void";
        }
        if (ASRUtils::is_complex(*type)) {
            headers.insert("complex.h");
        } else if (ASRUtils::is_logical(*type)) {
            headers.insert("stdbool.h");
        } else if (ASRUtils::is_integer(*type) || ASRUtils::is_unsigned_integer(*type)) {
            headers.insert("inttypes.h");
        }
        return CUtils::get_c_type_from_ttype_t(type);
    }

    std::string get_fortran_external_arg_decl(const ASR::Variable_t &arg) {
        std::string arg_name = CUtils::sanitize_c_identifier(arg.m_name);
        ASR::ttype_t *arg_type =
            ASRUtils::type_get_past_allocatable_pointer(arg.m_type);
        if (ASRUtils::is_array(arg.m_type)) {
            std::string type_name = get_fortran_external_value_type(arg_type);
            return format_type_c("", type_name + " *", arg_name, false, true);
        }
        if (arg_type != nullptr && ASRUtils::is_character(*arg_type)) {
            return format_type_c("", "char *", arg_name, false, true);
        }
        std::string type_name = get_fortran_external_value_type(arg_type);
        return format_type_c("", type_name + " *", arg_name, false, true);
    }

    // Returns the declaration, no semi colon at the end
    std::string get_function_declaration(const ASR::Function_t &x, bool &has_typevar,
                                         bool is_pointer=false,
                                         bool record_forward_decl=true) {
        template_for_Kokkos.clear();
        template_number = 0;
        std::string sub, inl, static_attr;
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);

        // This helps to check if the function is generic.
        // If it is generic we skip the codegen for that function.
        has_typevar = false;
        if (f_type->m_inline && !is_pointer) {
            inl = "inline __attribute__((always_inline)) ";
        }
        if( f_type->m_static && !is_pointer) {
            static_attr = "static ";
        }
        if (x.m_return_var) {
            ASR::Variable_t *return_var = ASRUtils::EXPR2VAR(x.m_return_var);
            has_typevar = ASR::is_a<ASR::TypeParameter_t>(*return_var->m_type);
            sub = get_return_var_type(return_var);
        } else {
            sub = "void ";
        }
        std::string sym_name = get_c_function_target_name(x);
        if (f_type->m_abi == ASR::abiType::BindPython &&
                f_type->m_deftype == ASR::deftypeType::Implementation) {
            sym_name = "_xx_internal_" + sym_name + "_xx";
        }
        if (is_c && f_type->m_abi == ASR::abiType::BindC
                && f_type->m_bindc_name
                && std::string(f_type->m_bindc_name).rfind("_lfortran_", 0) == 0) {
            return "";
        }
        if (is_c && f_type->m_abi == ASR::abiType::BindC
                && f_type->m_deftype == ASR::deftypeType::Interface
                && CUtils::is_standard_c_runtime_function(sym_name)) {
            return "";
        }
        std::string func = static_attr + inl + sub;
        bool use_fortran_external_abi_args =
            is_fortran_external_interface_function(x);
        if (is_pointer) {
            func += "(*" + sym_name + ")(";
        } else {
            func += sym_name + "(";
        }
        bracket_open++;
        for (size_t i=0; i<x.n_args; i++) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
                LCOMPILERS_ASSERT(ASRUtils::is_arg_dummy(arg->m_intent));
                if (use_fortran_external_abi_args) {
                    func += get_fortran_external_arg_decl(*arg);
                } else if( is_c ) {
                    CDeclarationOptions c_decl_options;
                    c_decl_options.pre_initialise_derived_type = false;
                    func += self().convert_variable_decl(*arg, &c_decl_options);
                } else {
                    CPPDeclarationOptions cpp_decl_options;
                    cpp_decl_options.use_static = false;
                    cpp_decl_options.use_templates_for_arrays = true;
                    func += self().convert_variable_decl(*arg, &cpp_decl_options);
                }
                if (ASR::is_a<ASR::TypeParameter_t>(*arg->m_type)) {
                    has_typevar = true;
                    bracket_open--;
                    return "";
                }
            } else if (ASR::is_a<ASR::Function_t>(*sym)) {
                ASR::Function_t *fun = ASR::down_cast<ASR::Function_t>(sym);
                func += get_function_declaration(*fun, has_typevar, true,
                                                 record_forward_decl);
            } else {
                throw CodeGenError("Unsupported function argument");
            }
            if (i < x.n_args-1) func += ", ";
        }
        func += ")";
        bracket_open--;
        if (record_forward_decl
                && is_c && f_type->m_abi == ASR::abiType::Source
                && f_type->m_deftype != ASR::deftypeType::Interface
                && !is_pointer) {
            forward_decl_functions += func + ";\n";
        }
        if( is_c || template_for_Kokkos.empty() ) {
            return func;
        }

        template_for_Kokkos.pop_back();
        template_for_Kokkos.pop_back();
        return "\ntemplate <" + template_for_Kokkos + ">\n" + func;
    }

    void record_forward_decl_for_function(const ASR::Function_t &x) {
        if (!is_c) {
            return;
        }
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type->m_deftype == ASR::deftypeType::Interface) {
            return;
        }
        bool has_typevar = false;
        std::string decl = get_function_declaration(x, has_typevar, false, false);
        if (has_typevar || decl.empty()) {
            return;
        }
        std::string decl_line = decl + ";\n";
        if (forward_decl_functions.find(decl_line) == std::string::npos) {
            forward_decl_functions += decl_line;
        }
        if (f_type->m_abi == ASR::abiType::Intrinsic) {
            return;
        }
        if (f_type->m_abi == ASR::abiType::BindC && x.m_module_file) {
            return;
        }
        uint64_t key = get_hash(reinterpret_cast<ASR::asr_t*>(
            const_cast<ASR::Function_t*>(&x)));
        if (pending_function_definition_hashes.insert(key).second) {
            pending_function_definitions.push_back(
                const_cast<ASR::Function_t*>(&x));
        }
    }

    bool get_pass_array_by_data_direct_call_target(const ASR::Function_t &x,
            std::string &target_name, std::vector<size_t> &indices) {
        if (!is_c) {
            return false;
        }
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        if (f_type->m_abi != ASR::abiType::Source
                || f_type->m_deftype == ASR::deftypeType::Interface) {
            return false;
        }
        if (!ASRUtils::is_pass_array_by_data_possible(
                const_cast<ASR::Function_t*>(&x), indices)) {
            return false;
        }
        if (indices.empty()) {
            return false;
        }
        std::string pass_suffix = get_pass_array_by_data_suffix(x, indices);
        std::string public_local_name;
        std::string specialized_local_name;
        bool body_is_specialized = false;
        get_pass_array_by_data_local_names(x, pass_suffix, public_local_name,
            specialized_local_name, body_is_specialized);
        if (specialized_local_name.empty()
                || specialized_local_name == std::string(x.m_name)) {
            return false;
        }
        target_name = get_emitted_function_name_with_local_name(x, specialized_local_name);
        return !target_name.empty();
    }

    bool try_construct_specialized_pass_array_no_copy_call_args(
            ASR::Function_t *f, size_t n_args, ASR::call_arg_t *m_args,
            std::string &args) {
        if (!is_c || f == nullptr || n_args != f->n_args) {
            return false;
        }
        std::vector<size_t> public_arg_positions;
        std::vector<size_t> public_indices_with_hidden_dims;
        std::string pass_suffix;
        if (!get_specialized_pass_array_by_data_args(
                *f, public_arg_positions, public_indices_with_hidden_dims,
                pass_suffix)) {
            return false;
        }

        std::string call_arg_setup;
        for (size_t i = 0; i < n_args; i++) {
            if (m_args[i].m_value == nullptr || !ASR::is_a<ASR::Var_t>(*f->m_args[i])) {
                return false;
            }

            ASR::expr_t *call_arg = m_args[i].m_value;
            ASR::Variable_t *param = ASRUtils::EXPR2VAR(f->m_args[i]);
            ASR::ttype_t *param_type = ASRUtils::expr_type(f->m_args[i]);
            ASR::ttype_t *param_type_unwrapped =
                ASRUtils::type_get_past_allocatable_pointer(param_type);
            bool param_is_array = param_type_unwrapped
                && ASRUtils::is_array(param_type_unwrapped);
            if (param_is_array) {
                ASR::ttype_t *param_element_type =
                    ASRUtils::type_get_past_allocatable_pointer(
                        ASRUtils::extract_type(param_type_unwrapped));
                ASR::ttype_t *arg_element_type =
                    ASRUtils::type_get_past_allocatable_pointer(
                        ASRUtils::extract_type(ASRUtils::expr_type(call_arg)));
                if ((param_element_type && ASRUtils::is_character(*param_element_type))
                        || (arg_element_type && ASRUtils::is_character(*arg_element_type))) {
                    return false;
                }
            }
            if (!param_is_array) {
                if (param->m_intent != ASRUtils::intent_in) {
                    return false;
                }
                if (ASRUtils::is_pointer(param_type)
                        || ASRUtils::is_allocatable(param_type)) {
                    return false;
                }
                if (ASRUtils::is_aggregate_type(param_type)) {
                    ASR::expr_t *raw_arg = unwrap_c_lvalue_expr(call_arg);
                    if (raw_arg == nullptr
                            || !ASR::is_a<ASR::Var_t>(*raw_arg)) {
                        return false;
                    }
                }
            }
            if (param_is_array) {
                ASR::expr_t *unwrapped_call_arg = unwrap_c_lvalue_expr(call_arg);
                if ((unwrapped_call_arg != nullptr
                            && ASR::is_a<ASR::ArraySection_t>(*unwrapped_call_arg))
                        || is_c_array_section_association_temp_expr(call_arg)) {
                    return false;
                }
            }

            visit_expr_without_c_pow_cache(*call_arg);
            std::string arg_src = src;
            std::string arg_setup = drain_tmp_buffer();
            arg_setup += extract_stmt_setup_from_expr(arg_src);

            if (param_is_array) {
                bool param_defines_actual = param
                    && (param->m_intent == ASRUtils::intent_out
                        || param->m_intent == ASRUtils::intent_inout
                        || (std::string(param->m_name) == "result"
                            && f != nullptr
                            && std::string(f->m_name).find("lcompilers_matmul")
                                != std::string::npos));
                if (is_c && param_defines_actual) {
                    std::string lazy_setup =
                        self().emit_c_lazy_automatic_array_temp_allocation(
                            call_arg, arg_src);
                    if (!lazy_setup.empty()) {
                        call_arg_setup += arg_setup;
                        arg_setup.clear();
                        call_arg_setup += lazy_setup;
                    }
                }
                bool no_copy_descriptor_view_actual =
                    try_build_c_array_no_copy_descriptor_view_arg(
                        call_arg, param_type,
                        param ? param->m_type_declaration : nullptr, arg_src,
                        true);
                ASR::expr_t *unwrapped_call_arg = unwrap_c_lvalue_expr(call_arg);
                if (!no_copy_descriptor_view_actual
                        && unwrapped_call_arg != nullptr
                        && ASR::is_a<ASR::ArraySection_t>(*unwrapped_call_arg)) {
                    ASR::ttype_t *param_array_type =
                        ASRUtils::type_get_past_allocatable_pointer(param_type);
                    if (param_array_type != nullptr
                            && ASRUtils::is_array(param_array_type)) {
                        std::string view_src = build_c_array_no_copy_descriptor_view(
                            param_array_type, call_arg, arg_src,
                            param ? param->m_type_declaration : nullptr, true);
                        if (view_src != arg_src) {
                            arg_src = view_src;
                            no_copy_descriptor_view_actual = true;
                        }
                    }
                }
                call_arg_setup += drain_tmp_buffer();
                if (!no_copy_descriptor_view_actual) {
                    call_arg_setup += arg_setup;
                }
            } else {
                call_arg_setup += arg_setup;
                if (ASRUtils::is_aggregate_type(param_type)) {
                    bool pointer_backed_aggregate_actual =
                        is_pointer_backed_struct_expr(call_arg);
                    if (!pointer_backed_aggregate_actual) {
                        ASR::expr_t *raw_arg = unwrap_c_lvalue_expr(call_arg);
                        ASR::expr_t *shape_arg = raw_arg ? raw_arg : call_arg;
                        std::string addressable =
                            get_addressable_call_arg_src(shape_arg, arg_src);
                        arg_src = cast_aggregate_pointer_actual_to_param_type(
                            param, "&" + addressable);
                    }
                }
            }

            if (!args.empty()) {
                args += ", ";
            }
            args += arg_src;
        }
        if (!call_arg_setup.empty()) {
            tmp_buffer_src.push_back(call_arg_setup);
        }
        return true;
    }

    bool try_construct_pass_array_by_data_direct_call_args(
            ASR::Function_t *f, size_t n_args, ASR::call_arg_t *m_args,
            const std::vector<size_t> &indices, std::string &args) {
        if (!is_c || f == nullptr || n_args != f->n_args) {
            return false;
        }
        std::string call_arg_setup;
        std::string dim_type = compiler_options.po.descriptor_index_64
            ? "int64_t" : "int32_t";
        for (size_t i = 0; i < n_args; i++) {
            if (m_args[i].m_value == nullptr || !ASR::is_a<ASR::Var_t>(*f->m_args[i])) {
                return false;
            }
            ASR::Variable_t *param = ASRUtils::EXPR2VAR(f->m_args[i]);
            ASR::ttype_t *param_type = ASRUtils::expr_type(f->m_args[i]);
            ASR::ttype_t *param_type_unwrapped =
                ASRUtils::type_get_past_allocatable_pointer(param_type);
            bool param_is_array = param_type_unwrapped
                && ASRUtils::is_array(param_type_unwrapped);
            if (!param_is_array
                    && (param->m_intent != ASRUtils::intent_in
                        || ASRUtils::is_aggregate_type(param_type)
                        || ASRUtils::is_pointer(param_type)
                        || ASRUtils::is_allocatable(param_type))) {
                return false;
            }
        }
        for (size_t i = 0; i < n_args; i++) {
            ASR::expr_t *call_arg = m_args[i].m_value;
            ASR::Variable_t *param = ASRUtils::EXPR2VAR(f->m_args[i]);
            ASR::ttype_t *param_type = ASRUtils::expr_type(f->m_args[i]);
            ASR::ttype_t *param_type_unwrapped =
                ASRUtils::type_get_past_allocatable_pointer(param_type);
            bool param_is_array = param_type_unwrapped
                && ASRUtils::is_array(param_type_unwrapped);
            if (param_is_array) {
                ASR::expr_t *unwrapped_call_arg = unwrap_c_lvalue_expr(call_arg);
                if ((unwrapped_call_arg != nullptr
                            && ASR::is_a<ASR::ArraySection_t>(*unwrapped_call_arg))
                        || is_c_array_section_association_temp_expr(call_arg)) {
                    return false;
                }
            }

            visit_expr_without_c_pow_cache(*call_arg);
            std::string arg_src = src;
            std::string arg_setup = drain_tmp_buffer();
            arg_setup += extract_stmt_setup_from_expr(arg_src);

            if (param_is_array) {
                bool param_defines_actual = param
                    && (param->m_intent == ASRUtils::intent_out
                        || param->m_intent == ASRUtils::intent_inout
                        || (std::string(param->m_name) == "result"
                            && f != nullptr
                            && std::string(f->m_name).find("lcompilers_matmul")
                                != std::string::npos));
                if (is_c && param_defines_actual) {
                    std::string lazy_setup =
                        self().emit_c_lazy_automatic_array_temp_allocation(
                            call_arg, arg_src);
                    if (!lazy_setup.empty()) {
                        call_arg_setup += arg_setup;
                        arg_setup.clear();
                        call_arg_setup += lazy_setup;
                    }
                }
                bool no_copy_descriptor_view_actual =
                    try_build_c_array_no_copy_descriptor_view_arg(
                        call_arg, param_type,
                        param ? param->m_type_declaration : nullptr, arg_src,
                        true);
                call_arg_setup += drain_tmp_buffer();
                if (!no_copy_descriptor_view_actual) {
                    call_arg_setup += arg_setup;
                }
                if (!args.empty()) {
                    args += ", ";
                }
                args += arg_src;
                if (std::find(indices.begin(), indices.end(), i) != indices.end()) {
                    std::string shape_src = get_c_descriptor_member_base_expr(arg_src);
                    int rank = ASRUtils::extract_n_dims_from_ttype(param_type_unwrapped);
                    for (int dim = 0; dim < rank; dim++) {
                        args += ", ((" + dim_type + ") " + shape_src + "->dims["
                            + std::to_string(dim) + "].length + " + shape_src
                            + "->dims[" + std::to_string(dim) + "].lower_bound - 1)";
                    }
                }
                continue;
            }

            call_arg_setup += arg_setup;
            if (!args.empty()) {
                args += ", ";
            }
            args += arg_src;
        }
        if (!call_arg_setup.empty()) {
            tmp_buffer_src.push_back(call_arg_setup);
        }
        return true;
    }

    void record_pass_array_by_data_direct_call_decl(const ASR::Function_t &x,
            const std::string &target_name, const std::vector<size_t> &indices) {
        if (!is_c || target_name.empty()) {
            return;
        }
        bool has_typevar = false;
        std::string decl;
        if (x.m_return_var) {
            decl += get_return_var_type(ASRUtils::EXPR2VAR(x.m_return_var));
        } else {
            decl += "void ";
        }
        decl += target_name + "(";
        std::string dim_type = compiler_options.po.descriptor_index_64
            ? "int64_t" : "int32_t";
        for (size_t i = 0; i < x.n_args; i++) {
            if (i > 0) {
                decl += ", ";
            }
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
                CDeclarationOptions c_decl_options;
                c_decl_options.pre_initialise_derived_type = false;
                decl += self().convert_variable_decl(*arg, &c_decl_options);
                if (ASR::is_a<ASR::TypeParameter_t>(*arg->m_type)) {
                    has_typevar = true;
                    break;
                }
                if (std::find(indices.begin(), indices.end(), i) != indices.end()) {
                    ASR::dimension_t *dims = nullptr;
                    int n_dims = ASRUtils::extract_dimensions_from_ttype(arg->m_type, dims);
                    std::string arg_name = CUtils::sanitize_c_identifier(arg->m_name);
                    for (int j = 0; j < n_dims; j++) {
                        decl += ", " + dim_type + " __" + std::to_string(j + 1)
                            + arg_name;
                    }
                }
            } else if (ASR::is_a<ASR::Function_t>(*sym)) {
                ASR::Function_t *fun = ASR::down_cast<ASR::Function_t>(sym);
                decl += get_function_declaration(*fun, has_typevar, true, false);
            } else {
                return;
            }
        }
        decl += ")";
        if (!has_typevar) {
            record_generated_forward_decl(decl);
        }
    }

    std::string get_arg_conv_bind_python(const ASR::Function_t &x) {
        std::string arg_conv = R"(
    pArgs = PyTuple_New()" + std::to_string(x.n_args) + R"();
)";
        for (size_t i = 0; i < x.n_args; ++i) {
            ASR::Variable_t *arg = ASRUtils::EXPR2VAR(x.m_args[i]);
            std::string arg_name = CUtils::sanitize_c_identifier(arg->m_name);
            std::string indent = "\n    ";
            if (ASRUtils::is_array(arg->m_type)) {
                arg_conv += indent + bind_py_utils_functions->get_conv_dims_to_1D_arr() + "(" + arg_name + "->n_dims, " + arg_name + "->dims, __new_dims);";
                std::string func_call = BindPyUtils::get_py_obj_type_conv_func_from_ttype_t(arg->m_type);
                arg_conv += indent + "pValue = " + func_call + "(" + arg_name + "->n_dims, __new_dims, "
                    + BindPyUtils::get_numpy_c_obj_type_conv_func_from_ttype_t(arg->m_type) + ", " + arg_name + "->data);";
            } else {
                arg_conv += indent + "pValue = " + BindPyUtils::get_py_obj_type_conv_func_from_ttype_t(arg->m_type)
                    + "(" + arg_name + ");";
            }
            arg_conv += R"(
    if (!pValue) {
        Py_DECREF(pArgs);
        Py_DECREF(pModule);
        fprintf(stderr, "Cannot convert argument\n");
        exit(1);
    }
    /* pValue reference stolen here: */
    PyTuple_SetItem(pArgs, )" + std::to_string(i) +  R"(, pValue);
)";
        }
        return arg_conv;
    }

    std::string get_return_value_conv_bind_python(const ASR::Function_t &x) {
        if (!x.m_return_var) return "";
        ASR::Variable_t* r_v = ASRUtils::EXPR2VAR(x.m_return_var);
        std::string indent = "\n    ";
        std::string ret_var_decl = indent + get_return_var_type(r_v) + " _lpython_return_variable;";
        std::string py_val_cnvrt = BindPyUtils::get_py_obj_ret_type_conv_fn_from_ttype(r_v->m_type,
            array_types_decls, c_ds_api, bind_py_utils_functions);
        std::string ret_assign = indent + "_lpython_return_variable = " + py_val_cnvrt + "(pValue);";
        std::string clear_pValue = indent + "Py_DECREF(pValue);";
        std::string ret_stmt = indent + "return _lpython_return_variable;";
        return ret_var_decl + ret_assign + clear_pValue + ret_stmt + "\n";
    }

    std::string get_func_body_bind_python(const ASR::Function_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string var_decls = "PyObject *pName, *pModule, *pFunc; PyObject *pArgs, *pValue;\n";
        std::string func_body = R"(
    pName = PyUnicode_FromString(")" + std::string(x.m_module_file) + R"(");
    if (pName == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to convert to unicode string )" + std::string(x.m_module_file) + R"(\n");
        exit(1);
    }

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (pModule == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to load python module )" + std::string(x.m_module_file) + R"(\n");
        exit(1);
    }

    pFunc = PyObject_GetAttrString(pModule, ")" + std::string(x.m_name) + R"(");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        if (PyErr_Occurred()) PyErr_Print();
        fprintf(stderr, "Cannot find function )" + std::string(x.m_name) + R"(\n");
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
        exit(1);
    }
)" + get_arg_conv_bind_python(x) + R"(
    pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
    if (pValue == NULL) {
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        PyErr_Print();
        fprintf(stderr,"Call failed\n");
        exit(1);
    }
)" + get_return_value_conv_bind_python(x);
        return "{\n" + indent + var_decls + func_body + "}\n";
    }

    std::string declare_all_functions(const SymbolTable &scope) {
        std::string code, t;
        for (auto &item : scope.get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                t = declare_all_functions(*s->m_symtab);
                bool has_typevar = false;
                std::string decl = get_function_declaration(*s, has_typevar, false, false);
                if (!has_typevar && !decl.empty()) {
                    code += t + decl + ";\n";
                } else {
                    code += t;
                }
            }
        }
        return code;
    }

    std::string get_type_format(ASR::ttype_t *type) {
        // See: https://docs.python.org/3/c-api/arg.html for more info on `type format`
        switch (type->type) {
            case ASR::ttypeType::Integer: {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(type);
                if (a_kind == 4) {
                    return "i";
                } else {
                    return "l";
                }
            } case ASR::ttypeType::Real : {
                int a_kind = ASRUtils::extract_kind_from_ttype_t(type);
                if (a_kind == 4) {
                    return "f";
                } else {
                    return "d";
                }
            } case ASR::ttypeType::Logical : {
                return "p";
            } case ASR::ttypeType::Array : {
                return "O";
            } default: {
                throw CodeGenError("CPython type format not supported yet");
            }
        }
    }

    void visit_Function(const ASR::Function_t &x) {
        std::string sub = "";
        std::set<uint64_t> emitted_nested_functions;
        auto emit_nested_functions = [&]() -> bool {
            bool emitted_any = false;
            for (auto &item : x.m_symtab->get_scope()) {
                if (!ASR::is_a<ASR::Function_t>(*item.second)) {
                    continue;
                }
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(item.second);
                uint64_t key = get_hash(reinterpret_cast<ASR::asr_t*>(f));
                if (!emitted_nested_functions.insert(key).second) {
                    continue;
                }
                visit_Function(*f);
                sub += src + "\n";
                emitted_any = true;
            }
            return emitted_any;
        };
        emit_nested_functions();

        current_body = "";
        current_function_has_explicit_return = false;
        current_function_heap_array_data.clear();
        current_function_conditional_heap_array_data.clear();
        current_function_local_allocatable_arrays.clear();
        current_function_local_allocatable_array_structs.clear();
        current_function_local_allocatable_array_strings.clear();
        current_function_local_allocatable_strings.clear();
        current_function_local_allocatable_scalars.clear();
        current_function_local_character_strings.clear();
        current_function_local_allocatable_structs.clear();
        current_function_local_structs.clear();
        current_function_local_descriptors.clear();
        current_function_array_descriptor_cache.clear();
        current_function_pow_cache.clear();
        current_function_lazy_automatic_array_storage.clear();
        SymbolTable* current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        if (std::string(x.m_name) == "size" && intrinsic_module ) {
            // Intrinsic function `size`
            SymbolInfo s;
            s.intrinsic_function = true;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
            src = "";
            return;
        } else if ((
                std::string(x.m_name) == "int" ||
                std::string(x.m_name) == "char" ||
                std::string(x.m_name) == "present" ||
                std::string(x.m_name) == "len" ||
                std::string(x.m_name) == "cabs" ||
                std::string(x.m_name) == "cacos" ||
                std::string(x.m_name) == "cacosh" ||
                std::string(x.m_name) == "casin" ||
                std::string(x.m_name) == "casinh" ||
                std::string(x.m_name) == "catan" ||
                std::string(x.m_name) == "catanh" ||
                std::string(x.m_name) == "ccos" ||
                std::string(x.m_name) == "ccosh" ||
                std::string(x.m_name) == "cexp" ||
                std::string(x.m_name) == "clog" ||
                std::string(x.m_name) == "csin" ||
                std::string(x.m_name) == "csinh" ||
                std::string(x.m_name) == "csqrt" ||
                std::string(x.m_name) == "ctan" ||
                std::string(x.m_name) == "ctanh" ||
                std::string(x.m_name) == "not"
                ) && intrinsic_module) {
            // Intrinsic function `int`
            SymbolInfo s;
            s.intrinsic_function = true;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
            src = "";
            return;
        } else {
            SymbolInfo s;
            s.intrinsic_function = false;
            sym_info[get_hash((ASR::asr_t*)&x)] = s;
        }
        bool has_typevar = false;
        ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(x);
        sub += get_function_declaration(x, has_typevar);
        if (has_typevar) {
            src = "";
            return;
        }
        if (is_c && std::string(x.m_name).find("_lcompilers_stringconcat") != std::string::npos) {
            sub += "\n{\n";
            indentation_level += 1;
            std::string indent(indentation_level*indentation_spaces, ' ');
            sub += indent
                + "(*concat_result) = _lfortran_strcat_alloc(_lfortran_get_default_allocator(), "
                + "s1, s1_len, s2, s2_len);\n";
            indentation_level -= 1;
            sub += "}\n";
            src = sub;
            current_scope = current_scope_copy;
            return;
        }
        bool generate_body = true;
        if (f_type->m_deftype == ASR::deftypeType::Interface) {
            if (is_c && f_type->m_abi == ASR::abiType::Source) {
                src = "";
                return;
            }
            generate_body = false;
            if (f_type->m_abi == ASR::abiType::BindC) {
                if (x.m_module_file) {
                    user_headers.insert(std::string(x.m_module_file));
                    src = "";
                    return;
                } else {
                    sub += ";\n";
                }
            } else if (f_type->m_abi == ASR::abiType::BindPython) {
                indentation_level += 1;
                sub += "\n" + get_func_body_bind_python(x);
                indentation_level -= 1;
            } else {
                sub += ";\n";
            }
        }
        if( generate_body ) {
            sub += "\n";

            indentation_level += 1;
            std::string indent(indentation_level*indentation_spaces, ' ');
            std::string decl;
            std::string nested_function_decls = declare_all_functions(*x.m_symtab);
            if (!nested_function_decls.empty()) {
                std::istringstream nested_decl_stream(nested_function_decls);
                std::string nested_decl_line;
                while (std::getline(nested_decl_stream, nested_decl_line)) {
                    decl += indent + nested_decl_line + "\n";
                }
            }
            std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
            current_return_var_name.clear();
            std::string heuristic_return_var_name;
            ASR::ttype_t *function_return_type = nullptr;
            if (x.m_return_var) {
                function_return_type = ASRUtils::expr_type(x.m_return_var);
            } else if (f_type->m_return_var_type) {
                function_return_type = f_type->m_return_var_type;
            }
            std::string function_prefix = CUtils::sanitize_c_identifier(x.m_name) + "__";
            for (auto &item : var_order) {
                ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
                if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
                    std::string emitted_name = CUtils::get_c_variable_name(*v);
                    if (v->m_intent == ASRUtils::intent_local ||
                        v->m_intent == ASRUtils::intent_return_var) {
                        std::string d = indent + self().convert_variable_decl(*v) + ";\n";
                        decl += check_tmp_buffer() + d;
                        if (is_c && v->m_intent == ASRUtils::intent_local
                                && ASRUtils::is_allocatable(v->m_type)
                                && ASRUtils::is_array(v->m_type)) {
                            register_current_function_local_allocatable_array_cleanup(
                                emitted_name);
                            ASR::ttype_t *v_type_unwrapped =
                                ASRUtils::type_get_past_allocatable_pointer(v->m_type);
                            ASR::ttype_t *element_type =
                                ASRUtils::extract_type(v_type_unwrapped);
                            if (element_type != nullptr
                                    && ASRUtils::is_character(*element_type)) {
                                register_current_function_local_allocatable_array_string_cleanup(
                                    emitted_name);
                            } else if (element_type != nullptr
                                    && ASR::is_a<ASR::StructType_t>(*element_type)
                                    && v->m_type_declaration != nullptr) {
                                ASR::symbol_t *struct_sym =
                                    ASRUtils::symbol_get_past_external(v->m_type_declaration);
                                if (struct_sym != nullptr
                                        && ASR::is_a<ASR::Struct_t>(*struct_sym)
                                        && c_struct_has_member_cleanup(
                                            ASR::down_cast<ASR::Struct_t>(struct_sym))) {
                                    register_current_function_local_allocatable_array_struct_cleanup(
                                        emitted_name,
                                        ASR::down_cast<ASR::Struct_t>(struct_sym));
                                }
                            }
                        } else if (is_c && v->m_intent == ASRUtils::intent_local
                                && ASRUtils::is_allocatable(v->m_type)
                                && !ASRUtils::is_array(v->m_type)) {
                            ASR::ttype_t *v_type_unwrapped =
                                ASRUtils::type_get_past_allocatable_pointer(v->m_type);
                            if (ASRUtils::is_character(*v_type_unwrapped)) {
                                register_current_function_local_allocatable_string_cleanup(
                                    emitted_name);
                            } else if ((!is_c_compiler_generated_temporary_name(emitted_name)
                                        || is_c_owned_function_call_struct_temp_name(emitted_name))
                                    && ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)
                                    && v->m_type_declaration != nullptr) {
                                ASR::symbol_t *struct_sym =
                                    ASRUtils::symbol_get_past_external(v->m_type_declaration);
                                if (struct_sym != nullptr
                                        && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                                    register_current_function_local_allocatable_struct_cleanup(
                                        emitted_name,
                                        ASR::down_cast<ASR::Struct_t>(struct_sym),
                                        ASRUtils::is_class_type(v_type_unwrapped),
                                        is_c_owned_function_call_struct_temp_name(
                                            emitted_name));
                                }
                            } else if (!is_c_compiler_generated_temporary_name(emitted_name)) {
                                register_current_function_local_allocatable_scalar_cleanup(
                                    emitted_name);
                            }
                        } else if (is_c && v->m_intent == ASRUtils::intent_local
                                && !ASRUtils::is_allocatable(v->m_type)
                                && !ASRUtils::is_pointer(v->m_type)
                                && !ASRUtils::is_array(v->m_type)
                                && v->m_storage != ASR::storage_typeType::Parameter
                                && v->m_symbolic_value == nullptr) {
                            ASR::ttype_t *v_type_unwrapped =
                                ASRUtils::type_get_past_allocatable_pointer(v->m_type);
                            if (ASRUtils::is_character(*v_type_unwrapped)) {
                                register_current_function_local_character_string_cleanup(
                                    emitted_name);
                            } else if (ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)
                                    && !ASRUtils::is_class_type(v_type_unwrapped)
                                    && v->m_type_declaration != nullptr) {
                                ASR::symbol_t *struct_sym =
                                    ASRUtils::symbol_get_past_external(v->m_type_declaration);
                                if (struct_sym != nullptr
                                        && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                                    bool free_member_array_descriptors =
                                        should_free_c_local_struct_member_array_descriptors(
                                            *v, emitted_name);
                                    bool free_scalar_member_array_descriptors =
                                        should_free_c_local_struct_scalar_member_array_descriptors(
                                            *v, emitted_name);
                                    register_current_function_local_struct_cleanup(
                                        get_c_local_struct_cleanup_target(*v, emitted_name),
                                        ASR::down_cast<ASR::Struct_t>(struct_sym),
                                        free_member_array_descriptors,
                                        free_scalar_member_array_descriptors);
                                }
                            }
                        }
                    }
                    if (v->m_intent == ASRUtils::intent_return_var) {
                        current_return_var_name = emitted_name;
                    } else if (current_return_var_name.empty()
                            && function_return_type
                            && emitted_name.rfind(function_prefix, 0) == 0
                            && ASRUtils::check_equal_type(v->m_type, function_return_type, nullptr, nullptr)) {
                        heuristic_return_var_name = emitted_name;
                    }
                    if (ASR::is_a<ASR::TypeParameter_t>(*v->m_type)) {
                        has_typevar = true;
                        break;
                    }
                }
            }
            if (current_return_var_name.empty()) {
                current_return_var_name = heuristic_return_var_name;
            }
            if (has_typevar) {
                indentation_level -= 1;
                src = "";
                current_return_var_name.clear();
                return;
            }

            current_function = &x;
            self().emit_function_arg_initialization(x, current_body, indent);
            decl += emit_current_function_array_descriptor_cache_decls(x, indent);

            for (size_t i=0; i<x.n_body; i++) {
                self().visit_stmt(*x.m_body[i]);
                current_body += src;
            }
            decl += check_tmp_buffer();
            current_function = nullptr;
            bool visited_return = false;

            if (x.n_body > 0 && ASR::is_a<ASR::Return_t>(*x.m_body[x.n_body-1])) {
                visited_return = true;
            }

            std::string function_cleanup = emit_current_function_heap_array_cleanup(indent);
            if (is_c && current_function_has_explicit_return) {
                std::string marker = "/* __lfortran_return_cleanup_marker__ */\n";
                size_t pos = 0;
                while ((pos = current_body.find(marker, pos)) != std::string::npos) {
                    current_body.replace(pos, marker.size(), function_cleanup);
                    pos += function_cleanup.size();
                }
            }

            if (!visited_return && !current_return_var_name.empty()) {
                current_body += function_cleanup;
                current_body += indent + "return " + get_current_return_var_name() + ";\n";
            } else if (!visited_return) {
                current_body += function_cleanup;
            }

            if (is_c
                    && std::string(x.m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                    && sub.find("(void* to)") != std::string::npos) {
                std::string from_pat = "(*to)";
                std::string to_pat = "(*((void**)to))";
                size_t pos = 0;
                while ((pos = current_body.find(from_pat, pos)) != std::string::npos) {
                    current_body.replace(pos, from_pat.size(), to_pat);
                    pos += to_pat.size();
                }
            }

            cache_c_default_allocator(decl, current_body, indent);

            if (decl.size() > 0 || current_body.size() > 0) {
                sub += "{\n" + decl + current_body + "}\n";
            } else {
                std::string empty_body = "";
                if (!current_return_var_name.empty()) {
                    empty_body = indent + "return " + get_current_return_var_name() + ";\n";
                }
                sub += "{\n" + empty_body + "}\n";
            }
            current_return_var_name.clear();
            current_function_has_explicit_return = false;
            current_function_heap_array_data.clear();
            current_function_conditional_heap_array_data.clear();
            current_function_local_allocatable_arrays.clear();
            current_function_local_allocatable_array_structs.clear();
            current_function_local_allocatable_array_strings.clear();
            current_function_local_allocatable_strings.clear();
            current_function_local_allocatable_scalars.clear();
            current_function_local_character_strings.clear();
            current_function_local_allocatable_structs.clear();
            current_function_local_structs.clear();
            current_function_local_descriptors.clear();
            current_function_array_descriptor_cache.clear();
            current_function_pow_cache.clear();
            current_function_lazy_automatic_array_storage.clear();
            indentation_level -= 1;
        }
        sub += "\n";
        std::string pass_array_by_data_wrapper = get_pass_array_by_data_wrapper(x);
        if (!pass_array_by_data_wrapper.empty()) {
            sub += pass_array_by_data_wrapper + "\n";
        }
        // Some helpers, such as lowered intrinsic wrappers, are materialized in
        // the function symbol table while visiting the body. Emit any such late
        // additions now so split-C callers do not see declarations without
        // matching definitions.
        while (emit_nested_functions()) {
        }
        std::string tbp_registration_wrapper = emit_c_tbp_registration_wrapper(x);
        if (!tbp_registration_wrapper.empty()) {
            sub += tbp_registration_wrapper + "\n";
        }
        src = sub;
        if (f_type->m_deftype == ASR::deftypeType::Implementation) {
            if (f_type->m_abi == ASR::abiType::BindC && x.m_module_file) {
                std::string header_name = std::string(x.m_module_file);
                user_headers.insert(header_name);
                emit_headers[header_name]+= "\n" + src;
                src = "";
            } else if (f_type->m_abi == ASR::abiType::BindPython) {
                indentation_level += 1;
                headers.insert("Python.h");
                std::string variables_decl = ""; // Stores the argument declarations
                std::string fill_parse_args_details = "";
                std::string type_format = "";
                std::string fn_args = "";
                std::string fill_array_details = "";
                std::string numpy_init = "";

                for (size_t i = 0; i < x.n_args; i++) {
                    ASR::Variable_t *arg = ASRUtils::EXPR2VAR(x.m_args[i]);
                    std::string arg_name = arg->m_name;
                    fill_parse_args_details += "&" + arg_name;
                    type_format += get_type_format(arg->m_type);
                    if (ASR::is_a<ASR::Array_t>(*arg->m_type)) {
                        if (numpy_init.size() == 0) {
                            numpy_init = R"(
    // Initialize NumPy
    import_array();
)";
                            // Insert the headers for array handling
                            headers.insert("numpy/ndarrayobject.h");
                            user_defines.insert("NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION");
                        }
    // -------------------------------------------------------------------------
    // `PyArray_AsCArray` is used to convert NumPy Arrays to C Arrays
    // `fill_array_details` contains array operations to be performed on the arguments
    // `fill_parse_args_details` are used to capture the args from CPython
    // `fn_args` are the arguments that are passed to the shared library function
                        std::string c_array_type = self().convert_variable_decl(*arg);
                        c_array_type = c_array_type.substr(0,
                            c_array_type.size() - arg_name.size() - 2);
                        fn_args += "s_array_" + arg_name;
                        variables_decl += "    PyArrayObject *" + arg_name + ";\n";

                        fill_array_details += "\n    // Fill array details for " + arg_name
    + "\n    if (PyArray_NDIM(" + arg_name + R"() != 1) {
        PyErr_SetString(PyExc_TypeError, "An error occurred in the `lpython` decorator: "
            "Only 1 dimension array is supported for now.");
        return NULL;
    }

    )" + c_array_type + " *s_array_" + arg_name + " = malloc(sizeof(" + c_array_type + R"());
    {
        )" + CUtils::get_c_type_from_ttype_t(arg->m_type) + R"( *array;
        // Create C arrays from numpy objects:
        PyArray_Descr *descr = PyArray_DescrFromType(PyArray_TYPE()" + arg_name + R"());
        npy_intp dims[1];
        if (PyArray_AsCArray((PyObject **)&)" + arg_name + R"(, (void *)&array, dims, 1, descr) < 0) {
            PyErr_SetString(PyExc_TypeError, "An error occurred in the `lpython` decorator: "
                "Failed to create a C array");
            return NULL;
        }

        s_array_)" + arg_name + R"(->data = array;
        s_array_)" + arg_name + R"(->n_dims = 1;
        s_array_)" + arg_name + R"(->dims[0].lower_bound = 0;
        s_array_)" + arg_name + R"(->dims[0].length = dims[0];
        s_array_)" + arg_name + R"(->dims[0].stride = 1;
        s_array_)" + arg_name + R"(->offset = 0;
        s_array_)" + arg_name + R"(->is_allocated = false;
    }
)";
                    } else {
                        fn_args += arg_name;
                        variables_decl += "    " + self().convert_variable_decl(*arg)
                            + ";\n";
                    }
                    if (i < x.n_args - 1) {
                        fill_parse_args_details += ", ";
                        fn_args += ", ";
                    }
                }

                if (fill_parse_args_details.size() > 0) {
                    fill_parse_args_details = R"(
    // Parse the arguments from Python
    if (!PyArg_ParseTuple(args, ")" + type_format + R"(", )" + fill_parse_args_details + R"()) {
        PyErr_SetString(PyExc_TypeError, "An error occurred in the `lpython` decorator: "
            "Failed to parse or receive arguments from Python");
        return NULL;
    }
)";
                }

                std::string fn_name = x.m_name;
                std::string fill_return_details = "\n    // Call the C function";
                if (variables_decl.size() > 0) {
                    variables_decl.insert(0, "\n    "
                    "// Declare arguments and return variable\n");
                }
                // Handle the return variable if any; otherwise, return None
                if(x.m_return_var) {
                    ASR::Variable_t *return_var = ASRUtils::EXPR2VAR(x.m_return_var);
                    variables_decl += "    " + self().convert_variable_decl(*return_var)
                        + ";\n";
                    fill_return_details += "\n    _lpython_return_variable = _xx_internal_"
                        + fn_name + "_xx(" + fn_args + ");\n";
                    if (ASR::is_a<ASR::Array_t>(*return_var->m_type)) {
                        ASR::Array_t *arr = ASR::down_cast<ASR::Array_t>(return_var->m_type);
                        if(arr->m_dims[0].m_length &&
                                ASR::is_a<ASR::Var_t>(*arr->m_dims[0].m_length)) {
                            // name() -> f64[n]: Extract `array_type` and `n`
                            std::string array_type
                                = BindPyUtils::get_numpy_c_obj_type_conv_func_from_ttype_t(arr->m_type);
                            std::string return_array_size = ASRUtils::EXPR2VAR(
                                arr->m_dims[0].m_length)->m_name;
                            fill_return_details += R"(
    // Copy the array elements and return the result as a Python object
    {
        npy_intp dims[] = {)" + return_array_size + R"(};
        PyObject* numpy_array = PyArray_SimpleNewFromData(1, dims, )" + array_type + R"(,
            _lpython_return_variable->data);
        if (numpy_array == NULL) {
            PyErr_SetString(PyExc_TypeError, "An error occurred in the `lpython` decorator: "
                "Failed to create an array that was used as a return variable");
            return NULL;
        }
        return numpy_array;
    })";
                        } else {
                            throw CodeGenError("Array return type without a length is not supported yet");
                        }
                    } else {
                        fill_return_details += R"(
    // Build and return the result as a Python object
    return Py_BuildValue(")" + get_type_format(return_var->m_type)
                            + "\", _lpython_return_variable);";
                    }
                } else {
                    fill_return_details += R"(
    _xx_internal_)" + fn_name + "_xx(" + fn_args + ");\n" + R"(
    // Return None
    Py_RETURN_NONE;)";
                }
                // `sub` contains the function to be called
                src = sub;
// Python wrapper for the Shared library
// TODO: Instead of a function call replace it with the function body
// Basically, inlining the function by hand
                src += R"(// Define the Python module and method mappings
static PyObject* )" + fn_name + R"((PyObject* self, PyObject* args) {)"
    + numpy_init + variables_decl + fill_parse_args_details
    + fill_array_details + fill_return_details + R"(
}

// Define the module's method table
static PyMethodDef )" + fn_name + R"(_module_methods[] = {
    {")" + fn_name + R"(", )" + fn_name + R"(, METH_VARARGS,
        "Handle arguments & return variable and call the function"},
    {NULL, NULL, 0, NULL}
};

// Define the module initialization function
static struct PyModuleDef )" + fn_name + R"(_module_def = {
    PyModuleDef_HEAD_INIT,
    "lpython_module_)" + fn_name + R"(",
    "Shared library to use LPython generated functions",
    -1,
    )" + fn_name + R"(_module_methods
};

PyMODINIT_FUNC PyInit_lpython_module_)" + fn_name + R"((void) {
    PyObject* module;

    // Create the module object
    module = PyModule_Create(&)" + fn_name + R"(_module_def);
    if (!module) {
        return NULL;
    }

    return module;
}

)";
            indentation_level -= 1;
            }
        }
        current_scope = current_scope_copy;
    }

    void visit_ArrayPhysicalCast(const ASR::ArrayPhysicalCast_t& x) {
        src = "";
        this->visit_expr(*x.m_arg);
        if (x.m_old == ASR::array_physical_typeType::FixedSizeArray &&
                x.m_new == ASR::array_physical_typeType::SIMDArray) {
            std::string arr_element_type = CUtils::get_c_type_from_ttype_t(ASRUtils::expr_type(x.m_arg));
            int64_t size = ASRUtils::get_fixed_size_of_array(ASRUtils::expr_type(x.m_arg));
            std::string cast = arr_element_type + " __attribute__ (( vector_size(sizeof("
                + arr_element_type + ") * " + std::to_string(size) + ") ))";
            src = "(" + cast + ") " + src;
        } else if (is_c && ASRUtils::is_array(x.m_type)
                && ASRUtils::is_array(ASRUtils::expr_type(x.m_arg))) {
            ASR::ttype_t *source_type = ASRUtils::expr_type(x.m_arg);
            ASR::symbol_t *source_type_decl = get_expr_type_declaration_symbol(x.m_arg);
            ASR::expr_t *source_lvalue = unwrap_c_lvalue_expr(x.m_arg);
            if (source_lvalue != nullptr
                    && ASR::is_a<ASR::ArraySection_t>(*source_lvalue)) {
                std::string view_src = build_c_array_no_copy_descriptor_view(
                    x.m_type, x.m_arg, src, nullptr, false);
                if (view_src != src) {
                    src = view_src;
                    return;
                }
            }
            if ((is_data_only_array_expr(x.m_arg)
                        || is_fixed_size_array_storage_expr(x.m_arg))) {
                src = build_c_array_wrapper_from_cast_target(x.m_type, x.m_arg, src);
            } else if (are_compatible_c_array_wrapper_types(x.m_type, source_type)) {
                src = cast_c_array_wrapper_ptr_to_target_type(
                    x.m_type, source_type, src, nullptr, source_type_decl);
            } else {
                src = build_c_array_wrapper_from_cast_target(x.m_type, x.m_arg, src);
            }
        }
     }

    ASR::Function_t* get_procedure_interface_function(ASR::symbol_t *sym) {
        ASR::symbol_t *base_sym = ASRUtils::symbol_get_past_external(sym);
        if (ASR::is_a<ASR::Function_t>(*base_sym)) {
            return ASR::down_cast<ASR::Function_t>(base_sym);
        }
        if (ASR::is_a<ASR::StructMethodDeclaration_t>(*base_sym)) {
            ASR::StructMethodDeclaration_t *meth =
                ASR::down_cast<ASR::StructMethodDeclaration_t>(base_sym);
            ASR::symbol_t *proc_sym = ASRUtils::symbol_get_past_external(meth->m_proc);
            if (ASR::is_a<ASR::Function_t>(*proc_sym)) {
                return ASR::down_cast<ASR::Function_t>(proc_sym);
            }
            return nullptr;
        }
        if (!ASR::is_a<ASR::Variable_t>(*base_sym)) {
            return nullptr;
        }
        ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(base_sym);
        if (!var->m_type_declaration) {
            return nullptr;
        }
        ASR::symbol_t *iface_sym = ASRUtils::symbol_get_past_external(var->m_type_declaration);
        if (ASR::is_a<ASR::Function_t>(*iface_sym)) {
            return ASR::down_cast<ASR::Function_t>(iface_sym);
        }
        return nullptr;
    }

    ASR::expr_t* unwrap_c_lvalue_expr(ASR::expr_t *expr) {
        while (expr != nullptr) {
            if (ASR::is_a<ASR::Cast_t>(*expr)) {
                expr = ASR::down_cast<ASR::Cast_t>(expr)->m_arg;
            } else if (ASR::is_a<ASR::BitCast_t>(*expr)) {
                expr = ASR::down_cast<ASR::BitCast_t>(expr)->m_source;
            } else if (ASR::is_a<ASR::GetPointer_t>(*expr)) {
                expr = ASR::down_cast<ASR::GetPointer_t>(expr)->m_arg;
            } else if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr)->m_arg;
            } else {
                break;
            }
        }
        return expr;
    }

    ASR::expr_t* get_array_index_expr(const ASR::array_index_t &idx) {
        if (idx.m_right) {
            return idx.m_right;
        }
        if (idx.m_left) {
            return idx.m_left;
        }
        return nullptr;
    }

    bool is_pointer_backed_struct_expr(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *expr_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (!(ASR::is_a<ASR::StructType_t>(*expr_type) || ASRUtils::is_class_type(expr_type))) {
            return false;
        }
        if (ASR::is_a<ASR::StructConstructor_t>(*expr)
                || ASR::is_a<ASR::StructConstant_t>(*expr)) {
            return false;
        }
        if (ASR::is_a<ASR::Var_t>(*expr)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(expr)->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                return false;
            }
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(sym);
            bool force_value_struct_temp =
                std::string(v->m_name).find("__libasr_created__struct_constructor_") != std::string::npos ||
                std::string(v->m_name).find("temp_struct_var__") != std::string::npos;
            std::string emitted_name = CUtils::get_c_variable_name(*v);
            bool emitted_pointer_backed_struct =
                emitted_pointer_backed_struct_names.find(emitted_name)
                != emitted_pointer_backed_struct_names.end();
            ASR::asr_t *owner = v->m_parent_symtab ? v->m_parent_symtab->asr_owner : nullptr;
            return ASRUtils::is_arg_dummy(v->m_intent)
                || ASRUtils::is_pointer(v->m_type)
                || ASRUtils::is_allocatable(v->m_type)
                || emitted_pointer_backed_struct
                || (owner
                    && !force_value_struct_temp
                    && (CUtils::is_symbol_owner<ASR::Function_t>(owner)
                        || CUtils::is_symbol_owner<ASR::Block_t>(owner)
                        || CUtils::is_symbol_owner<ASR::AssociateBlock_t>(owner))
                    && (v->m_intent == ASRUtils::intent_local
                        || v->m_intent == ASRUtils::intent_return_var));
        }
        if (ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            ASR::StructInstanceMember_t *member = ASR::down_cast<ASR::StructInstanceMember_t>(expr);
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(member->m_m);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(sym);
                ASR::ttype_t *member_type = ASRUtils::type_get_past_allocatable_pointer(v->m_type);
                return ASRUtils::is_pointer(v->m_type)
                    || ASRUtils::is_allocatable(v->m_type)
                    || ASRUtils::is_class_type(member_type);
            }
        }
        if (ASR::is_a<ASR::ArrayItem_t>(*expr)) {
            return false;
        }
        return false;
    }

    bool is_scalar_allocatable_storage_type(ASR::ttype_t *type) {
        if (type == nullptr || !ASR::is_a<ASR::Allocatable_t>(*type)) {
            return false;
        }
        ASR::ttype_t *value_type = ASRUtils::type_get_past_allocatable_pointer(type);
        return value_type != nullptr
            && !ASRUtils::is_array(type)
            && !ASRUtils::is_character(*value_type)
            && !ASRUtils::is_aggregate_type(value_type);
    }

    std::string coerce_c_struct_value_for_target(ASR::ttype_t *target_type,
            ASR::symbol_t *target_type_decl, ASR::expr_t *value_expr,
            std::string value_src) {
        if (!is_c || target_type == nullptr || value_expr == nullptr) {
            return value_src;
        }
        ASR::ttype_t *target_value_type =
            ASRUtils::type_get_past_allocatable_pointer(target_type);
        if (!(ASR::is_a<ASR::StructType_t>(*target_value_type)
                || ASRUtils::is_class_type(target_value_type))) {
            return value_src;
        }
        bool target_is_pointer_backed =
            ASRUtils::is_pointer(target_type)
            || ASRUtils::is_allocatable(target_type)
            || ASRUtils::is_class_type(target_value_type);
        bool value_is_pointer_backed = is_pointer_backed_struct_expr(value_expr);
        if (!target_is_pointer_backed && value_is_pointer_backed) {
            return "(*(" + value_src + "))";
        }
        if (target_is_pointer_backed && !value_is_pointer_backed) {
            ASR::expr_t *unwrapped_value_expr = unwrap_c_lvalue_expr(value_expr);
            ASR::symbol_t *value_struct_sym = ASRUtils::symbol_get_past_external(
                ASRUtils::get_struct_sym_from_struct_expr(unwrapped_value_expr));
            std::string target_type_name = get_c_concrete_type_from_ttype_t(
                target_value_type, target_type_decl);
            if ((ASR::is_a<ASR::StructConstructor_t>(*unwrapped_value_expr)
                    || ASR::is_a<ASR::StructConstant_t>(*unwrapped_value_expr))
                    && value_struct_sym
                    && ASR::is_a<ASR::Struct_t>(*value_struct_sym)) {
                if (!target_type_name.empty() && target_type_name != "void*") {
                    return "((" + target_type_name + "*)(&(" + value_src + ")))";
                }
                return "&(" + value_src + ")";
            }
            if (!target_type_name.empty() && target_type_name != "void*") {
                return "((" + target_type_name + "*)(&(" + value_src + ")))";
            }
            return "&(" + value_src + ")";
        }
        return value_src;
    }

    std::string get_c_var_storage_name(ASR::Variable_t *sv) {
        ASR::asr_t *owner = sv->m_parent_symtab ? sv->m_parent_symtab->asr_owner : nullptr;
        bool use_local_name = owner == nullptr
            || ASRUtils::is_arg_dummy(sv->m_intent)
            || CUtils::is_symbol_owner<ASR::Function_t>(owner)
            || CUtils::is_symbol_owner<ASR::Program_t>(owner)
            || CUtils::is_symbol_owner<ASR::Block_t>(owner)
            || CUtils::is_symbol_owner<ASR::Struct_t>(owner)
            || CUtils::is_symbol_owner<ASR::Union_t>(owner)
            || CUtils::is_symbol_owner<ASR::Enum_t>(owner);
        if (use_local_name) {
            return CUtils::sanitize_c_identifier(sv->m_name);
        }
        return CUtils::get_c_symbol_name(reinterpret_cast<ASR::symbol_t*>(sv));
    }

    std::string get_c_mutable_scalar_expr(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr != nullptr && ASR::is_a<ASR::Var_t>(*expr)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(expr)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::Variable_t *sv = ASR::down_cast<ASR::Variable_t>(sym);
                std::string var_name = get_c_var_storage_name(sv);
                ASR::ttype_t *value_type = ASRUtils::type_get_past_allocatable_pointer(sv->m_type);
                if (value_type != nullptr
                        && ASRUtils::is_character(*value_type)
                        && !ASRUtils::is_array(sv->m_type)
                        && (ASRUtils::is_arg_dummy(sv->m_intent)
                            || sv->m_intent == ASRUtils::intent_inout
                            || sv->m_intent == ASRUtils::intent_out)) {
                    return "(*" + var_name + ")";
                }
                if (is_scalar_allocatable_storage_type(sv->m_type)) {
                    if (is_scalar_allocatable_dummy_slot_type(sv)) {
                        return "(*" + var_name + ")";
                    }
                    return var_name;
                }
            }
        }
        self().visit_expr(*expr);
        return src;
    }

    bool is_c_component_array_expr(const ASR::StructInstanceMember_t &x) {
        if (!is_c || !ASRUtils::is_array(x.m_type)) {
            return false;
        }
        ASR::expr_t *base_expr = x.m_v;
        if (base_expr != nullptr && ASR::is_a<ASR::ArrayPhysicalCast_t>(*base_expr)) {
            base_expr = ASR::down_cast<ASR::ArrayPhysicalCast_t>(base_expr)->m_arg;
        }
        ASR::ttype_t *base_type = ASRUtils::type_get_past_pointer(
            ASRUtils::type_get_past_allocatable(ASRUtils::expr_type(base_expr)));
        if (base_type == nullptr || !ASRUtils::is_array(base_type)) {
            return false;
        }
        ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(x.m_m);
        if (!ASR::is_a<ASR::Variable_t>(*member_sym)) {
            return false;
        }
        ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
        return !ASRUtils::is_array(member_var->m_type);
    }

    std::string get_c_component_array_expr(const ASR::StructInstanceMember_t &x,
            const std::string &base_expr) {
        ASR::ttype_t *base_type = ASRUtils::type_get_past_pointer(
            ASRUtils::type_get_past_allocatable(ASRUtils::expr_type(x.m_v)));
        LCOMPILERS_ASSERT(base_type != nullptr && ASRUtils::is_array(base_type));
        ASR::symbol_t *base_struct_sym = ASRUtils::symbol_get_past_external(
            ASRUtils::get_struct_sym_from_struct_expr(x.m_v));
        LCOMPILERS_ASSERT(base_struct_sym != nullptr);

        std::string wrapper_type = get_c_declared_array_wrapper_type_name(x.m_type);
        std::string member = CUtils::get_c_member_name(
            ASRUtils::symbol_get_past_external(x.m_m));
        std::string component_type = CUtils::get_c_array_element_type_from_ttype_t(x.m_type);
        std::string base_struct_type = "struct " + CUtils::get_c_symbol_name(base_struct_sym);
        std::string stride_multiplier = "((int32_t)(sizeof(" + base_struct_type
            + ") / sizeof(" + component_type + ")))";
        std::string member_offset = "((size_t)&(((" + base_struct_type
            + "*)0)->" + member + "))";
        std::string data_expr = "((" + component_type + "*)(((char*)(" + base_expr
            + ")->data) + sizeof(" + base_struct_type + ")*(" + base_expr
            + ")->offset + " + member_offset + "))";
        std::string dims_init = "{";
        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(base_type);
        for (size_t i = 0; i < n_dims; i++) {
            std::string dim_idx = std::to_string(i);
            dims_init += "{(" + base_expr + ")->dims[" + dim_idx + "].lower_bound, ("
                + base_expr + ")->dims[" + dim_idx + "].length, ("
                + base_expr + ")->dims[" + dim_idx + "].stride * "
                + stride_multiplier + "}";
            if (i + 1 < n_dims) {
                dims_init += ", ";
            }
        }
        dims_init += "}";
        return "(&(" + wrapper_type + "){.data = " + data_expr
            + ", .dims = " + dims_init
            + ", .n_dims = (" + base_expr + ")->n_dims"
            + ", .offset = 0"
            + ", .is_allocated = false})";
    }

    std::string get_struct_instance_member_expr(const ASR::StructInstanceMember_t& x,
            bool load_scalar_allocatable) {
        std::string der_expr, member;
        this->visit_expr(*x.m_v);
        der_expr = std::move(src);
        if (is_c_component_array_expr(x)) {
            return get_c_component_array_expr(x, der_expr);
        }
        ASR::expr_t *base_v = unwrap_c_lvalue_expr(x.m_v);
        bool plain_aggregate_dummy_pointee =
            emits_plain_aggregate_dummy_pointee_value(base_v ? base_v : x.m_v);
        member = CUtils::get_c_member_name(ASRUtils::symbol_get_past_external(x.m_m));
        ASR::ttype_t *v_type = ASRUtils::expr_type(x.m_v);
        ASR::ttype_t *v_type_unwrapped = ASRUtils::type_get_past_allocatable_pointer(v_type);
        bool var_is_byref = false;
        bool symbol_pointer_backed_struct = false;
        if (base_v && ASR::is_a<ASR::Var_t>(*base_v)) {
            ASR::symbol_t *v = ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Var_t>(base_v)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*v)) {
                ASR::Variable_t *vv = ASR::down_cast<ASR::Variable_t>(v);
                if (is_c && is_pointer_dummy_slot_type(vv)) {
                    ASR::ttype_t *ptr_target = ASRUtils::type_get_past_pointer(vv->m_type);
                    if (ptr_target != nullptr && ASRUtils::is_aggregate_type(ptr_target)) {
                        der_expr = "(*" + get_c_var_storage_name(vv) + ")";
                    }
                } else if (is_c && (is_aggregate_dummy_slot_type(vv)
                        || is_plain_aggregate_dummy_pointee_type(vv))) {
                    der_expr = "(*" + get_c_var_storage_name(vv) + ")";
                }
                var_is_byref = ASRUtils::is_arg_dummy(vv->m_intent);
                ASR::asr_t *owner = vv->m_parent_symtab ? vv->m_parent_symtab->asr_owner : nullptr;
                bool force_value_struct_temp =
                    std::string(vv->m_name).find("__libasr_created__struct_constructor_") != std::string::npos ||
                    std::string(vv->m_name).find("temp_struct_var__") != std::string::npos;
                symbol_pointer_backed_struct = var_is_byref
                    || ASRUtils::is_pointer(vv->m_type)
                    || ASRUtils::is_allocatable(vv->m_type)
                    || (!force_value_struct_temp
                        && owner
                        && (CUtils::is_symbol_owner<ASR::Function_t>(owner)
                            || CUtils::is_symbol_owner<ASR::Block_t>(owner)
                            || CUtils::is_symbol_owner<ASR::AssociateBlock_t>(owner))
                        && (vv->m_intent == ASRUtils::intent_local
                            || vv->m_intent == ASRUtils::intent_return_var));
            }
        }
        bool is_parameter_value = false;
        if (base_v && ASR::is_a<ASR::Var_t>(*base_v)) {
            ASR::symbol_t *v = ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Var_t>(base_v)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*v)) {
                ASR::Variable_t *vv = ASR::down_cast<ASR::Variable_t>(v);
                is_parameter_value = vv->m_storage == ASR::storage_typeType::Parameter;
            }
        }
        bool emitted_pointer_backed_struct = emitted_pointer_backed_struct_names.find(der_expr)
            != emitted_pointer_backed_struct_names.end();
        bool value_backed_struct = ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)
            && !(symbol_pointer_backed_struct
                || emitted_pointer_backed_struct
                || is_pointer_backed_struct_expr(base_v ? base_v : x.m_v));
        bool base_expr_pointer_backed = is_pointer_backed_struct_expr(base_v ? base_v : x.m_v);
        bool use_dot = (base_v && ASR::is_a<ASR::ArrayItem_t>(*base_v)) ||
            (base_v && ASR::is_a<ASR::UnionInstanceMember_t>(*base_v)) ||
            ((base_v && ASR::is_a<ASR::StructInstanceMember_t>(*base_v)) && !base_expr_pointer_backed) ||
            plain_aggregate_dummy_pointee ||
            value_backed_struct ||
            ASR::is_a<ASR::EnumType_t>(*v_type_unwrapped) ||
            (is_parameter_value && ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)) ||
            (!var_is_byref && !ASRUtils::is_pointer(v_type) && !ASRUtils::is_allocatable(v_type) &&
                ASR::is_a<ASR::EnumType_t>(*v_type_unwrapped));
        std::string member_expr = use_dot ? der_expr + "." + member : der_expr + "->" + member;
        ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(x.m_m);
        if (load_scalar_allocatable && ASR::is_a<ASR::Variable_t>(*member_sym)) {
            ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            if (is_scalar_allocatable_storage_type(member_var->m_type)) {
                return "(*(" + member_expr + "))";
            }
        }
        return member_expr;
    }

    std::string get_procedure_component_callee_expr(ASR::expr_t *dispatch_target,
            ASR::symbol_t *callee_sym) {
        LCOMPILERS_ASSERT(dispatch_target != nullptr);
        LCOMPILERS_ASSERT(callee_sym != nullptr);
        ASR::expr_t *dispatch_expr = unwrap_c_lvalue_expr(dispatch_target);
        if (dispatch_expr && ASR::is_a<ASR::StructInstanceMember_t>(*dispatch_expr)) {
            ASR::StructInstanceMember_t *member_expr =
                ASR::down_cast<ASR::StructInstanceMember_t>(dispatch_expr);
            if (ASRUtils::symbol_get_past_external(member_expr->m_m)
                    == ASRUtils::symbol_get_past_external(callee_sym)) {
                this->visit_expr(*dispatch_target);
                return std::move(src);
            }
        }
        this->visit_expr(*dispatch_target);
        std::string der_expr = std::move(src);
        ASR::expr_t *base_v = dispatch_expr;
        bool plain_aggregate_dummy_pointee =
            emits_plain_aggregate_dummy_pointee_value(base_v ? base_v : dispatch_target);
        ASR::ttype_t *v_type = ASRUtils::expr_type(dispatch_target);
        ASR::ttype_t *v_type_unwrapped = ASRUtils::type_get_past_allocatable_pointer(v_type);
        bool var_is_byref = false;
        bool symbol_pointer_backed_struct = false;
        if (base_v && ASR::is_a<ASR::Var_t>(*base_v)) {
            ASR::symbol_t *v = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(base_v)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*v)) {
                ASR::Variable_t *vv = ASR::down_cast<ASR::Variable_t>(v);
                var_is_byref = ASRUtils::is_arg_dummy(vv->m_intent);
                ASR::asr_t *owner = vv->m_parent_symtab ? vv->m_parent_symtab->asr_owner : nullptr;
                bool force_value_struct_temp =
                    std::string(vv->m_name).find("__libasr_created__struct_constructor_") != std::string::npos ||
                    std::string(vv->m_name).find("temp_struct_var__") != std::string::npos;
                symbol_pointer_backed_struct = var_is_byref
                    || ASRUtils::is_pointer(vv->m_type)
                    || ASRUtils::is_allocatable(vv->m_type)
                    || (!force_value_struct_temp
                        && owner
                        && (CUtils::is_symbol_owner<ASR::Function_t>(owner)
                            || CUtils::is_symbol_owner<ASR::Block_t>(owner)
                            || CUtils::is_symbol_owner<ASR::AssociateBlock_t>(owner))
                        && (vv->m_intent == ASRUtils::intent_local
                            || vv->m_intent == ASRUtils::intent_return_var));
            }
        }
        bool is_parameter_value = false;
        if (base_v && ASR::is_a<ASR::Var_t>(*base_v)) {
            ASR::symbol_t *v = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(base_v)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*v)) {
                ASR::Variable_t *vv = ASR::down_cast<ASR::Variable_t>(v);
                is_parameter_value = vv->m_storage == ASR::storage_typeType::Parameter;
            }
        }
        bool emitted_pointer_backed_struct = emitted_pointer_backed_struct_names.find(der_expr)
            != emitted_pointer_backed_struct_names.end();
        bool value_backed_struct = ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)
            && !(symbol_pointer_backed_struct
                || emitted_pointer_backed_struct
                || is_pointer_backed_struct_expr(base_v ? base_v : dispatch_target));
        bool base_expr_pointer_backed =
            is_pointer_backed_struct_expr(base_v ? base_v : dispatch_target);
        bool use_dot = (base_v && ASR::is_a<ASR::ArrayItem_t>(*base_v)) ||
            (base_v && ASR::is_a<ASR::UnionInstanceMember_t>(*base_v)) ||
            ((base_v && ASR::is_a<ASR::StructInstanceMember_t>(*base_v)) && !base_expr_pointer_backed) ||
            plain_aggregate_dummy_pointee ||
            value_backed_struct ||
            ASR::is_a<ASR::EnumType_t>(*v_type_unwrapped) ||
            (is_parameter_value && ASR::is_a<ASR::StructType_t>(*v_type_unwrapped)) ||
            (!var_is_byref && !ASRUtils::is_pointer(v_type) && !ASRUtils::is_allocatable(v_type) &&
                ASR::is_a<ASR::EnumType_t>(*v_type_unwrapped));
        std::string member = CUtils::get_c_member_name(ASRUtils::symbol_get_past_external(callee_sym));
        return use_dot ? der_expr + "." + member : der_expr + "->" + member;
    }

    std::string emit_c_array_constant_brace_init(ASR::expr_t *expr, ASR::ttype_t *target_type) {
        ASR::expr_t *value = ASRUtils::expr_value(expr);
        if (value == nullptr) {
            value = expr;
        }
        if (value == nullptr || !ASR::is_a<ASR::ArrayConstant_t>(*value)) {
            return "";
        }
        ASR::ttype_t *member_type = ASRUtils::type_get_past_pointer(
            ASRUtils::type_get_past_allocatable(target_type));
        if (member_type == nullptr || !ASR::is_a<ASR::Array_t>(*member_type)) {
            return "";
        }
        ASR::Array_t *array_type = ASR::down_cast<ASR::Array_t>(member_type);
        if (array_type->m_physical_type != ASR::array_physical_typeType::FixedSizeArray) {
            return "";
        }
        ASR::ArrayConstant_t *arr = ASR::down_cast<ASR::ArrayConstant_t>(value);
        size_t n = ASRUtils::get_fixed_size_of_array(member_type);
        std::string init = "{";
        for (size_t i = 0; i < n; i++) {
            init += get_c_array_constant_init_element_for_c_index(
                arr, member_type, array_type->m_type, i);
            if (i + 1 < n) {
                init += ", ";
            }
        }
        init += "}";
        return init;
    }

    size_t get_c_array_constant_element_size(ASR::ttype_t *element_type) {
        int64_t element_size = ASRUtils::get_type_byte_size(element_type);
        if (element_size <= 0) {
            return 0;
        }
        return static_cast<size_t>(element_size);
    }

    std::string emit_c_array_constant_byte_initializer(
            ASR::ArrayConstant_t *arr, ASR::ttype_t *array_type,
            ASR::ttype_t *element_type) {
        size_t element_size = get_c_array_constant_element_size(element_type);
        if (element_size == 0 || arr->m_data == nullptr) {
            return "";
        }
        size_t n = ASRUtils::get_fixed_size_of_array(array_type);
        const unsigned char *raw_data =
            reinterpret_cast<const unsigned char*>(arr->m_data);
        std::string init;
        size_t emitted = 0;
        for (size_t i = 0; i < n; i++) {
            size_t storage_index =
                get_c_array_constant_storage_index(arr, array_type, i);
            const unsigned char *element =
                raw_data + storage_index * element_size;
            for (size_t j = 0; j < element_size; j++) {
                if (emitted % 16 == 0) {
                    init += "        ";
                }
                init += std::to_string(static_cast<unsigned int>(element[j]));
                if (i + 1 < n || j + 1 < element_size) {
                    init += ", ";
                }
                emitted++;
                if (emitted % 16 == 0 || (i + 1 == n && j + 1 == element_size)) {
                    init += "\n";
                }
            }
        }
        return init;
    }

    void ensure_compact_constant_data_prelude() {
        if (!compact_constant_data_body.empty()) {
            return;
        }
        compact_constant_data_body +=
            "#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L\n"
            "#include <stdalign.h>\n"
            "#define LFORTRAN_CDATA_ALIGNAS(T) alignas(T)\n"
            "#elif defined(__GNUC__) || defined(__clang__)\n"
            "#define LFORTRAN_CDATA_ALIGNAS(T) __attribute__((aligned(__alignof__(T))))\n"
            "#else\n"
            "#define LFORTRAN_CDATA_ALIGNAS(T)\n"
            "#endif\n\n";
    }

    std::string register_compact_array_constant_data_helper(
            ASR::ArrayConstant_t *arr, ASR::ttype_t *array_type,
            ASR::ttype_t *element_type, const std::string &c_element_type) {
        std::string byte_init =
            emit_c_array_constant_byte_initializer(arr, array_type, element_type);
        if (byte_init.empty()) {
            return "";
        }
        size_t n = ASRUtils::get_fixed_size_of_array(array_type);
        size_t element_size = get_c_array_constant_element_size(element_type);
        std::string suffix = std::to_string(compact_constant_data_count++);
        if (!lcompilers_unique_ID_separate_compilation.empty()) {
            suffix += "_" + lcompilers_unique_ID_separate_compilation;
        }
        std::string helper_name = "__lfortran_const_copy_" + suffix;
        std::string blob_name = "__lfortran_const_blob_" + suffix;

        compact_constant_data_decls +=
            "void " + helper_name + "(void *dst, int64_t stride);\n";
        ensure_compact_constant_data_prelude();
        compact_constant_data_body +=
            "LFORTRAN_CDATA_ALIGNAS(" + c_element_type + ")\n"
            "static const unsigned char " + blob_name + "["
            + std::to_string(n * element_size) + "] = {\n"
            + byte_init + "};\n\n";
        compact_constant_data_body +=
            "void " + helper_name + "(void *dst, int64_t stride)\n"
            "{\n"
            "    unsigned char *dst_bytes = (unsigned char*) dst;\n"
            "    if (stride == 1) {\n"
            "        memcpy(dst_bytes, " + blob_name + ", sizeof(" + blob_name + "));\n"
            "        return;\n"
            "    }\n"
            "    for (int64_t i = 0; i < " + std::to_string(n) + "; i++) {\n"
            "        memcpy(dst_bytes + i * stride * sizeof(" + c_element_type + "), "
                + blob_name + " + i * sizeof(" + c_element_type + "), "
                "sizeof(" + c_element_type + "));\n"
            "    }\n"
            "}\n\n";
        return helper_name;
    }

    ASR::ArrayConstant_t *get_c_array_constant_expr(ASR::expr_t *expr) {
        while (expr != nullptr) {
            ASR::expr_t *value = ASRUtils::expr_value(expr);
            if (value != nullptr && value != expr) {
                expr = value;
                continue;
            }
            if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr)->m_arg;
                continue;
            }
            if (ASR::is_a<ASR::Cast_t>(*expr)) {
                expr = ASR::down_cast<ASR::Cast_t>(expr)->m_arg;
                continue;
            }
            if (ASR::is_a<ASR::GetPointer_t>(*expr)) {
                expr = ASR::down_cast<ASR::GetPointer_t>(expr)->m_arg;
                continue;
            }
            break;
        }
        if (expr != nullptr && ASR::is_a<ASR::ArrayConstant_t>(*expr)) {
            return ASR::down_cast<ASR::ArrayConstant_t>(expr);
        }
        return nullptr;
    }

    bool is_c_unit_step_expr(ASR::expr_t *step) {
        if (step == nullptr) {
            return true;
        }
        ASR::expr_t *value = ASRUtils::expr_value(step);
        if (value == nullptr) {
            value = step;
        }
        return ASR::is_a<ASR::IntegerConstant_t>(*value)
            && ASR::down_cast<ASR::IntegerConstant_t>(value)->m_n == 1;
    }

    bool is_c_generated_extent_upper_bound_expr(ASR::expr_t *expr) {
        while (expr != nullptr && (ASR::is_a<ASR::Cast_t>(*expr)
                    || ASR::is_a<ASR::ArrayPhysicalCast_t>(*expr))) {
            if (ASR::is_a<ASR::Cast_t>(*expr)) {
                expr = ASR::down_cast<ASR::Cast_t>(expr)->m_arg;
            } else {
                expr = ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr)->m_arg;
            }
        }
        if (expr == nullptr || !ASR::is_a<ASR::IntegerBinOp_t>(*expr)) {
            return false;
        }
        ASR::IntegerBinOp_t *sub = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
        if (sub->m_op != ASR::binopType::Sub || !is_c_unit_step_expr(sub->m_right)) {
            return false;
        }
        ASR::expr_t *left = sub->m_left;
        while (left != nullptr && (ASR::is_a<ASR::Cast_t>(*left)
                    || ASR::is_a<ASR::ArrayPhysicalCast_t>(*left))) {
            if (ASR::is_a<ASR::Cast_t>(*left)) {
                left = ASR::down_cast<ASR::Cast_t>(left)->m_arg;
            } else {
                left = ASR::down_cast<ASR::ArrayPhysicalCast_t>(left)->m_arg;
            }
        }
        if (left == nullptr || !ASR::is_a<ASR::IntegerBinOp_t>(*left)) {
            return false;
        }
        ASR::IntegerBinOp_t *add = ASR::down_cast<ASR::IntegerBinOp_t>(left);
        return add->m_op == ASR::binopType::Add
            && (is_c_unit_step_expr(add->m_left)
                || is_c_unit_step_expr(add->m_right));
    }

    ASR::expr_t *unwrap_c_array_expr(ASR::expr_t *expr) {
        while (expr != nullptr) {
            if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr)->m_arg;
                continue;
            }
            if (ASR::is_a<ASR::Cast_t>(*expr)) {
                expr = ASR::down_cast<ASR::Cast_t>(expr)->m_arg;
                continue;
            }
            break;
        }
        return expr;
    }

    bool is_c_scalarizable_element_type(ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable_pointer(type));
        return type != nullptr
            && (ASRUtils::is_integer(*type)
                || ASRUtils::is_unsigned_integer(*type)
                || ASRUtils::is_real(*type)
                || ASRUtils::is_logical(*type));
    }

    bool is_c_fixed_size_struct_member_array_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            return false;
        }
        ASR::StructInstanceMember_t *member =
            ASR::down_cast<ASR::StructInstanceMember_t>(expr);
        ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member->m_m);
        if (!ASR::is_a<ASR::Variable_t>(*member_sym)) {
            return false;
        }
        ASR::ttype_t *member_type = ASRUtils::type_get_past_allocatable_pointer(
            ASR::down_cast<ASR::Variable_t>(member_sym)->m_type);
        return member_type != nullptr
            && ASR::is_a<ASR::Array_t>(*member_type)
            && ASR::down_cast<ASR::Array_t>(member_type)->m_physical_type
                == ASR::array_physical_typeType::FixedSizeArray;
    }

    bool is_c_fixed_size_descriptor_storage_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        if (!ASR::is_a<ASR::Variable_t>(*sym)) {
            return false;
        }
        ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
        if (ASRUtils::is_arg_dummy(var->m_intent)
                || var->m_storage == ASR::storage_typeType::Parameter) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(var->m_type);
        return type != nullptr
            && ASRUtils::is_array(type)
            && ASRUtils::is_fixed_size_array(type);
    }

    bool is_c_local_fixed_size_descriptor_storage_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        if (!ASR::is_a<ASR::Variable_t>(*sym)) {
            return false;
        }
        ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
        if (var->m_storage == ASR::storage_typeType::Parameter
                || ASRUtils::is_arg_dummy(var->m_intent)
                || ASRUtils::is_allocatable(var->m_type)
                || ASRUtils::is_pointer(var->m_type)
                || !(var->m_intent == ASRUtils::intent_local
                    || var->m_intent == ASRUtils::intent_return_var)) {
            return false;
        }
        ASR::asr_t *owner = var->m_parent_symtab
            ? var->m_parent_symtab->asr_owner : nullptr;
        if (!(owner
                && (CUtils::is_symbol_owner<ASR::Program_t>(owner)
                    || CUtils::is_symbol_owner<ASR::Function_t>(owner)
                    || CUtils::is_symbol_owner<ASR::Block_t>(owner)
                    || CUtils::is_symbol_owner<ASR::AssociateBlock_t>(owner)))) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(var->m_type);
        return type != nullptr
            && ASRUtils::is_array(type)
            && ASRUtils::is_fixed_size_array(type);
    }

    bool is_c_compiler_created_array_temp_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr != nullptr && ASR::is_a<ASR::ArraySection_t>(*expr)) {
            expr = unwrap_c_array_expr(ASR::down_cast<ASR::ArraySection_t>(expr)->m_v);
        }
        if (expr == nullptr || !ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        if (!ASR::is_a<ASR::Variable_t>(*sym)) {
            return false;
        }
        return std::string(ASR::down_cast<ASR::Variable_t>(sym)->m_name)
            .rfind("__libasr_created_", 0) == 0;
    }

    bool is_c_array_section_association_temp_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        return c_array_section_association_temps.find(
            get_hash(reinterpret_cast<ASR::asr_t*>(sym))) !=
            c_array_section_association_temps.end();
    }

    bool is_c_whole_allocatable_or_pointer_array_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        return type != nullptr
            && ASRUtils::is_array(type)
            && ASRUtils::is_allocatable_or_pointer(type);
    }

    bool is_c_rank1_unit_array_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return false;
        }
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
            if (is_c_fixed_size_struct_member_array_expr(section->m_v)) {
                return false;
            }
            ASR::ttype_t *section_type = ASRUtils::expr_type(expr);
            if (!ASRUtils::is_array(section_type)
                    || ASRUtils::extract_n_dims_from_ttype(section_type) != 1) {
                return false;
            }
            size_t slice_dims = 0;
            for (size_t i = 0; i < section->n_args; i++) {
                ASR::array_index_t idx = section->m_args[i];
                bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
                if (!is_slice) {
                    continue;
                }
                slice_dims++;
                if (!is_c_unit_step_expr(idx.m_step)) {
                    return false;
                }
            }
            return slice_dims == 1;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || ASRUtils::extract_n_dims_from_ttype(type) != 1) {
            return false;
        }
        return ASR::is_a<ASR::Var_t>(*expr)
            || (ASR::is_a<ASR::StructInstanceMember_t>(*expr)
                && !is_c_fixed_size_struct_member_array_expr(expr));
    }

    bool is_c_rank1_vector_subscript_array_item_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::ArrayItem_t>(*expr)) {
            return false;
        }
        ASR::ttype_t *expr_type = ASRUtils::expr_type(expr);
        if (!ASRUtils::is_array(expr_type)
                || ASRUtils::extract_n_dims_from_ttype(expr_type) != 1) {
            return false;
        }
        ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
        if (is_c_fixed_size_struct_member_array_expr(item->m_v)
                || is_fixed_size_array_storage_expr(item->m_v)) {
            return false;
        }
        if (!is_c_scalarizable_element_type(expr_type)) {
            return false;
        }
        int vector_dims = 0;
        for (size_t i = 0; i < item->n_args; i++) {
            ASR::expr_t *idx_expr = get_array_index_expr(item->m_args[i]);
            if (idx_expr == nullptr) {
                return false;
            }
            if (is_vector_subscript_expr(idx_expr)) {
                vector_dims++;
            }
        }
        return vector_dims == 1;
    }

    bool is_c_rank1_scalarizable_array_expr(ASR::expr_t *expr) {
        return is_c_rank1_unit_array_expr(expr)
            || is_c_rank1_vector_subscript_array_item_expr(expr);
    }

    bool is_c_scalarizable_array_constructor(const ASR::ArrayConstructor_t &x) {
        if (!is_c_scalarizable_element_type(x.m_type)) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(x.m_type);
        if (type == nullptr || ASRUtils::extract_n_dims_from_ttype(type) != 1) {
            return false;
        }
        for (size_t i = 0; i < x.n_args; i++) {
            if (ASR::is_a<ASR::ImpliedDoLoop_t>(*x.m_args[i])) {
                return false;
            }
        }
        return true;
    }

    bool is_c_scalarizable_array_constant(const ASR::ArrayConstant_t &x) {
        if (!is_c_scalarizable_element_type(x.m_type)) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(x.m_type);
        return type != nullptr && ASRUtils::extract_n_dims_from_ttype(type) == 1;
    }

    bool is_c_rank2_scalarizable_reshape(const ASR::ArrayReshape_t &x) {
        if (x.m_pad != nullptr || x.m_order != nullptr) {
            return false;
        }
        ASR::ttype_t *reshape_type = ASRUtils::type_get_past_allocatable_pointer(x.m_type);
        ASR::ttype_t *source_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(x.m_array));
        return reshape_type != nullptr
            && source_type != nullptr
            && ASRUtils::extract_n_dims_from_ttype(reshape_type) == 2
            && ASRUtils::extract_n_dims_from_ttype(source_type) == 1
            && ASRUtils::is_fixed_size_array(reshape_type)
            && is_c_scalarizable_array_expr(x.m_array);
    }

    bool is_c_scalarizable_array_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        if (!ASRUtils::is_array(type)) {
            return true;
        }
        if (!is_c_scalarizable_element_type(type)) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection: {
                return is_c_rank1_unit_array_expr(expr);
            }
            case ASR::exprType::ArrayConstant: {
                return is_c_scalarizable_array_constant(
                    *ASR::down_cast<ASR::ArrayConstant_t>(expr));
            }
            case ASR::exprType::ArrayConstructor: {
                return is_c_scalarizable_array_constructor(
                    *ASR::down_cast<ASR::ArrayConstructor_t>(expr));
            }
            case ASR::exprType::ArrayItem: {
                return is_c_rank1_vector_subscript_array_item_expr(expr);
            }
            case ASR::exprType::ArrayBroadcast: {
                return is_c_scalarizable_array_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_scalarizable_array_expr(binop->m_left)
                    && is_c_scalarizable_array_expr(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_scalarizable_array_expr(binop->m_left)
                    && is_c_scalarizable_array_expr(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop = ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_scalarizable_array_expr(binop->m_left)
                    && is_c_scalarizable_array_expr(binop->m_right);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_scalarizable_array_expr(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_scalarizable_array_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::ArrayReshape: {
                return is_c_rank2_scalarizable_reshape(
                    *ASR::down_cast<ASR::ArrayReshape_t>(expr));
            }
            case ASR::exprType::RealSqrt: {
                return is_c_scalarizable_array_expr(
                    ASR::down_cast<ASR::RealSqrt_t>(expr)->m_arg);
            }
            case ASR::exprType::IntrinsicElementalFunction: {
                ASR::IntrinsicElementalFunction_t *ief =
                    ASR::down_cast<ASR::IntrinsicElementalFunction_t>(expr);
                switch (static_cast<ASRUtils::IntrinsicElementalFunctions>(
                            ief->m_intrinsic_id)) {
                    case ASRUtils::IntrinsicElementalFunctions::Abs:
                    case ASRUtils::IntrinsicElementalFunctions::Sin:
                    case ASRUtils::IntrinsicElementalFunctions::Cos:
                    case ASRUtils::IntrinsicElementalFunctions::Tan:
                    case ASRUtils::IntrinsicElementalFunctions::Exp:
                    case ASRUtils::IntrinsicElementalFunctions::Sqrt:
                    case ASRUtils::IntrinsicElementalFunctions::Max:
                    case ASRUtils::IntrinsicElementalFunctions::Min:
                    case ASRUtils::IntrinsicElementalFunctions::FMA: {
                        for (size_t i = 0; i < ief->n_args; i++) {
                            if (!is_c_scalarizable_array_expr(ief->m_args[i])) {
                                return false;
                            }
                        }
                        return true;
                    }
                    default: {
                        return false;
                    }
                }
            }
            default: {
                return false;
            }
        }
    }

    std::string get_c_array_section_bound_expr(ASR::expr_t *expr,
            const std::string &fallback) {
        if (expr == nullptr) {
            return fallback;
        }
        if (ASR::is_a<ASR::ArrayBound_t>(*expr)) {
            return fallback;
        }
        self().visit_expr(*expr);
        return src;
    }

    bool get_c_static_rank1_array_element_expr(ASR::expr_t *expr,
            const std::string &index_name, std::string &setup, std::string &out) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !is_c_rank1_unit_array_expr(expr)) {
            return false;
        }

        ASR::expr_t *base_expr = expr;
        ASR::ArraySection_t *section = nullptr;
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            section = ASR::down_cast<ASR::ArraySection_t>(expr);
            base_expr = unwrap_c_array_expr(section->m_v);
        }
        bool base_is_simd = ASRUtils::is_simd_array(base_expr);
        if (!base_is_simd && !is_c_fixed_size_descriptor_storage_expr(base_expr)) {
            return false;
        }

        ASR::ttype_t *base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(base_expr));
        ASR::dimension_t *base_dims = nullptr;
        int base_rank = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
        if (base_dims == nullptr || base_rank <= 0) {
            return false;
        }

        std::vector<int64_t> static_lbs;
        std::vector<int64_t> static_lengths;
        std::vector<int64_t> static_strides;
        int64_t stride = 1;
        static_lbs.reserve(base_rank);
        static_lengths.reserve(base_rank);
        static_strides.reserve(base_rank);
        for (int i = 0; i < base_rank; i++) {
            int64_t lb = 1, len = -1;
            if (base_dims[i].m_start != nullptr
                    && !get_c_constant_int_expr_value(base_dims[i].m_start, lb)) {
                return false;
            }
            if (base_dims[i].m_length == nullptr
                    || !get_c_constant_int_expr_value(base_dims[i].m_length, len)
                    || len <= 0) {
                return false;
            }
            static_lbs.push_back(lb);
            static_lengths.push_back(len);
            static_strides.push_back(stride);
            stride *= len;
        }

        self().visit_expr(*base_expr);
        std::string base = src;
        setup += drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(base);

        std::vector<std::string> offset_terms;
        offset_terms.push_back("0");
        int64_t slice_stride = 1;
        if (section == nullptr) {
            if (base_rank != 1) {
                return false;
            }
            slice_stride = static_strides[0];
        } else {
            if ((int)section->n_args != base_rank) {
                return false;
            }
            bool found_slice = false;
            for (int source_dim = 0; source_dim < base_rank; source_dim++) {
                ASR::array_index_t idx = section->m_args[source_dim];
                bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
                if (is_slice) {
                    bool left_is_default = idx.m_left == nullptr
                        || ASR::is_a<ASR::ArrayBound_t>(*idx.m_left);
                    bool right_is_default = idx.m_right == nullptr
                        || ASR::is_a<ASR::ArrayBound_t>(*idx.m_right);
                    if (found_slice || !left_is_default || !right_is_default
                            || !is_c_unit_step_expr(idx.m_step)) {
                        return false;
                    }
                    found_slice = true;
                    slice_stride = static_strides[source_dim];
                    continue;
                }
                ASR::expr_t *idx_expr = get_array_index_expr(idx);
                if (idx_expr == nullptr) {
                    return false;
                }
                self().visit_expr(*idx_expr);
                std::string idx_src = src;
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(idx_src);
                offset_terms.push_back(std::to_string(static_strides[source_dim])
                    + " * ((" + idx_src + ") - "
                    + std::to_string(static_lbs[source_dim]) + ")");
            }
            if (!found_slice) {
                return false;
            }
        }

        std::string offset_expr = "(";
        for (size_t i = 0; i < offset_terms.size(); i++) {
            if (i > 0) {
                offset_expr += " + ";
            }
            offset_expr += offset_terms[i];
        }
        offset_expr += ")";
        std::string base_data = base_is_simd
            ? base
            : (is_c_local_fixed_size_descriptor_storage_expr(base_expr)
                ? base + "_data" : base + "->data");
        out = base_data + "[" + offset_expr + " + " + index_name
            + " * " + std::to_string(slice_stride) + "]";
        return true;
    }

    bool get_c_direct_rank1_descriptor_element_expr(ASR::expr_t *expr,
            const std::string &index_name, std::string &setup, std::string &out) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !is_c_rank1_unit_array_expr(expr)) {
            return false;
        }

        ASR::expr_t *base_expr = expr;
        ASR::ArraySection_t *section = nullptr;
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            section = ASR::down_cast<ASR::ArraySection_t>(expr);
            base_expr = unwrap_c_array_expr(section->m_v);
        }
        if (base_expr == nullptr || is_c_fixed_size_descriptor_storage_expr(base_expr)
                || !(ASR::is_a<ASR::Var_t>(*base_expr)
                    || ASR::is_a<ASR::StructInstanceMember_t>(*base_expr))) {
            return false;
        }

        self().visit_expr(*base_expr);
        std::string base = src;
        setup += drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(base);

        ASR::ttype_t *base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(base_expr));
        ASR::dimension_t *base_dims = nullptr;
        int base_rank = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
        if (base_rank <= 0) {
            return false;
        }
        const CArrayDescriptorCache *cache =
            get_current_function_array_descriptor_cache(base, base_rank);
        std::vector<bool> has_known_base_lbs(static_cast<size_t>(base_rank), false);
        std::vector<int64_t> known_base_lbs(static_cast<size_t>(base_rank), 0);
        if (base_dims != nullptr
                && ASRUtils::extract_physical_type(base_type)
                    == ASR::array_physical_typeType::PointerArray) {
            for (int i = 0; i < base_rank; i++) {
                int64_t lb = 1;
                if (base_dims[i].m_start != nullptr
                        && get_c_constant_int_expr_value(base_dims[i].m_start, lb)) {
                    has_known_base_lbs[static_cast<size_t>(i)] = true;
                    known_base_lbs[static_cast<size_t>(i)] = lb;
                }
            }
        }

        if (section == nullptr) {
            if (base_rank != 1) {
                return false;
            }
            std::string data = cache && !cache->data.empty()
                ? cache->data : base + "->data";
            std::string offset = cache ? cache->offset : base + "->offset";
            std::string stride = cache ? cache->strides[0]
                : base + "->dims[0].stride";
            out = data + "[(" + offset + " + " + index_name
                + " * " + stride + ")]";
            return true;
        }

        if ((int)section->n_args != base_rank) {
            return false;
        }

        std::vector<std::string> offset_terms;
        offset_terms.push_back(cache ? cache->offset : base + "->offset");
        std::string slice_stride;
        bool found_slice = false;
        for (int source_dim = 0; source_dim < base_rank; source_dim++) {
            ASR::array_index_t idx = section->m_args[source_dim];
            std::string base_lb = base + "->dims[" + std::to_string(source_dim)
                + "].lower_bound";
            if (cache) {
                base_lb = cache->lower_bounds[source_dim];
            }
            if (has_known_base_lbs[static_cast<size_t>(source_dim)]) {
                base_lb = std::to_string(
                    known_base_lbs[static_cast<size_t>(source_dim)]);
            }
            std::string base_stride = base + "->dims[" + std::to_string(source_dim)
                + "].stride";
            if (cache) {
                base_stride = cache->strides[source_dim];
            }
            bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
            if (is_slice) {
                if (found_slice || !is_c_unit_step_expr(idx.m_step)) {
                    return false;
                }
                bool left_is_default = idx.m_left == nullptr
                    || ASR::is_a<ASR::ArrayBound_t>(*idx.m_left);
                if (!left_is_default) {
                    std::string left = get_c_array_section_bound_expr(idx.m_left, base_lb);
                    setup += drain_tmp_buffer();
                    setup += extract_stmt_setup_from_expr(left);
                    int64_t left_value = 0;
                    bool left_is_known_lb =
                        has_known_base_lbs[static_cast<size_t>(source_dim)]
                        && get_c_constant_int_expr_value(idx.m_left, left_value)
                        && left_value == known_base_lbs[static_cast<size_t>(source_dim)];
                    if (!left_is_known_lb) {
                        offset_terms.push_back(base_stride + " * ((" + left + ") - "
                            + base_lb + ")");
                    }
                }
                std::string step = get_c_array_section_bound_expr(idx.m_step, "1");
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(step);
                slice_stride = "(" + base_stride + " * (" + step + "))";
                found_slice = true;
                continue;
            }

            ASR::expr_t *idx_expr = get_array_index_expr(idx);
            if (idx_expr == nullptr) {
                return false;
            }
            self().visit_expr(*idx_expr);
            std::string idx_src = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(idx_src);
            offset_terms.push_back(base_stride + " * ((" + idx_src + ") - "
                + base_lb + ")");
        }
        if (!found_slice) {
            return false;
        }

        std::string offset_expr = "(";
        for (size_t i = 0; i < offset_terms.size(); i++) {
            if (i > 0) {
                offset_expr += " + ";
            }
            offset_expr += offset_terms[i];
        }
        offset_expr += ")";
        std::string data = cache && !cache->data.empty()
            ? cache->data : base + "->data";
        out = data + "[" + offset_expr + " + " + index_name
            + " * " + slice_stride + "]";
        return true;
    }

    bool get_c_rank1_array_access(ASR::expr_t *expr, const std::string &prefix,
            std::string &setup, std::string &data_name, std::string &offset_name,
            std::string &stride_name, std::string &length_name,
            bool need_length=true) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !is_c_rank1_unit_array_expr(expr)) {
            return false;
        }
        ASR::expr_t *base_expr = expr;
        ASR::ArraySection_t *section = nullptr;
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            section = ASR::down_cast<ASR::ArraySection_t>(expr);
            base_expr = section->m_v;
        }
        self().visit_expr(*base_expr);
        std::string base = src;
        setup += drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(base);

        ASR::ttype_t *base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(base_expr));
        ASR::dimension_t *base_dims = nullptr;
        int base_rank = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
        bool use_static_base_dims = is_c_fixed_size_descriptor_storage_expr(base_expr)
            && base_dims != nullptr && base_rank > 0;
        std::vector<int64_t> static_lbs;
        std::vector<int64_t> static_lengths;
        std::vector<int64_t> static_strides;
        if (use_static_base_dims) {
            int64_t stride = 1;
            static_lbs.reserve(base_rank);
            static_lengths.reserve(base_rank);
            static_strides.reserve(base_rank);
            for (int i = 0; i < base_rank; i++) {
                int64_t lb = 1, len = -1;
                if (base_dims[i].m_start != nullptr
                        && !get_c_constant_int_expr_value(base_dims[i].m_start, lb)) {
                    use_static_base_dims = false;
                    break;
                }
                if (base_dims[i].m_length == nullptr
                        || !get_c_constant_int_expr_value(base_dims[i].m_length, len)
                        || len <= 0) {
                    use_static_base_dims = false;
                    break;
                }
                static_lbs.push_back(lb);
                static_lengths.push_back(len);
                static_strides.push_back(stride);
                stride *= len;
            }
        }

        std::string base_lb = base + "->dims[0].lower_bound";
        std::string base_len = base + "->dims[0].length";
        std::string base_stride = base + "->dims[0].stride";
        if (use_static_base_dims) {
            base_lb = std::to_string(static_lbs[0]);
            base_len = std::to_string(static_lengths[0]);
            base_stride = std::to_string(static_strides[0]);
        }
        const CArrayDescriptorCache *cache =
            use_static_base_dims ? nullptr
            : get_current_function_array_descriptor_cache(base, base_rank);
        if (cache) {
            base_lb = cache->lower_bounds[0];
            base_stride = cache->strides[0];
        }
        std::string left = base_lb;
        std::string right = "(" + base_lb + " + " + base_len + " - 1)";
        std::string step = "1";
        std::vector<std::string> offset_terms;
        offset_terms.push_back(use_static_base_dims ? "0" :
            (cache ? cache->offset : base + "->offset"));
        if (section != nullptr) {
            if ((int)section->n_args != base_rank) {
                return false;
            }
            bool found_slice = false;
            for (int source_dim = 0; source_dim < base_rank; source_dim++) {
                ASR::array_index_t idx = section->m_args[source_dim];
                std::string dim_idx = std::to_string(source_dim);
                std::string dim_lb = base + "->dims[" + dim_idx + "].lower_bound";
                std::string dim_len = base + "->dims[" + dim_idx + "].length";
                std::string dim_stride = base + "->dims[" + dim_idx + "].stride";
                if (use_static_base_dims) {
                    dim_lb = std::to_string(static_lbs[source_dim]);
                    dim_len = std::to_string(static_lengths[source_dim]);
                    dim_stride = std::to_string(static_strides[source_dim]);
                } else if (cache) {
                    dim_lb = cache->lower_bounds[source_dim];
                    dim_stride = cache->strides[source_dim];
                }
                std::string dim_ub = "(" + dim_lb + " + " + dim_len + " - 1)";
                bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
                if (!is_slice) {
                    std::string item = get_c_array_section_bound_expr(idx.m_right, dim_lb);
                    setup += drain_tmp_buffer();
                    setup += extract_stmt_setup_from_expr(item);
                    offset_terms.push_back(dim_stride + " * ((" + item + ") - "
                        + dim_lb + ")");
                    continue;
                }
                if (found_slice) {
                    return false;
                }
                found_slice = true;
                base_lb = dim_lb;
                base_stride = dim_stride;
                left = get_c_array_section_bound_expr(idx.m_left, dim_lb);
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(left);
                right = get_c_array_section_bound_expr(idx.m_right, dim_ub);
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(right);
                step = get_c_array_section_bound_expr(idx.m_step, "1");
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(step);
                bool left_is_default = idx.m_left == nullptr
                    || ASR::is_a<ASR::ArrayBound_t>(*idx.m_left);
                if (!left_is_default) {
                    offset_terms.push_back(dim_stride + " * ((" + left + ") - "
                        + dim_lb + ")");
                }
            }
            if (!found_slice) {
                return false;
            }
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        data_name = get_unique_local_name(prefix + "_data");
        offset_name = get_unique_local_name(prefix + "_offset");
        stride_name = get_unique_local_name(prefix + "_stride");
        length_name.clear();
        if (need_length) {
            length_name = get_unique_local_name(prefix + "_length");
        }
        std::string elem_type = CUtils::get_c_type_from_ttype_t(
            ASRUtils::type_get_past_array(
                ASRUtils::type_get_past_allocatable_pointer(
                    ASRUtils::expr_type(base_expr))));
        setup += indent + elem_type + " *" + data_name + " = "
            + (cache && !cache->data.empty() ? cache->data : base + "->data") + ";\n";
        setup += indent + "int64_t " + stride_name + " = " + base_stride + " * (" + step + ");\n";
        std::string offset_expr = "(";
        for (size_t i = 0; i < offset_terms.size(); i++) {
            if (i > 0) {
                offset_expr += " + ";
            }
            offset_expr += offset_terms[i];
        }
        offset_expr += ")";
        if (section == nullptr) {
            offset_expr = use_static_base_dims ? "0" :
                (cache ? cache->offset : base + "->offset");
        }
        setup += indent + "int64_t " + offset_name + " = " + offset_expr + ";\n";
        if (need_length) {
            setup += indent + "int64_t " + length_name + " = ((((" + right + ") - ("
                + left + ")) / (" + step + ")) + 1);\n";
        }
        return true;
    }

    bool get_c_constant_int_expr_value(ASR::expr_t *expr, int64_t &value) {
        if (expr == nullptr) {
            return false;
        }
        ASR::expr_t *expr_value = ASRUtils::expr_value(expr);
        if (expr_value == nullptr) {
            expr_value = expr;
        }
        return ASRUtils::extract_value(expr_value, value);
    }

    bool get_c_constant_vector_subscript_values(ASR::expr_t *expr,
            std::vector<std::string> &values, int64_t max_length=8) {
        ASR::ArrayConstant_t *arr = get_c_array_constant_expr(expr);
        if (arr == nullptr) {
            return false;
        }
        ASR::ttype_t *array_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (array_type == nullptr || !ASRUtils::is_fixed_size_array(array_type)) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(array_type);
        if (element_type == nullptr || !ASRUtils::is_integer(*element_type)) {
            return false;
        }
        int64_t length = ASRUtils::get_fixed_size_of_array(array_type);
        if (length <= 0 || length > max_length) {
            return false;
        }
        values.clear();
        values.reserve(static_cast<size_t>(length));
        for (int64_t i = 0; i < length; i++) {
            values.push_back(get_c_array_constant_init_element_for_c_index(
                arr, array_type, element_type, static_cast<size_t>(i)));
        }
        return true;
    }

    std::string get_c_constant_vector_subscript_index_expr(
            const std::vector<std::string> &values,
            const std::string &index_name) {
        LCOMPILERS_ASSERT(!values.empty());
        std::string out = "(" + values.back() + ")";
        for (int64_t i = static_cast<int64_t>(values.size()) - 2; i >= 0; i--) {
            out = "((" + index_name + ") == " + std::to_string(i) + " ? ("
                + values[static_cast<size_t>(i)] + ") : " + out + ")";
        }
        return out;
    }

    bool get_c_rank1_array_constant_element_expr(ASR::ArrayConstant_t *arr,
            const std::string &index_name, std::string &out) {
        ASR::ttype_t *array_type = ASRUtils::type_get_past_allocatable_pointer(arr->m_type);
        if (array_type == nullptr || !ASRUtils::is_fixed_size_array(array_type)
                || ASRUtils::extract_n_dims_from_ttype(array_type) != 1) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(array_type);
        int64_t length = ASRUtils::get_fixed_size_of_array(array_type);
        if (element_type == nullptr || length <= 0) {
            return false;
        }
        out = "(" + get_c_array_constant_init_element_for_c_index(
            arr, array_type, element_type, static_cast<size_t>(length - 1)) + ")";
        for (int64_t i = length - 2; i >= 0; i--) {
            out = "((" + index_name + ") == " + std::to_string(i) + " ? ("
                + get_c_array_constant_init_element_for_c_index(
                    arr, array_type, element_type, static_cast<size_t>(i))
                + ") : " + out + ")";
        }
        return true;
    }

    bool get_c_rank1_array_constructor_element_expr(const ASR::ArrayConstructor_t &x,
            const std::string &index_name, std::string &setup, std::string &out) {
        if (!is_c_scalarizable_array_constructor(x) || x.n_args == 0) {
            return false;
        }
        std::vector<std::string> values;
        values.reserve(x.n_args);
        for (size_t i = 0; i < x.n_args; i++) {
            self().visit_expr(*x.m_args[i]);
            std::string value = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(value);
            values.push_back("(" + value + ")");
        }
        out = values.back();
        for (int64_t i = static_cast<int64_t>(values.size()) - 2; i >= 0; i--) {
            out = "((" + index_name + ") == " + std::to_string(i) + " ? "
                + values[static_cast<size_t>(i)] + " : " + out + ")";
        }
        return true;
    }

    bool get_c_rank1_compile_time_length(ASR::expr_t *expr, int64_t &length) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return false;
        }
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            if (!is_c_rank1_unit_array_expr(expr)) {
                return false;
            }
            ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
            for (size_t i = 0; i < section->n_args; i++) {
                ASR::array_index_t idx = section->m_args[i];
                bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
                if (!is_slice) {
                    continue;
                }
                int64_t left = 1, right = -1, step = 1;
                if (idx.m_left != nullptr
                        && !get_c_constant_int_expr_value(idx.m_left, left)) {
                    return false;
                }
                if (idx.m_right == nullptr
                        || !get_c_constant_int_expr_value(idx.m_right, right)) {
                    return false;
                }
                if (idx.m_step != nullptr
                        && !get_c_constant_int_expr_value(idx.m_step, step)) {
                    return false;
                }
                if (step <= 0 || right < left) {
                    return false;
                }
                length = ((right - left) / step) + 1;
                return length > 0;
            }
            return false;
        }
        if (ASR::is_a<ASR::ArrayItem_t>(*expr)) {
            if (!is_c_rank1_vector_subscript_array_item_expr(expr)) {
                return false;
            }
            ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
            int vector_dim = get_vector_subscript_dim(*item);
            if (vector_dim < 0) {
                return false;
            }
            ASR::expr_t *vector_expr = get_array_index_expr(item->m_args[vector_dim]);
            ASR::ttype_t *vector_type = ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(vector_expr));
            if (vector_type == nullptr || !ASRUtils::is_fixed_size_array(vector_type)) {
                return false;
            }
            length = ASRUtils::get_fixed_size_of_array(vector_type);
            return length > 0;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || !ASRUtils::is_fixed_size_array(type)) {
            return false;
        }
        length = ASRUtils::get_fixed_size_of_array(type);
        return length > 0;
    }

    bool get_c_rank1_array_element_expr(ASR::expr_t *expr, const std::string &prefix,
            const std::string &index_name, std::string &setup, std::string &out,
            std::string &length_name, bool need_length=true) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        if (is_c_rank1_unit_array_expr(expr)) {
            if (!need_length
                    && get_c_static_rank1_array_element_expr(
                        expr, index_name, setup, out)) {
                length_name.clear();
                return true;
            }
            if (!need_length
                    && get_c_direct_rank1_descriptor_element_expr(
                        expr, index_name, setup, out)) {
                length_name.clear();
                return true;
            }
            std::string data_name, offset_name, stride_name;
            if (!get_c_rank1_array_access(expr, prefix,
                    setup, data_name, offset_name, stride_name, length_name,
                    need_length)) {
                return false;
            }
            out = data_name + "[" + offset_name + " + " + index_name
                + " * " + stride_name + "]";
            return true;
        }
        if (!is_c_rank1_vector_subscript_array_item_expr(expr)) {
            return false;
        }

        ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
        int vector_dim = get_vector_subscript_dim(*item);
        if (vector_dim < 0) {
            return false;
        }

        self().visit_expr(*item->m_v);
        std::string base_array = src;
        setup += drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(base_array);

        ASR::expr_t *vector_expr = get_array_index_expr(item->m_args[vector_dim]);
        LCOMPILERS_ASSERT(vector_expr != nullptr);
        std::vector<std::string> constant_vector_values;
        bool use_constant_vector = get_c_constant_vector_subscript_values(
            vector_expr, constant_vector_values);

        std::string vector_src;
        if (!use_constant_vector) {
            self().visit_expr(*vector_expr);
            vector_src = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(vector_src);
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string vec_data, vec_offset, vec_stride;
        length_name.clear();
        if (need_length) {
            length_name = get_unique_local_name(prefix + "_length");
        }
        if (use_constant_vector) {
            if (need_length) {
                setup += indent + "int64_t " + length_name + " = "
                    + std::to_string(constant_vector_values.size()) + ";\n";
            }
        } else {
            ASR::ttype_t *index_type = ASRUtils::type_get_past_array(
                ASRUtils::type_get_past_allocatable_pointer(
                    ASRUtils::expr_type(vector_expr)));
            std::string index_c_type = CUtils::get_c_type_from_ttype_t(index_type);
            vec_data = get_unique_local_name(prefix + "_vec_data");
            vec_offset = get_unique_local_name(prefix + "_vec_offset");
            vec_stride = get_unique_local_name(prefix + "_vec_stride");
            setup += indent + index_c_type + " *" + vec_data + " = "
                + get_c_array_data_expr(vector_expr, vector_src) + ";\n";
            setup += indent + "int64_t " + vec_offset + " = "
                + get_c_array_offset_expr(vector_expr, vector_src) + ";\n";
            setup += indent + "int64_t " + vec_stride + " = "
                + get_c_array_stride_expr(vector_expr, vector_src) + ";\n";
            if (need_length) {
                setup += indent + "int64_t " + length_name + " = "
                    + get_c_array_length_expr(vector_expr, vector_src) + ";\n";
            }
        }

        std::vector<std::string> indices;
        indices.reserve(item->n_args);
        for (size_t i = 0; i < item->n_args; i++) {
            if (static_cast<int>(i) == vector_dim) {
                if (use_constant_vector) {
                    indices.push_back(get_c_constant_vector_subscript_index_expr(
                        constant_vector_values, index_name));
                } else {
                    indices.push_back(vec_data + "[" + vec_offset + " + " + index_name
                        + " * " + vec_stride + "]");
                }
                continue;
            }
            ASR::expr_t *idx_expr = get_array_index_expr(item->m_args[i]);
            LCOMPILERS_ASSERT(idx_expr != nullptr);
            self().visit_expr(*idx_expr);
            std::string idx_src = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(idx_src);
            indices.push_back(idx_src);
        }

        std::string target_index = cmo_convertor_single_element(
            base_array, indices, static_cast<int>(item->n_args), false);
        out = base_array + "->data[" + target_index + "]";
        return true;
    }

    bool try_emit_c_inline_dot_product_call(const ASR::FunctionCall_t &x,
            ASR::Function_t *fn, const std::string &fn_name, std::string &out) {
        if (!is_c || fn == nullptr || (x.n_args != 2 && x.n_args != 4)
                || x.m_args[0].m_value == nullptr
                || x.m_args[x.n_args == 4 ? 2 : 1].m_value == nullptr) {
            return false;
        }
        std::string fn_internal_name = fn->m_name;
        if (fn_internal_name.find("lcompilers_dot_product") == std::string::npos
                && fn_name.find("lcompilers_dot_product") == std::string::npos) {
            return false;
        }

        size_t rhs_arg_index = x.n_args == 4 ? 2 : 1;
        ASR::expr_t *lhs = x.m_args[0].m_value;
        ASR::expr_t *rhs = x.m_args[rhs_arg_index].m_value;
        ASR::ttype_t *lhs_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(lhs));
        ASR::ttype_t *rhs_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(rhs));
        if (lhs_type == nullptr || rhs_type == nullptr
                || !ASRUtils::is_array(lhs_type)
                || !ASRUtils::is_array(rhs_type)
                || ASRUtils::extract_n_dims_from_ttype(lhs_type) != 1
                || ASRUtils::extract_n_dims_from_ttype(rhs_type) != 1) {
            return false;
        }
        ASR::ttype_t *lhs_element_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::type_get_past_array(lhs_type));
        ASR::ttype_t *rhs_element_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::type_get_past_array(rhs_type));
        if (lhs_element_type == nullptr || rhs_element_type == nullptr
                || !ASRUtils::check_equal_type(lhs_element_type, rhs_element_type,
                    nullptr, nullptr)
                || !(ASRUtils::is_real(*x.m_type)
                    || ASRUtils::is_integer(*x.m_type))) {
            return false;
        }

        std::string setup;
        std::string index_name =
            get_unique_local_name("__libasr_created__dot_product_i");
        std::string lhs_item;
        std::string rhs_item;
        std::string lhs_length;
        std::string rhs_length;
        if (!get_c_rank1_array_element_expr(lhs, "__libasr_created__dot_product_lhs",
                index_name, setup, lhs_item, lhs_length, true)
                || lhs_length.empty()
                || !get_c_rank1_array_element_expr(rhs,
                    "__libasr_created__dot_product_rhs", index_name, setup,
                    rhs_item, rhs_length, false)) {
            return false;
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string inner_indent((indentation_level + 1) * indentation_spaces, ' ');
        std::string result_type = CUtils::get_c_type_from_ttype_t(x.m_type);
        std::string result_name =
            get_unique_local_name("__libasr_created__dot_product");
        setup += indent + result_type + " " + result_name + " = ("
            + result_type + ")0;\n";
        auto unrolled_sum_expr = [&](int64_t length) {
            std::string expr;
            for (int64_t i = 0; i < length; i++) {
                std::string index_literal = std::to_string(i);
                std::string term = replace_all_substrings(lhs_item, index_name,
                    index_literal) + " * " + replace_all_substrings(rhs_item,
                    index_name, index_literal);
                expr += i == 0 ? term : " + " + term;
            }
            return expr;
        };
        setup += indent + "if (" + lhs_length + " == 3) {\n";
        setup += inner_indent + result_name + " = " + unrolled_sum_expr(3) + ";\n";
        setup += indent + "} else if (" + lhs_length + " == 6) {\n";
        setup += inner_indent + result_name + " = " + unrolled_sum_expr(6) + ";\n";
        setup += indent + "} else {\n";
        setup += inner_indent + "for (int64_t " + index_name + " = 0; "
            + index_name + " < " + lhs_length + "; " + index_name + "++) {\n";
        setup += std::string((indentation_level + 2) * indentation_spaces, ' ')
            + result_name + " += " + lhs_item + " * "
            + rhs_item + ";\n";
        setup += inner_indent + "}\n";
        setup += indent + "}\n";
        tmp_buffer_src.push_back(setup);
        out = result_name;
        return true;
    }

    std::string c_binop_to_str(ASR::binopType op) {
        switch (op) {
            case ASR::binopType::Add: return " + ";
            case ASR::binopType::Sub: return " - ";
            case ASR::binopType::Mul: return " * ";
            case ASR::binopType::Div: return " / ";
            case ASR::binopType::BitAnd: return " & ";
            case ASR::binopType::BitOr: return " | ";
            case ASR::binopType::BitXor: return " ^ ";
            case ASR::binopType::BitLShift: return " << ";
            case ASR::binopType::BitRShift: return " >> ";
            case ASR::binopType::LBitRShift: return " >> ";
            default: throw CodeGenError("C scalarized array binop not implemented");
        }
    }

    template <typename T>
    bool get_c_scalarized_binop_expr(const T &x, const std::string &index_name,
            std::string &setup, std::string &out, bool need_length=true) {
        std::string left, right;
        if (!get_c_scalarized_array_expr(x.m_left, index_name, setup, left,
                    need_length)
                || !get_c_scalarized_array_expr(x.m_right, index_name, setup, right,
                    need_length)) {
            return false;
        }
        out = "(" + left + c_binop_to_str(x.m_op) + right + ")";
        return true;
    }

    bool get_c_scalarized_intrinsic_expr(const ASR::IntrinsicElementalFunction_t &x,
            const std::string &index_name, std::string &setup, std::string &out,
            bool need_length=true) {
        using IEF = ASRUtils::IntrinsicElementalFunctions;
        std::vector<std::string> args;
        for (size_t i = 0; i < x.n_args; i++) {
            std::string arg;
            if (!get_c_scalarized_array_expr(x.m_args[i], index_name, setup, arg,
                    need_length)) {
                return false;
            }
            args.push_back(arg);
        }
        switch (static_cast<IEF>(x.m_intrinsic_id)) {
            case IEF::Abs: {
                headers.insert("math.h");
                ASR::ttype_t *t = ASRUtils::expr_type(x.m_args[0]);
                out = ASRUtils::is_real(*ASRUtils::type_get_past_array(t))
                    ? "fabs(" + args[0] + ")" : "abs(" + args[0] + ")";
                return true;
            }
            case IEF::Sin: headers.insert("math.h"); out = "sin(" + args[0] + ")"; return true;
            case IEF::Cos: headers.insert("math.h"); out = "cos(" + args[0] + ")"; return true;
            case IEF::Tan: headers.insert("math.h"); out = "tan(" + args[0] + ")"; return true;
            case IEF::Exp: headers.insert("math.h"); out = "exp(" + args[0] + ")"; return true;
            case IEF::Sqrt: headers.insert("math.h"); out = "sqrt(" + args[0] + ")"; return true;
            case IEF::Max: {
                if (args.empty()) return false;
                out = args[0];
                ASR::ttype_t *t = ASRUtils::expr_type(x.m_args[0]);
                for (size_t i = 1; i < args.size(); i++) {
                    if (ASRUtils::is_real(*ASRUtils::type_get_past_array(t))) {
                        headers.insert("math.h");
                        out = "fmax(" + out + ", " + args[i] + ")";
                    } else {
                        out = "((" + out + ") > (" + args[i] + ") ? (" + out + ") : (" + args[i] + "))";
                    }
                }
                return true;
            }
            case IEF::Min: {
                if (args.empty()) return false;
                out = args[0];
                ASR::ttype_t *t = ASRUtils::expr_type(x.m_args[0]);
                for (size_t i = 1; i < args.size(); i++) {
                    if (ASRUtils::is_real(*ASRUtils::type_get_past_array(t))) {
                        headers.insert("math.h");
                        out = "fmin(" + out + ", " + args[i] + ")";
                    } else {
                        out = "((" + out + ") < (" + args[i] + ") ? (" + out + ") : (" + args[i] + "))";
                    }
                }
                return true;
            }
            case IEF::FMA: {
                if (args.size() != 3) return false;
                out = "(" + args[0] + " + " + args[1] + " * " + args[2] + ")";
                return true;
            }
            default: return false;
        }
    }

    bool get_c_scalarized_array_expr(ASR::expr_t *expr,
            const std::string &index_name, std::string &setup, std::string &out,
            bool need_length=true) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        if (ASR::is_a<ASR::IntrinsicElementalFunction_t>(*expr)) {
            ASR::IntrinsicElementalFunction_t *ief =
                ASR::down_cast<ASR::IntrinsicElementalFunction_t>(expr);
            bool has_array_arg = false;
            for (size_t i = 0; i < ief->n_args; i++) {
                if (ASRUtils::is_array(ASRUtils::expr_type(ief->m_args[i]))) {
                    has_array_arg = true;
                    break;
                }
            }
            if (has_array_arg) {
                return get_c_scalarized_intrinsic_expr(*ief, index_name, setup, out,
                    need_length);
            }
        }
        if (ASR::is_a<ASR::RealSqrt_t>(*expr)) {
            std::string arg;
            if (!get_c_scalarized_array_expr(
                    ASR::down_cast<ASR::RealSqrt_t>(expr)->m_arg,
                    index_name, setup, arg, need_length)) {
                return false;
            }
            headers.insert("math.h");
            out = "sqrt(" + arg + ")";
            return true;
        }
        if (!ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            self().visit_expr(*expr);
            out = "(" + src + ")";
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(out);
            return true;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection:
            case ASR::exprType::ArrayItem: {
                std::string length_name;
                if (!get_c_rank1_array_element_expr(expr, "__lfortran_rhs",
                        index_name, setup, out, length_name, need_length)) {
                    return false;
                }
                return true;
            }
            case ASR::exprType::ArrayConstant: {
                return get_c_rank1_array_constant_element_expr(
                    ASR::down_cast<ASR::ArrayConstant_t>(expr), index_name, out);
            }
            case ASR::exprType::ArrayConstructor: {
                return get_c_rank1_array_constructor_element_expr(
                    *ASR::down_cast<ASR::ArrayConstructor_t>(expr),
                    index_name, setup, out);
            }
            case ASR::exprType::ArrayBroadcast: {
                return get_c_scalarized_array_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array,
                    index_name, setup, out, need_length);
            }
            case ASR::exprType::RealBinOp:
                return get_c_scalarized_binop_expr(
                    *ASR::down_cast<ASR::RealBinOp_t>(expr), index_name, setup, out,
                    need_length);
            case ASR::exprType::IntegerBinOp:
                return get_c_scalarized_binop_expr(
                    *ASR::down_cast<ASR::IntegerBinOp_t>(expr), index_name, setup, out,
                    need_length);
            case ASR::exprType::UnsignedIntegerBinOp:
                return get_c_scalarized_binop_expr(
                    *ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr), index_name, setup, out,
                    need_length);
            case ASR::exprType::RealUnaryMinus: {
                std::string arg;
                if (!get_c_scalarized_array_expr(
                        ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg,
                        index_name, setup, arg, need_length)) return false;
                out = "-(" + arg + ")";
                return true;
            }
            case ASR::exprType::IntegerUnaryMinus: {
                std::string arg;
                if (!get_c_scalarized_array_expr(
                        ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg,
                        index_name, setup, arg, need_length)) return false;
                out = "-(" + arg + ")";
                return true;
            }
            case ASR::exprType::RealSqrt: {
                std::string arg;
                if (!get_c_scalarized_array_expr(
                        ASR::down_cast<ASR::RealSqrt_t>(expr)->m_arg,
                        index_name, setup, arg, need_length)) return false;
                headers.insert("math.h");
                out = "sqrt(" + arg + ")";
                return true;
            }
            case ASR::exprType::IntrinsicElementalFunction:
                return get_c_scalarized_intrinsic_expr(
                    *ASR::down_cast<ASR::IntrinsicElementalFunction_t>(expr),
                    index_name, setup, out, need_length);
            default:
                return false;
        }
    }

    ASR::symbol_t *get_c_array_assignment_root_symbol(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        while (expr != nullptr) {
            if (ASR::is_a<ASR::ArrayItem_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArrayItem_t>(expr)->m_v;
                expr = unwrap_c_lvalue_expr(expr);
                continue;
            }
            if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArraySection_t>(expr)->m_v;
                expr = unwrap_c_lvalue_expr(expr);
                continue;
            }
            break;
        }
        if (expr != nullptr && ASR::is_a<ASR::Var_t>(*expr)) {
            return ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Var_t>(expr)->m_v);
        }
        return nullptr;
    }

    bool c_expr_references_root_symbol(ASR::expr_t *expr, ASR::symbol_t *root_sym) {
        if (expr == nullptr || root_sym == nullptr) {
            return false;
        }
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var: {
                return ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(expr)->m_v) == root_sym;
            }
            case ASR::exprType::ArrayItem: {
                ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
                if (c_expr_references_root_symbol(item->m_v, root_sym)) {
                    return true;
                }
                for (size_t i = 0; i < item->n_args; i++) {
                    if (c_expr_references_root_symbol(get_array_index_expr(item->m_args[i]), root_sym)) {
                        return true;
                    }
                }
                return false;
            }
            case ASR::exprType::ArraySection: {
                ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
                if (c_expr_references_root_symbol(section->m_v, root_sym)) {
                    return true;
                }
                for (size_t i = 0; i < section->n_args; i++) {
                    ASR::array_index_t idx = section->m_args[i];
                    if (c_expr_references_root_symbol(idx.m_left, root_sym)
                            || c_expr_references_root_symbol(idx.m_right, root_sym)
                            || c_expr_references_root_symbol(idx.m_step, root_sym)) {
                        return true;
                    }
                }
                return false;
            }
            case ASR::exprType::StructInstanceMember: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::StructInstanceMember_t>(expr)->m_v, root_sym);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return c_expr_references_root_symbol(binop->m_left, root_sym)
                    || c_expr_references_root_symbol(binop->m_right, root_sym);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return c_expr_references_root_symbol(binop->m_left, root_sym)
                    || c_expr_references_root_symbol(binop->m_right, root_sym);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return c_expr_references_root_symbol(binop->m_left, root_sym)
                    || c_expr_references_root_symbol(binop->m_right, root_sym);
            }
            case ASR::exprType::RealUnaryMinus: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg, root_sym);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg, root_sym);
            }
            case ASR::exprType::RealSqrt: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::RealSqrt_t>(expr)->m_arg, root_sym);
            }
            case ASR::exprType::IntrinsicElementalFunction: {
                ASR::IntrinsicElementalFunction_t *ief =
                    ASR::down_cast<ASR::IntrinsicElementalFunction_t>(expr);
                for (size_t i = 0; i < ief->n_args; i++) {
                    if (c_expr_references_root_symbol(ief->m_args[i], root_sym)) {
                        return true;
                    }
                }
                return false;
            }
            case ASR::exprType::ArrayBroadcast: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array, root_sym);
            }
            case ASR::exprType::ArrayReshape: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::ArrayReshape_t>(expr)->m_array, root_sym);
            }
            case ASR::exprType::ArrayConstructor: {
                ASR::ArrayConstructor_t *arr =
                    ASR::down_cast<ASR::ArrayConstructor_t>(expr);
                for (size_t i = 0; i < arr->n_args; i++) {
                    if (c_expr_references_root_symbol(arr->m_args[i], root_sym)) {
                        return true;
                    }
                }
                return false;
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_default_unit_array_slice(const ASR::array_index_t &idx) {
        bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
        if (!is_slice || !is_c_unit_step_expr(idx.m_step)) {
            return false;
        }
        bool left_is_default = idx.m_left == nullptr
            || ASR::is_a<ASR::ArrayBound_t>(*idx.m_left)
            || is_c_unit_step_expr(idx.m_left);
        bool right_is_default = idx.m_right == nullptr
            || ASR::is_a<ASR::ArrayBound_t>(*idx.m_right)
            || is_c_generated_extent_upper_bound_expr(idx.m_right);
        return left_is_default && right_is_default;
    }

    bool is_c_rank2_full_array_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || (!ASRUtils::is_array(ASRUtils::expr_type(expr))
                && !ASRUtils::is_array_t(expr))) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr
                || ASRUtils::extract_n_dims_from_ttype(
                    ASRUtils::expr_type(expr)) != 2) {
            return false;
        }
        if (ASR::is_a<ASR::Var_t>(*expr)
                || ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            return true;
        }
        if (!ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return false;
        }
        ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
        for (size_t i = 0; i < section->n_args; i++) {
            if (!is_c_default_unit_array_slice(section->m_args[i])) {
                return false;
            }
        }
        return true;
    }

    bool c_rank2_full_ref_same_target(ASR::expr_t *target, ASR::expr_t *expr) {
        target = unwrap_c_array_expr(target);
        expr = unwrap_c_array_expr(expr);
        if (target == nullptr || expr == nullptr
                || !is_c_rank2_full_array_expr(target)
                || !is_c_rank2_full_array_expr(expr)) {
            return false;
        }
        ASR::symbol_t *target_root = get_c_array_assignment_root_symbol(target);
        ASR::symbol_t *expr_root = get_c_array_assignment_root_symbol(expr);
        return target_root != nullptr && target_root == expr_root;
    }

    bool is_c_rank2_scalarizable_array_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        if (!ASRUtils::is_array(type) && !ASRUtils::is_array_t(type)) {
            return true;
        }
        if (!is_c_scalarizable_element_type(type)) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection: {
                return is_c_rank2_full_array_expr(expr);
            }
            case ASR::exprType::ArrayBroadcast: {
                return is_c_rank2_scalarizable_array_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_scalarizable_array_expr(binop->m_left)
                    && is_c_rank2_scalarizable_array_expr(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_scalarizable_array_expr(binop->m_left)
                    && is_c_rank2_scalarizable_array_expr(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_scalarizable_array_expr(binop->m_left)
                    && is_c_rank2_scalarizable_array_expr(binop->m_right);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_rank2_scalarizable_array_expr(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_rank2_scalarizable_array_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::ArrayReshape: {
                return is_c_rank2_scalarizable_reshape(
                    *ASR::down_cast<ASR::ArrayReshape_t>(expr));
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_rank2_nonself_update_expr(ASR::expr_t *expr) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        if (!ASRUtils::is_array(type) && !ASRUtils::is_array_t(type)) {
            return true;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember: {
                return true;
            }
            case ASR::exprType::ArrayBroadcast: {
                return is_c_rank2_nonself_update_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_nonself_update_expr(binop->m_left)
                    && is_c_rank2_nonself_update_expr(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_nonself_update_expr(binop->m_left)
                    && is_c_rank2_nonself_update_expr(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_nonself_update_expr(binop->m_left)
                    && is_c_rank2_nonself_update_expr(binop->m_right);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_rank2_nonself_update_expr(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_rank2_nonself_update_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    template <typename BinOp>
    bool c_rank2_binop_has_one_full_self_ref(const BinOp *binop,
            ASR::expr_t *target_expr, ASR::symbol_t *target_root) {
        bool left_is_self = c_rank2_full_ref_same_target(target_expr, binop->m_left);
        bool right_is_self = c_rank2_full_ref_same_target(target_expr, binop->m_right);
        if (left_is_self == right_is_self) {
            return false;
        }
        ASR::expr_t *other = left_is_self ? binop->m_right : binop->m_left;
        return is_c_rank2_nonself_update_expr(other)
            && !c_expr_references_root_symbol(other, target_root);
    }

    bool is_c_rank2_full_self_update_expr(ASR::expr_t *target_expr,
            ASR::expr_t *value_expr) {
        target_expr = unwrap_c_array_expr(target_expr);
        value_expr = unwrap_c_array_expr(value_expr);
        if (target_expr == nullptr || value_expr == nullptr
                || !is_c_rank2_full_array_expr(target_expr)
                || !is_c_rank2_scalarizable_array_expr(value_expr)) {
            return false;
        }
        ASR::symbol_t *target_root = get_c_array_assignment_root_symbol(target_expr);
        if (target_root == nullptr) {
            return false;
        }
        switch (value_expr->type) {
            case ASR::exprType::RealBinOp: {
                return c_rank2_binop_has_one_full_self_ref(
                    ASR::down_cast<ASR::RealBinOp_t>(value_expr),
                    target_expr, target_root);
            }
            case ASR::exprType::IntegerBinOp: {
                return c_rank2_binop_has_one_full_self_ref(
                    ASR::down_cast<ASR::IntegerBinOp_t>(value_expr),
                    target_expr, target_root);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                return c_rank2_binop_has_one_full_self_ref(
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(value_expr),
                    target_expr, target_root);
            }
            default: {
                return false;
            }
        }
    }

    bool get_c_rank2_full_array_access(ASR::expr_t *expr, const std::string &prefix,
            std::string &setup, std::string &data_name, std::string &offset_name,
            std::string &stride1_name, std::string &stride2_name,
            std::string &length1_name, std::string &length2_name) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr || !is_c_rank2_full_array_expr(expr)) {
            return false;
        }
        ASR::expr_t *base_expr = expr;
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            base_expr = ASR::down_cast<ASR::ArraySection_t>(expr)->m_v;
            base_expr = unwrap_c_array_expr(base_expr);
        }
        if (base_expr == nullptr) {
            return false;
        }
        self().visit_expr(*base_expr);
        std::string base = src;
        setup += drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(base);

        ASR::ttype_t *base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(base_expr));
        ASR::dimension_t *base_dims = nullptr;
        int base_rank = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
        if (base_rank != 2) {
            return false;
        }
        bool use_static_base_dims = is_c_fixed_size_descriptor_storage_expr(base_expr)
            && base_dims != nullptr;
        std::vector<int64_t> static_lengths;
        std::vector<int64_t> static_strides;
        if (use_static_base_dims) {
            int64_t stride = 1;
            static_lengths.reserve(base_rank);
            static_strides.reserve(base_rank);
            for (int i = 0; i < base_rank; i++) {
                int64_t len = -1;
                if (base_dims[i].m_length == nullptr
                        || !get_c_constant_int_expr_value(base_dims[i].m_length, len)
                        || len <= 0) {
                    use_static_base_dims = false;
                    break;
                }
                static_lengths.push_back(len);
                static_strides.push_back(stride);
                stride *= len;
            }
        }
        const CArrayDescriptorCache *cache = use_static_base_dims ? nullptr :
            get_current_function_array_descriptor_cache(base, base_rank);
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string elem_type = CUtils::get_c_type_from_ttype_t(
            ASRUtils::type_get_past_array(base_type));
        data_name = get_unique_local_name(prefix + "_data");
        offset_name = get_unique_local_name(prefix + "_offset");
        stride1_name = get_unique_local_name(prefix + "_stride1");
        stride2_name = get_unique_local_name(prefix + "_stride2");
        length1_name = get_unique_local_name(prefix + "_length1");
        length2_name = get_unique_local_name(prefix + "_length2");
        std::string base_data = use_static_base_dims ? base + "_data" :
            (cache && !cache->data.empty() ? cache->data : base + "->data");
        setup += indent + elem_type + " *" + data_name + " = "
            + base_data + ";\n";
        setup += indent + "int64_t " + offset_name + " = "
            + (use_static_base_dims ? "0" : (cache ? cache->offset : base + "->offset")) + ";\n";
        setup += indent + "int64_t " + stride1_name + " = "
            + (use_static_base_dims ? std::to_string(static_strides[0])
                : (cache ? cache->strides[0] : base + "->dims[0].stride")) + ";\n";
        setup += indent + "int64_t " + stride2_name + " = "
            + (use_static_base_dims ? std::to_string(static_strides[1])
                : (cache ? cache->strides[1] : base + "->dims[1].stride")) + ";\n";
        setup += indent + "int64_t " + length1_name + " = "
            + (use_static_base_dims ? std::to_string(static_lengths[0])
                : base + "->dims[0].length") + ";\n";
        setup += indent + "int64_t " + length2_name + " = "
            + (use_static_base_dims ? std::to_string(static_lengths[1])
                : base + "->dims[1].length") + ";\n";
        return true;
    }

    bool get_c_rank2_scalarized_expr(ASR::expr_t *expr,
            const std::string &index1_name, const std::string &index2_name,
            std::string &setup, std::string &out) {
        expr = unwrap_c_array_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        if (!ASRUtils::is_array(ASRUtils::expr_type(expr))
                && !ASRUtils::is_array_t(expr)) {
            self().visit_expr(*expr);
            out = "(" + src + ")";
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(out);
            return true;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection: {
                std::string data, offset, stride1, stride2, length1, length2;
                if (!get_c_rank2_full_array_access(expr, "__lfortran_rhs2",
                        setup, data, offset, stride1, stride2, length1, length2)) {
                    return false;
                }
                out = data + "[" + offset + " + " + index1_name + " * "
                    + stride1 + " + " + index2_name + " * " + stride2 + "]";
                return true;
            }
            case ASR::exprType::ArrayBroadcast: {
                return get_c_rank2_scalarized_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array,
                    index1_name, index2_name, setup, out);
            }
            case ASR::exprType::ArrayReshape: {
                ASR::ArrayReshape_t *reshape =
                    ASR::down_cast<ASR::ArrayReshape_t>(expr);
                if (!is_c_rank2_scalarizable_reshape(*reshape)) {
                    return false;
                }
                ASR::dimension_t *reshape_dims = nullptr;
                int n_dims = ASRUtils::extract_dimensions_from_ttype(
                    reshape->m_type, reshape_dims);
                int64_t dim1_length = -1;
                if (n_dims != 2 || reshape_dims == nullptr
                        || reshape_dims[0].m_length == nullptr
                        || !get_c_constant_int_expr_value(
                            reshape_dims[0].m_length, dim1_length)
                        || dim1_length <= 0) {
                    return false;
                }
                std::string flat_index = "(" + index1_name + " + "
                    + index2_name + " * " + std::to_string(dim1_length) + ")";
                return get_c_scalarized_array_expr(
                    reshape->m_array, flat_index, setup, out, false);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                if (binop->m_op == ASR::binopType::Pow) {
                    return false;
                }
                std::string left, right;
                if (!get_c_rank2_scalarized_expr(binop->m_left,
                        index1_name, index2_name, setup, left)
                        || !get_c_rank2_scalarized_expr(binop->m_right,
                            index1_name, index2_name, setup, right)) {
                    return false;
                }
                out = "(" + left + c_binop_to_str(binop->m_op) + right + ")";
                return true;
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                if (binop->m_op == ASR::binopType::Pow) {
                    return false;
                }
                std::string left, right;
                if (!get_c_rank2_scalarized_expr(binop->m_left,
                        index1_name, index2_name, setup, left)
                        || !get_c_rank2_scalarized_expr(binop->m_right,
                            index1_name, index2_name, setup, right)) {
                    return false;
                }
                out = "(" + left + c_binop_to_str(binop->m_op) + right + ")";
                return true;
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                if (binop->m_op == ASR::binopType::Pow) {
                    return false;
                }
                std::string left, right;
                if (!get_c_rank2_scalarized_expr(binop->m_left,
                        index1_name, index2_name, setup, left)
                        || !get_c_rank2_scalarized_expr(binop->m_right,
                            index1_name, index2_name, setup, right)) {
                    return false;
                }
                out = "(" + left + c_binop_to_str(binop->m_op) + right + ")";
                return true;
            }
            case ASR::exprType::RealUnaryMinus: {
                std::string arg;
                if (!get_c_rank2_scalarized_expr(
                        ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg,
                        index1_name, index2_name, setup, arg)) {
                    return false;
                }
                out = "-(" + arg + ")";
                return true;
            }
            case ASR::exprType::IntegerUnaryMinus: {
                std::string arg;
                if (!get_c_rank2_scalarized_expr(
                        ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg,
                        index1_name, index2_name, setup, arg)) {
                    return false;
                }
                out = "-(" + arg + ")";
                return true;
            }
            default: {
                return false;
            }
        }
    }

    CArrayExprLoweringPlan plan_c_array_expr_assignment(
            const ASR::Assignment_t &x, ASR::expr_t *unwrapped_target_expr) {
        CArrayExprLoweringPlan plan;
        if (!is_c || unwrapped_target_expr == nullptr) {
            return plan;
        }
        unwrapped_target_expr = unwrap_c_lvalue_expr(unwrapped_target_expr);
        if (unwrapped_target_expr == nullptr) {
            return plan;
        }
        plan.target_expr = unwrapped_target_expr;
        if ((ASRUtils::is_array(ASRUtils::expr_type(x.m_value))
                    || ASRUtils::is_array_t(x.m_value))
                && is_c_rank2_full_self_update_expr(
                    unwrapped_target_expr, x.m_value)) {
            plan.kind = CArrayExprLoweringKind::Rank2FullSelfUpdateLoop;
            plan.value_expr = x.m_value;
            return plan;
        }
        if ((ASRUtils::is_array(ASRUtils::expr_type(x.m_value))
                    || ASRUtils::is_array_t(x.m_value))
                && !is_c_whole_allocatable_or_pointer_array_expr(unwrapped_target_expr)
                && is_c_rank2_full_array_expr(unwrapped_target_expr)
                && !c_expr_references_root_symbol(x.m_value,
                    get_c_array_assignment_root_symbol(unwrapped_target_expr))) {
            plan.kind = CArrayExprLoweringKind::Rank2ScalarizedLoop;
            plan.value_expr = x.m_value;
            return plan;
        }
        ASR::expr_t *base_expr = unwrapped_target_expr;
        if (ASR::is_a<ASR::ArraySection_t>(*unwrapped_target_expr)) {
            plan.target_section = ASR::down_cast<ASR::ArraySection_t>(unwrapped_target_expr);
            if (!is_c_rank1_unit_array_expr(unwrapped_target_expr)) {
                return plan;
            }
            base_expr = plan.target_section->m_v;
        }
        plan.target_base_expr = base_expr;
        if (is_c_scalarizable_array_expr(x.m_value)
                && !is_c_compiler_created_array_temp_expr(unwrapped_target_expr)
                && !is_c_whole_allocatable_or_pointer_array_expr(unwrapped_target_expr)
                && is_c_rank1_scalarizable_array_expr(unwrapped_target_expr)
                && !c_expr_references_root_symbol(x.m_value,
                    get_c_array_assignment_root_symbol(unwrapped_target_expr))) {
            plan.kind = CArrayExprLoweringKind::ScalarizedLoop;
            plan.value_expr = x.m_value;
            return plan;
        }
        ASR::ttype_t *target_rank_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(unwrapped_target_expr));
        if (target_rank_type == nullptr || !ASRUtils::is_array(target_rank_type)) {
            return plan;
        }
        int target_rank = ASRUtils::extract_n_dims_from_ttype(target_rank_type);
        bool target_is_rank1_section = ASR::is_a<ASR::ArraySection_t>(*unwrapped_target_expr)
            && is_c_rank1_unit_array_expr(unwrapped_target_expr);
        if (target_rank != 1 && !target_is_rank1_section) {
            return plan;
        }
        ASR::ArrayConstant_t *arr = get_c_array_constant_expr(x.m_value);
        if (arr == nullptr) {
            if (ASRUtils::is_array(ASRUtils::expr_type(x.m_value))) {
                plan.kind = CArrayExprLoweringKind::MaterializedTemporary;
            }
            return plan;
        }
        ASR::ttype_t *value_type = ASRUtils::type_get_past_allocatable_pointer(arr->m_type);
        if (value_type == nullptr || !ASR::is_a<ASR::Array_t>(*value_type)) {
            return plan;
        }
        ASR::Array_t *value_array_type = ASR::down_cast<ASR::Array_t>(value_type);
        ASR::ttype_t *element_type = value_array_type->m_type;
        if (ASRUtils::is_character(*element_type)) {
            return plan;
        }
        size_t n = ASRUtils::get_fixed_size_of_array(value_type);

        ASR::ttype_t *base_type = ASRUtils::expr_type(base_expr);
        ASR::ttype_t *base_type_unwrapped =
            ASRUtils::type_get_past_allocatable_pointer(base_type);
        if (base_type_unwrapped == nullptr || !ASR::is_a<ASR::Array_t>(*base_type_unwrapped)) {
            return plan;
        }
        ASR::Array_t *base_array_type = ASR::down_cast<ASR::Array_t>(base_type_unwrapped);
        ASR::array_physical_typeType base_phys = base_array_type->m_physical_type;
        bool descriptor_backed =
            base_phys == ASR::array_physical_typeType::DescriptorArray
            || base_phys == ASR::array_physical_typeType::PointerArray
            || base_phys == ASR::array_physical_typeType::UnboundedPointerArray
            || base_phys == ASR::array_physical_typeType::ISODescriptorArray
            || base_phys == ASR::array_physical_typeType::NumPyArray;
        if (!descriptor_backed) {
            return plan;
        }

        plan.kind = CArrayExprLoweringKind::CompactConstantCopy;
        plan.constant_value = arr;
        plan.constant_array_type = value_type;
        plan.constant_element_type = element_type;
        plan.constant_size = n;
        return plan;
    }

    bool try_emit_c_scalarized_array_expr_assignment(
            const CArrayExprLoweringPlan &plan) {
        if (plan.kind != CArrayExprLoweringKind::ScalarizedLoop
                || plan.target_expr == nullptr
                || plan.value_expr == nullptr) {
            return false;
        }
        std::string setup;
        std::string target_expr, target_length;
        std::string index_name = get_unique_local_name("__lfortran_i");
        int64_t compile_time_length = -1;
        bool use_unrolled = get_c_rank1_compile_time_length(
            plan.target_expr, compile_time_length)
            && compile_time_length > 0 && compile_time_length <= 8;
        bool need_length = !use_unrolled;
        if (!get_c_rank1_array_element_expr(plan.target_expr, "__lfortran_lhs",
                index_name, setup, target_expr, target_length, need_length)) {
            return false;
        }
        std::string value_expr;
        if (!get_c_scalarized_array_expr(
                plan.value_expr, index_name, setup, value_expr, need_length)) {
            return false;
        }
        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer();
        src += indent + "{\n";
        indentation_level++;
        std::string inner_indent(indentation_level * indentation_spaces, ' ');
        src += setup;
        if (use_unrolled) {
            if (setup.find(index_name) == std::string::npos) {
                for (int64_t i = 0; i < compile_time_length; i++) {
                    std::string index_literal = std::to_string(i);
                    src += inner_indent
                        + replace_all_substrings(target_expr, index_name, index_literal)
                        + " = "
                        + replace_all_substrings(value_expr, index_name, index_literal)
                        + ";\n";
                }
            } else {
                src += inner_indent + "int64_t " + index_name + ";\n";
                for (int64_t i = 0; i < compile_time_length; i++) {
                    src += inner_indent + index_name + " = "
                        + std::to_string(i) + ";\n";
                    src += inner_indent + target_expr + " = " + value_expr + ";\n";
                }
            }
            indentation_level--;
            src += indent + "}\n";
            return true;
        }
        src += inner_indent + "for (int64_t " + index_name + " = 0; "
            + index_name + " < " + target_length + "; " + index_name + "++) {\n";
        indentation_level++;
        std::string loop_indent(indentation_level * indentation_spaces, ' ');
        src += loop_indent + target_expr + " = " + value_expr + ";\n";
        indentation_level--;
        src += inner_indent + "}\n";
        indentation_level--;
        src += indent + "}\n";
        return true;
    }

    bool try_emit_c_rank2_full_self_update_assignment(
            const CArrayExprLoweringPlan &plan) {
        if ((plan.kind != CArrayExprLoweringKind::Rank2FullSelfUpdateLoop
                    && plan.kind != CArrayExprLoweringKind::Rank2ScalarizedLoop)
                || plan.target_expr == nullptr
                || plan.value_expr == nullptr) {
            return false;
        }
        std::string setup;
        std::string target_data, target_offset, target_stride1, target_stride2;
        std::string target_length1, target_length2;
        if (!get_c_rank2_full_array_access(plan.target_expr, "__lfortran_lhs2",
                setup, target_data, target_offset, target_stride1, target_stride2,
                target_length1, target_length2)) {
            return false;
        }
        std::string index1_name = get_unique_local_name("__lfortran_i");
        std::string index2_name = get_unique_local_name("__lfortran_j");
        std::string target_element = target_data + "[" + target_offset
            + " + " + index1_name + " * " + target_stride1
            + " + " + index2_name + " * " + target_stride2 + "]";
        std::string value_expr;
        if (!get_c_rank2_scalarized_expr(plan.value_expr, index1_name,
                index2_name, setup, value_expr)) {
            return false;
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer();
        src += indent + "{\n";
        indentation_level++;
        std::string inner_indent(indentation_level * indentation_spaces, ' ');
        src += setup;
        src += inner_indent + "for (int64_t " + index2_name + " = 0; "
            + index2_name + " < " + target_length2 + "; "
            + index2_name + "++) {\n";
        indentation_level++;
        std::string loop1_indent(indentation_level * indentation_spaces, ' ');
        src += loop1_indent + "for (int64_t " + index1_name + " = 0; "
            + index1_name + " < " + target_length1 + "; "
            + index1_name + "++) {\n";
        indentation_level++;
        std::string loop2_indent(indentation_level * indentation_spaces, ' ');
        src += loop2_indent + target_element + " = " + value_expr + ";\n";
        indentation_level--;
        src += loop1_indent + "}\n";
        indentation_level--;
        src += inner_indent + "}\n";
        indentation_level--;
        src += indent + "}\n";
        return true;
    }

    bool try_emit_c_char_array_bitcast_assignment(const ASR::Assignment_t &x,
            ASR::expr_t *target_expr) {
        if (!is_c || target_expr == nullptr ||
                !ASR::is_a<ASR::BitCast_t>(*x.m_value) ||
                !is_len_one_character_array_type(ASRUtils::expr_type(target_expr))) {
            return false;
        }
        ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(x.m_value);
        ASR::expr_t *source = bitcast->m_value ? bitcast->m_value : bitcast->m_source;
        if (source == nullptr) {
            return false;
        }

        std::string setup;
        std::string target_data, target_offset, target_stride, target_length;
        if (!get_c_rank1_array_access(target_expr, "__lfortran_char_lhs",
                setup, target_data, target_offset, target_stride, target_length)) {
            return false;
        }

        std::string value_data, value_length, value_setup, value_cleanup;
        if (!try_get_unit_step_string_section_view(
                source, value_data, value_length, value_setup)) {
            ASR::ttype_t *source_type =
                ASRUtils::type_get_past_allocatable_pointer(ASRUtils::expr_type(source));
            if (source_type == nullptr
                    || ASRUtils::is_array(source_type)
                    || !ASRUtils::is_character(*source_type)) {
                return false;
            }
            self().visit_expr(*source);
            value_data = src;
            value_setup += drain_tmp_buffer();
            value_setup += extract_stmt_setup_from_expr(value_data);
            materialize_allocating_string_expr(
                source, value_data, value_length, value_setup, value_cleanup);
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer();
        src += indent + "{\n";
        indentation_level++;
        std::string inner_indent(indentation_level * indentation_spaces, ' ');
        src += setup;
        src += value_setup;
        std::string index_name = get_unique_local_name("__lfortran_i");
        src += inner_indent + "for (int64_t " + index_name + " = 0; "
            + index_name + " < " + target_length + "; " + index_name + "++) {\n";
        indentation_level++;
        std::string loop_indent(indentation_level * indentation_spaces, ' ');
        std::string zero_name = get_unique_local_name("__lfortran_zero_char");
        src += loop_indent + "char " + zero_name + "[1] = {0};\n";
        std::string source_name = get_unique_local_name("__lfortran_char_src");
        src += loop_indent + "char* " + source_name + " = (" + index_name
            + " < " + value_length + ") ? (" + value_data + " + "
            + index_name + ") : " + zero_name + ";\n";
        src += loop_indent + "_lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &"
            + target_data + "[" + target_offset + " + " + index_name
            + " * " + target_stride + "], NULL, true, true, "
            + source_name + ", 1);\n";
        indentation_level--;
        src += inner_indent + "}\n";
        src += value_cleanup;
        indentation_level--;
        src += indent + "}\n";
        headers.insert("string.h");
        return true;
    }

    bool try_emit_c_char_array_bitcast_to_string_assignment(
            const ASR::Assignment_t &x, const std::string &target,
            ASR::ttype_t *target_type) {
        if (!is_c || !ASR::is_a<ASR::BitCast_t>(*x.m_value)) {
            return false;
        }
        ASR::ttype_t *target_scalar_type =
            ASRUtils::type_get_past_allocatable_pointer(target_type);
        if (target_scalar_type == nullptr
                || ASRUtils::is_array(target_scalar_type)
                || !ASRUtils::is_character(*target_scalar_type)) {
            return false;
        }
        ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(x.m_value);
        ASR::expr_t *source = bitcast->m_value ? bitcast->m_value : bitcast->m_source;
        if (source == nullptr ||
                !is_len_one_character_array_type(ASRUtils::expr_type(source))) {
            return false;
        }

        std::string setup;
        std::string source_data, source_offset, source_stride, source_length;
        if (!get_c_rank1_array_access(source, "__lfortran_char_transfer_src",
                setup, source_data, source_offset, source_stride, source_length)) {
            return false;
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string len_name = get_unique_local_name("__lfortran_transfer_len");
        std::string tmp_name = get_unique_local_name("__lfortran_transfer_str");
        std::string idx_name = get_unique_local_name("__lfortran_transfer_i");
        std::string elem_name = get_unique_local_name("__lfortran_transfer_elem");
        src = check_tmp_buffer();
        src += indent + "{\n";
        indentation_level++;
        std::string inner_indent(indentation_level * indentation_spaces, ' ');
        src += setup;
        src += inner_indent + "int64_t " + len_name + " = " + source_length + ";\n";
        src += inner_indent + "char *" + tmp_name
            + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
            + "(" + len_name + " + 1));\n";
        src += inner_indent + "for (int64_t " + idx_name + " = 0; "
            + idx_name + " < " + len_name + "; " + idx_name + "++) {\n";
        indentation_level++;
        std::string loop_indent(indentation_level * indentation_spaces, ' ');
        src += loop_indent + "char *" + elem_name + " = " + source_data + "["
            + source_offset + " + " + idx_name + " * " + source_stride + "];\n";
        src += loop_indent + tmp_name + "[" + idx_name + "] = ("
            + elem_name + " != NULL) ? " + elem_name + "[0] : '\\0';\n";
        indentation_level--;
        src += inner_indent + "}\n";
        src += inner_indent + tmp_name + "[" + len_name + "] = '\\0';\n";
        src += inner_indent + "_lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &"
            + target + ", NULL, true, true, " + tmp_name + ", " + len_name + ");\n";
        src += inner_indent + "_lfortran_free_alloc(_lfortran_get_default_allocator(), "
            + tmp_name + ");\n";
        indentation_level--;
        src += indent + "}\n";
        headers.insert("string.h");
        return true;
    }

    bool try_emit_c_char_array_bitcast_to_numeric_array_assignment(
            const ASR::Assignment_t &x, ASR::expr_t *target_expr) {
        if (!is_c || target_expr == nullptr ||
                !ASR::is_a<ASR::BitCast_t>(*x.m_value) ||
                !ASRUtils::is_array(ASRUtils::expr_type(target_expr)) ||
                !is_c_rank1_unit_array_expr(target_expr)) {
            return false;
        }
        ASR::ttype_t *target_elem_type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(target_expr)));
        if (target_elem_type == nullptr
                || ASRUtils::is_character(*target_elem_type)) {
            return false;
        }
        ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(x.m_value);
        ASR::expr_t *source = bitcast->m_value ? bitcast->m_value : bitcast->m_source;
        if (source == nullptr ||
                !is_len_one_character_array_type(ASRUtils::expr_type(source))) {
            return false;
        }

        std::string setup;
        std::string target_data, target_offset, target_stride, target_length;
        if (!get_c_rank1_array_access(target_expr, "__lfortran_transfer_lhs",
                setup, target_data, target_offset, target_stride, target_length)) {
            return false;
        }
        std::string source_data, source_offset, source_stride, source_length;
        if (!get_c_rank1_array_access(source, "__lfortran_transfer_src",
                setup, source_data, source_offset, source_stride, source_length)) {
            return false;
        }

        std::string elem_type = CUtils::get_c_type_from_ttype_t(target_elem_type);
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string elem_idx = get_unique_local_name("__lfortran_transfer_elem_i");
        std::string byte_idx = get_unique_local_name("__lfortran_transfer_byte_i");
        std::string src_idx = get_unique_local_name("__lfortran_transfer_src_i");
        std::string dst_bytes = get_unique_local_name("__lfortran_transfer_dst");
        std::string src_elem = get_unique_local_name("__lfortran_transfer_src_elem");
        src = check_tmp_buffer();
        src += indent + "{\n";
        indentation_level++;
        std::string inner_indent(indentation_level * indentation_spaces, ' ');
        src += setup;
        src += inner_indent + "int64_t " + src_idx + " = 0;\n";
        src += inner_indent + "for (int64_t " + elem_idx + " = 0; "
            + elem_idx + " < " + target_length + "; " + elem_idx + "++) {\n";
        indentation_level++;
        std::string loop_indent(indentation_level * indentation_spaces, ' ');
        src += loop_indent + "unsigned char *" + dst_bytes + " = (unsigned char*) &"
            + target_data + "[" + target_offset + " + " + elem_idx
            + " * " + target_stride + "];\n";
        src += loop_indent + "memset(" + dst_bytes + ", 0, sizeof(" + elem_type + "));\n";
        src += loop_indent + "for (int64_t " + byte_idx + " = 0; "
            + byte_idx + " < (int64_t) sizeof(" + elem_type + ") && "
            + src_idx + " < " + source_length + "; " + byte_idx + "++, "
            + src_idx + "++) {\n";
        indentation_level++;
        std::string byte_indent(indentation_level * indentation_spaces, ' ');
        src += byte_indent + "char *" + src_elem + " = " + source_data + "["
            + source_offset + " + " + src_idx + " * " + source_stride + "];\n";
        src += byte_indent + dst_bytes + "[" + byte_idx + "] = (unsigned char)(("
            + src_elem + " != NULL) ? " + src_elem + "[0] : '\\0');\n";
        indentation_level--;
        src += loop_indent + "}\n";
        indentation_level--;
        src += inner_indent + "}\n";
        indentation_level--;
        src += indent + "}\n";
        headers.insert("string.h");
        return true;
    }

    bool try_emit_c_array_section_reshape_assignment(const ASR::Assignment_t &x,
            ASR::expr_t *target_expr) {
        if (!is_c || target_expr == nullptr ||
                !ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ||
                !is_c_rank1_unit_array_expr(target_expr)) {
            return false;
        }
        ASR::ttype_t *target_type = ASRUtils::expr_type(target_expr);
        ASR::ttype_t *target_element_type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable_pointer(target_type));
        if (target_element_type == nullptr || ASRUtils::is_character(*target_element_type)) {
            return false;
        }

        std::string setup;
        std::string target_data, target_offset, target_stride, target_length;
        if (!get_c_rank1_array_access(target_expr, "__lfortran_reshape_lhs",
                setup, target_data, target_offset, target_stride, target_length)) {
            return false;
        }

        self().visit_expr(*x.m_value);
        std::string value_expr = src;
        std::string value_setup = drain_tmp_buffer();
        value_setup += extract_stmt_setup_from_expr(value_expr);
        std::string value_type = get_c_declared_array_wrapper_type_name(
            ASRUtils::expr_type(x.m_value));
        if (value_type.empty()) {
            return false;
        }
        std::string value_name = get_unique_local_name("__lfortran_reshape_value");
        std::string index_name = get_unique_local_name("__lfortran_i");

        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer();
        src += indent + "{\n";
        indentation_level++;
        std::string inner_indent(indentation_level * indentation_spaces, ' ');
        src += setup;
        src += value_setup;
        src += inner_indent + value_type + " *" + value_name + " = "
            + value_expr + ";\n";
        src += inner_indent + "for (int64_t " + index_name + " = 0; "
            + index_name + " < " + target_length + "; " + index_name + "++) {\n";
        indentation_level++;
        std::string loop_indent(indentation_level * indentation_spaces, ' ');
        src += loop_indent + target_data + "[" + target_offset + " + "
            + index_name + " * " + target_stride + "] = " + value_name
            + "->data[" + index_name + "];\n";
        indentation_level--;
        src += inner_indent + "}\n";
        src += inner_indent + "free(" + value_name + "->data);\n";
        src += inner_indent + "free(" + value_name + ");\n";
        indentation_level--;
        src += indent + "}\n";
        headers.insert("stdlib.h");
        return true;
    }

    bool try_emit_c_array_expr_assignment_plan(
            const CArrayExprLoweringPlan &plan) {
        if (try_emit_c_rank2_full_self_update_assignment(plan)) {
            return true;
        }
        if (try_emit_c_scalarized_array_expr_assignment(plan)) {
            return true;
        }
        if (plan.kind != CArrayExprLoweringKind::CompactConstantCopy
                || plan.target_expr == nullptr
                || plan.target_base_expr == nullptr
                || plan.constant_value == nullptr
                || plan.constant_array_type == nullptr
                || plan.constant_element_type == nullptr) {
            return false;
        }
        ASR::ArrayConstant_t *arr = plan.constant_value;
        ASR::ttype_t *value_type = plan.constant_array_type;
        ASR::ttype_t *element_type = plan.constant_element_type;
        size_t n = plan.constant_size;

        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string setup;
        std::string target_data, target_offset, target_stride, target_length;
        if (!get_c_rank1_array_access(plan.target_expr, "__lfortran_const_lhs",
                setup, target_data, target_offset, target_stride, target_length,
                false)) {
            return false;
        }

        std::string c_element_type = CUtils::get_c_type_from_ttype_t(element_type);
        if (emit_compact_constant_data_units) {
            std::string helper_name = register_compact_array_constant_data_helper(
                arr, value_type, element_type, c_element_type);
            if (!helper_name.empty()) {
                std::string start_name = get_unique_local_name("__lfortran_const_start");
                std::string stride_name = get_unique_local_name("__lfortran_const_stride");
                src = check_tmp_buffer();
                src += setup;
                src += indent + "{\n";
                std::string inner_indent = indent + std::string(indentation_spaces, ' ');
                src += inner_indent + "int64_t " + stride_name + " = "
                    + target_stride + ";\n";
                src += inner_indent + "int64_t " + start_name + " = "
                    + target_offset + ";\n";
                src += inner_indent + helper_name + "(" + target_data + " + "
                    + start_name + ", " + stride_name + ");\n";
                src += indent + "}\n";
                return true;
            }
        }

        std::string const_name = get_unique_local_name("__lfortran_const_data");
        std::string idx_name = get_unique_local_name("__lfortran_const_i");
        std::string start_name = get_unique_local_name("__lfortran_const_start");
        std::string stride_name = get_unique_local_name("__lfortran_const_stride");

        headers.insert("string.h");
        src = check_tmp_buffer();
        src += setup;
        src += indent + "{\n";
        std::string inner_indent = indent + std::string(indentation_spaces, ' ');
        src += inner_indent + "static const " + c_element_type + " " + const_name
            + "[" + std::to_string(n) + "] = {\n";
        std::string data_indent = inner_indent + std::string(indentation_spaces, ' ');
        for (size_t i = 0; i < n; i++) {
            if (i % 8 == 0) {
                src += data_indent;
            }
            src += get_c_array_constant_init_element_for_c_index(
                arr, value_type, element_type, i);
            if (i + 1 < n) {
                src += ", ";
            }
            if (i % 8 == 7 || i + 1 == n) {
                src += "\n";
            }
        }
        src += inner_indent + "};\n";
        src += inner_indent + "int64_t " + stride_name + " = "
            + target_stride + ";\n";
        src += inner_indent + "int64_t " + start_name + " = "
            + target_offset + ";\n";
        src += inner_indent + "if (" + stride_name + " == 1) {\n";
        src += inner_indent + std::string(indentation_spaces, ' ')
            + "memcpy(" + target_data + " + " + start_name + ", "
            + const_name + ", sizeof(" + const_name + "));\n";
        src += inner_indent + "} else {\n";
        src += inner_indent + std::string(indentation_spaces, ' ')
            + "for (int64_t " + idx_name + " = 0; " + idx_name + " < "
            + std::to_string(n) + "; " + idx_name + "++) {\n";
        src += inner_indent + std::string(indentation_spaces * 2, ' ')
            + target_data + "[" + start_name + " + " + idx_name + " * "
            + stride_name + "] = " + const_name + "[" + idx_name + "];\n";
        src += inner_indent + std::string(indentation_spaces, ' ') + "}\n";
        src += inner_indent + "}\n";
        src += indent + "}\n";
        return true;
    }

    bool is_len_one_character_array_type(ASR::ttype_t *type) {
        return CUtils::is_len_one_character_array_type(type);
    }

    ASR::symbol_t *get_c_array_element_type_declaration_symbol(ASR::ttype_t *type,
            ASR::symbol_t *type_decl=nullptr) {
        ASR::ttype_t *array_base = get_c_array_wrapper_base_type(type);
        if (array_base == nullptr) {
            return nullptr;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(array_base);
        if (element_type == nullptr) {
            return nullptr;
        }
        type_decl = ASRUtils::symbol_get_past_external(type_decl);
        if (type_decl != nullptr && ASR::is_a<ASR::Variable_t>(*type_decl)) {
            type_decl = ASR::down_cast<ASR::Variable_t>(type_decl)->m_type_declaration;
            type_decl = ASRUtils::symbol_get_past_external(type_decl);
        }
        if (type_decl == nullptr) {
            return nullptr;
        }
        if (ASR::is_a<ASR::StructType_t>(*element_type)
                && ASR::is_a<ASR::Struct_t>(*type_decl)) {
            return type_decl;
        }
        if (ASR::is_a<ASR::UnionType_t>(*element_type)
                && ASR::is_a<ASR::Union_t>(*type_decl)) {
            return type_decl;
        }
        if (ASR::is_a<ASR::EnumType_t>(*element_type)
                && ASR::is_a<ASR::Enum_t>(*type_decl)) {
            return type_decl;
        }
        return nullptr;
    }

    std::string get_c_array_wrapper_type_name(ASR::ttype_t *type,
            ASR::symbol_t *type_decl=nullptr) {
        LCOMPILERS_ASSERT(type != nullptr);
        std::string array_type_name;
        std::string array_type_code;
        ASR::symbol_t *element_type_decl =
            get_c_array_element_type_declaration_symbol(type, type_decl);
        if (element_type_decl != nullptr) {
            element_type_decl = ASRUtils::symbol_get_past_external(element_type_decl);
            array_type_name = get_c_concrete_type_from_ttype_t(
                ASRUtils::type_get_past_array(get_c_array_wrapper_base_type(type)),
                element_type_decl);
            array_type_code = CUtils::sanitize_c_identifier(
                "x" + CUtils::get_c_symbol_name(element_type_decl));
        } else {
            array_type_name = CUtils::get_c_array_element_type_from_ttype_t(type);
            array_type_code = CUtils::get_c_array_type_code(type);
        }
        return c_ds_api->get_array_type(array_type_name, array_type_code, array_types_decls, false);
    }

    ASR::ttype_t *get_c_array_wrapper_base_type(ASR::ttype_t *type) {
        if (!is_c || type == nullptr) {
            return nullptr;
        }
        if (ASRUtils::is_pointer(type) && !ASRUtils::is_array(type)) {
            type = ASRUtils::type_get_past_pointer(type);
        }
        type = ASRUtils::type_get_past_allocatable_pointer(type);
        if (type == nullptr || !ASRUtils::is_array(type)) {
            return nullptr;
        }
        return type;
    }

    bool are_compatible_c_array_wrapper_types(ASR::ttype_t *target_type,
            ASR::ttype_t *value_type) {
        ASR::ttype_t *target_base = get_c_array_wrapper_base_type(target_type);
        ASR::ttype_t *value_base = get_c_array_wrapper_base_type(value_type);
        if (target_base == nullptr || value_base == nullptr) {
            return false;
        }
        if (ASRUtils::extract_n_dims_from_ttype(target_base)
                != ASRUtils::extract_n_dims_from_ttype(value_base)) {
            return false;
        }
        return CUtils::get_c_type_from_ttype_t(ASRUtils::type_get_past_array(target_base))
            == CUtils::get_c_type_from_ttype_t(ASRUtils::type_get_past_array(value_base));
    }

    bool have_compatible_c_array_wrapper_element_type(ASR::ttype_t *target_type,
            ASR::ttype_t *value_type) {
        ASR::ttype_t *target_base = get_c_array_wrapper_base_type(target_type);
        ASR::ttype_t *value_base = get_c_array_wrapper_base_type(value_type);
        if (target_base == nullptr || value_base == nullptr) {
            return false;
        }
        return CUtils::get_c_type_from_ttype_t(ASRUtils::type_get_past_array(target_base))
            == CUtils::get_c_type_from_ttype_t(ASRUtils::type_get_past_array(value_base));
    }

    std::string get_c_declared_array_wrapper_type_name(ASR::ttype_t *type,
            ASR::symbol_t *type_decl=nullptr) {
        ASR::ttype_t *array_base = get_c_array_wrapper_base_type(type);
        if (array_base == nullptr) {
            return "";
        }
        if (get_c_array_element_type_declaration_symbol(type, type_decl) != nullptr) {
            return get_c_array_wrapper_type_name(array_base, type_decl);
        }
        if (!ASRUtils::is_allocatable(type)) {
            return get_c_array_wrapper_type_name(array_base, type_decl);
        }
        std::string array_type_name =
            CUtils::get_c_array_element_type_from_ttype_t(array_base);
        std::string array_type_code = CUtils::get_c_type_code(type);
        return c_ds_api->get_array_type(array_type_name, array_type_code,
            array_types_decls, false);
    }

    std::string cast_c_array_wrapper_ptr_to_target_type(ASR::ttype_t *target_type,
            ASR::ttype_t *value_type, const std::string &value_expr,
            ASR::symbol_t *target_type_decl=nullptr,
            ASR::symbol_t *value_type_decl=nullptr) {
        if (!are_compatible_c_array_wrapper_types(target_type, value_type)) {
            return value_expr;
        }
        std::string target_wrapper =
            get_c_declared_array_wrapper_type_name(target_type, target_type_decl);
        std::string value_wrapper =
            get_c_declared_array_wrapper_type_name(value_type, value_type_decl);
        if (target_wrapper.empty() || value_wrapper.empty()) {
            return value_expr;
        }
        if (target_type_decl == nullptr && value_type_decl != nullptr) {
            return value_expr;
        }
        if (target_wrapper == value_wrapper) {
            return value_expr;
        }
        return "((" + target_wrapper + "*)(" + value_expr + "))";
    }

    std::string get_c_array_is_allocated_expr(ASR::expr_t *expr, const std::string &expr_src) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return "false";
        }
        ASR::expr_t *unwrapped_expr = ASRUtils::expr_value(expr);
        if (unwrapped_expr == nullptr) {
            unwrapped_expr = expr;
        }
        if (is_data_only_array_expr(unwrapped_expr)
                || is_fixed_size_array_storage_expr(unwrapped_expr)) {
            return "false";
        }
        return expr_src + "->is_allocated";
    }

    std::string get_c_dimension_expr_src(ASR::expr_t *dim_expr,
            const std::map<std::string, std::string> *dim_expr_overrides) {
        self().visit_expr(*dim_expr);
        std::string result = src;
        if (dim_expr_overrides) {
            auto override_it = dim_expr_overrides->find(result);
            if (override_it != dim_expr_overrides->end()) {
                return override_it->second;
            }
            for (const auto &override_entry: *dim_expr_overrides) {
                const std::string &name = override_entry.first;
                if (result.rfind(name + "->", 0) == 0 ||
                        result.rfind(name + ".", 0) == 0) {
                    return "(" + override_entry.second + ")" + result.substr(name.size());
                }
                bool replaced = false;
                auto is_identifier_char = [](char c) {
                    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                           (c >= '0' && c <= '9') || c == '_';
                };
                std::array<std::string, 2> member_markers = {name + "->", name + "."};
                for (const std::string &marker: member_markers) {
                    size_t pos = result.find(marker);
                    while (pos != std::string::npos) {
                        if (pos == 0 || !is_identifier_char(result[pos - 1])) {
                            result.replace(pos, name.size(),
                                "(" + override_entry.second + ")");
                            replaced = true;
                            pos += override_entry.second.size() + 2;
                        } else {
                            pos += marker.size();
                        }
                        pos = result.find(marker, pos);
                    }
                }
                if (replaced) {
                    return result;
                }
            }
            ASR::expr_t *raw_dim_expr = unwrap_c_lvalue_expr(dim_expr);
            if (raw_dim_expr && ASR::is_a<ASR::Var_t>(*raw_dim_expr)) {
                ASR::symbol_t *dim_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(raw_dim_expr)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*dim_sym)) {
                    std::string dim_name = CUtils::get_c_variable_name(
                        *ASR::down_cast<ASR::Variable_t>(dim_sym));
                    override_it = dim_expr_overrides->find(dim_name);
                    if (override_it != dim_expr_overrides->end()) {
                        return override_it->second;
                    }
                }
            }
        }
        return result;
    }

    std::string build_c_array_wrapper_from_cast_target(ASR::ttype_t *target_type,
            ASR::expr_t *source_expr, const std::string &source_src,
            const std::map<std::string, std::string> *dim_expr_overrides=nullptr) {
        if (!is_c || target_type == nullptr || source_expr == nullptr) {
            return source_src;
        }
        std::string source_src_copy = source_src;
        auto build_sequence_array_item_wrapper = [&]() -> std::string {
            ASR::expr_t *source_lvalue = unwrap_c_lvalue_expr(source_expr);
            if (source_lvalue == nullptr
                    || !ASR::is_a<ASR::ArrayItem_t>(*source_lvalue)) {
                return "";
            }
            ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(source_lvalue);
            ASR::expr_t *base_expr = unwrap_c_array_expr(item->m_v);
            if (base_expr == nullptr) {
                return "";
            }
            ASR::ttype_t *target_array_type =
                ASRUtils::type_get_past_allocatable_pointer(target_type);
            ASR::dimension_t *target_dims = nullptr;
            int target_rank = ASRUtils::extract_dimensions_from_ttype(
                target_array_type, target_dims);
            if (target_rank <= 0) {
                return "";
            }
            ASR::ttype_t *target_element_type =
                ASRUtils::type_get_past_array(target_array_type);
            ASR::ttype_t *source_element_type = ASRUtils::expr_type(source_lvalue);
            if (target_element_type == nullptr || source_element_type == nullptr
                    || !ASRUtils::check_equal_type(
                        ASRUtils::type_get_past_allocatable_pointer(target_element_type),
                        ASRUtils::type_get_past_allocatable_pointer(source_element_type),
                        nullptr, nullptr)) {
                return "";
            }
            std::string target_wrapper =
                get_c_declared_array_wrapper_type_name(target_type);
            if (target_wrapper.empty()) {
                return "";
            }
            ASR::ttype_t *base_array_type =
                ASRUtils::type_get_past_allocatable_pointer(
                    ASRUtils::expr_type(base_expr));
            ASR::dimension_t *base_dims = nullptr;
            int base_rank = ASRUtils::extract_dimensions_from_ttype(
                base_array_type, base_dims);
            if (base_rank <= 0 || item->n_args == 0
                    || target_rank > base_rank) {
                return "";
            }

            std::string saved_src = src;
            self().visit_expr(*base_expr);
            std::string base_src = src;
            std::vector<std::string> item_indices(base_rank);
            for (int i = 0; i < base_rank; i++) {
                ASR::expr_t *idx_expr = nullptr;
                if (i < (int)item->n_args) {
                    idx_expr = get_array_index_expr(item->m_args[i]);
                }
                if (idx_expr != nullptr) {
                    self().visit_expr(*idx_expr);
                    item_indices[i] = src;
                } else {
                    item_indices[i] = base_src + "->dims[" + std::to_string(i)
                        + "].lower_bound";
                }
            }
            src = saved_src;

            std::string offset = base_src + "->offset";
            for (int i = 0; i < base_rank; i++) {
                std::string source_dim = base_src + "->dims[" + std::to_string(i) + "]";
                offset = "(" + offset + " + (" + source_dim + ".stride * (("
                    + item_indices[i] + ") - " + source_dim + ".lower_bound)))";
            }

            std::vector<std::string> dim_inits;
            dim_inits.reserve(target_rank);
            std::string synthesized_stride;
            for (int i = 0; i < target_rank; i++) {
                std::string lower_bound = "1";
                if (target_dims != nullptr && target_dims[i].m_start != nullptr) {
                    lower_bound = get_c_dimension_expr_src(
                        target_dims[i].m_start, dim_expr_overrides);
                }
                std::string length;
                if (target_dims != nullptr && target_dims[i].m_length != nullptr) {
                    length = get_c_dimension_expr_src(
                        target_dims[i].m_length, dim_expr_overrides);
                } else if (i < base_rank) {
                    std::string source_dim = base_src + "->dims[" + std::to_string(i) + "]";
                    length = "(((" + source_dim + ".length + " + source_dim
                        + ".lower_bound - 1) - (" + item_indices[i] + ")) + 1)";
                } else {
                    length = "1";
                }
                std::string stride;
                if (i < base_rank) {
                    stride = base_src + "->dims[" + std::to_string(i) + "].stride";
                } else if (!synthesized_stride.empty()) {
                    stride = synthesized_stride;
                } else {
                    stride = "1";
                }
                synthesized_stride = "(" + stride + " * (" + length + "))";
                dim_inits.push_back("{"
                    + lower_bound + ", " + length + ", " + stride + "}");
            }

            std::string dims_init;
            for (size_t i = 0; i < dim_inits.size(); i++) {
                if (i > 0) {
                    dims_init += ", ";
                }
                dims_init += dim_inits[i];
            }
            return "(&(" + target_wrapper + "){ .data = " + base_src
                + "->data, .dims = {" + dims_init + "}, .n_dims = "
                + std::to_string(target_rank) + ", .offset = " + offset
                + ", .is_allocated = " + base_src + "->is_allocated })";
        };
        std::string sequence_item_wrapper = build_sequence_array_item_wrapper();
        if (!sequence_item_wrapper.empty()) {
            return sequence_item_wrapper;
        }
        ASR::ttype_t *source_type = ASRUtils::expr_type(source_expr);
        if (!have_compatible_c_array_wrapper_element_type(target_type, source_type)) {
            return source_src_copy;
        }
        ASR::ttype_t *target_array_type =
            ASRUtils::type_get_past_allocatable_pointer(target_type);
        ASR::dimension_t *target_dims = nullptr;
        int target_rank = ASRUtils::extract_dimensions_from_ttype(
            target_array_type, target_dims);
        ASR::ttype_t *source_array_type =
            ASRUtils::type_get_past_allocatable_pointer(source_type);
        ASR::dimension_t *source_dims = nullptr;
        int source_rank = ASRUtils::extract_dimensions_from_ttype(
            source_array_type, source_dims);
        if (target_rank > 0 && source_rank > 0 && target_rank != source_rank) {
            if (target_rank < source_rank) {
                return source_src_copy;
            }
            std::string target_wrapper = get_c_declared_array_wrapper_type_name(target_type);
            if (target_wrapper.empty()) {
                return source_src_copy;
            }
            for (int i = 0; i < target_rank; i++) {
                if (target_dims[i].m_length == nullptr) {
                    return source_src_copy;
                }
            }

            std::string source_data = get_c_array_data_expr(source_expr, source_src_copy);
            std::string source_offset = get_c_array_offset_expr(source_expr, source_src_copy);
            std::string source_is_allocated = get_c_array_is_allocated_expr(source_expr, source_src_copy);
            std::string saved_src = src;
            std::vector<std::string> target_lbs(target_rank);
            std::vector<std::string> target_lengths(target_rank);
            for (int i = 0; i < target_rank; i++) {
                std::string lower_bound = "1";
                if (target_dims[i].m_start != nullptr) {
                    lower_bound = get_c_dimension_expr_src(
                        target_dims[i].m_start, dim_expr_overrides);
                }
                src = get_c_dimension_expr_src(
                    target_dims[i].m_length, dim_expr_overrides);
                target_lbs[i] = lower_bound;
                target_lengths[i] = src;
            }

            std::vector<std::string> source_strides(source_rank);
            if (is_data_only_array_expr(source_expr)
                    || is_fixed_size_array_storage_expr(source_expr)) {
                std::string source_stride = "1";
                for (int i = 0; i < source_rank; i++) {
                    source_strides[i] = source_stride;
                    std::string source_length = "1";
                    if (source_dims[i].m_length != nullptr) {
                        self().visit_expr(*source_dims[i].m_length);
                        source_length = src;
                    }
                    source_stride = "(" + source_stride + " * (" + source_length + "))";
                }
            } else {
                for (int i = 0; i < source_rank; i++) {
                    source_strides[i] = source_src_copy + "->dims[" + std::to_string(i) + "].stride";
                }
            }
            src = saved_src;

            std::vector<std::string> target_strides(target_rank);
            int target_dim = 0;
            for (int source_dim = 0; source_dim < source_rank; source_dim++) {
                int group_end = (source_dim == source_rank - 1)
                    ? target_rank : target_dim + 1;
                std::string target_stride = source_strides[source_dim];
                for (; target_dim < group_end; target_dim++) {
                    target_strides[target_dim] = target_stride;
                    target_stride = "(" + target_stride + " * ("
                        + target_lengths[target_dim] + "))";
                }
            }

            std::string dims_init;
            for (int i = 0; i < target_rank; i++) {
                if (i > 0) {
                    dims_init += ", ";
                }
                dims_init += "{"
                    + target_lbs[i] + ", "
                    + target_lengths[i] + ", "
                    + target_strides[i] + "}";
            }
            return "(&(" + target_wrapper + "){ .data = " + source_data
                + ", .dims = {" + dims_init + "}, .n_dims = "
                + std::to_string(target_rank)
                + ", .offset = " + source_offset
                + ", .is_allocated = " + source_is_allocated + " })";
        }
        if (!is_data_only_array_expr(source_expr)
                && !is_fixed_size_array_storage_expr(source_expr)) {
            return cast_c_array_wrapper_ptr_to_target_type(
                target_type, source_type, source_src_copy, nullptr,
                get_expr_type_declaration_symbol(source_expr));
        }
        std::string target_wrapper = get_c_declared_array_wrapper_type_name(target_type);
        if (target_wrapper.empty()) {
            return source_src_copy;
        }
        if (source_rank <= 0) {
            return source_src_copy;
        }

        std::string source_data = get_c_array_data_expr(source_expr, source_src_copy);
        std::string source_offset = get_c_array_offset_expr(source_expr, source_src_copy);
        std::string source_is_allocated = get_c_array_is_allocated_expr(source_expr, source_src_copy);
        std::string saved_src = src;
        std::vector<std::string> dim_inits(source_rank);
        std::string stride = "1";
        for (int i = 0; i < source_rank; i++) {
            std::string lower_bound = "1";
            std::string length = "1";
            if (source_dims[i].m_start != nullptr) {
                self().visit_expr(*source_dims[i].m_start);
                lower_bound = src;
            }
            if (source_dims[i].m_length != nullptr) {
                self().visit_expr(*source_dims[i].m_length);
                length = src;
            }
            dim_inits[i] = "{"
                + lower_bound + ", "
                + length + ", "
                + stride + "}";
            stride = "(" + stride + " * (" + length + "))";
        }
        src = saved_src;

        std::string dims_init;
        for (int i = 0; i < source_rank; i++) {
            if (i > 0) {
                dims_init += ", ";
            }
            dims_init += dim_inits[i];
        }
        return "(&(" + target_wrapper + "){ .data = " + source_data
            + ", .dims = {" + dims_init + "}, .n_dims = " + std::to_string(source_rank)
            + ", .offset = " + source_offset
            + ", .is_allocated = " + source_is_allocated + " })";
    }

    std::string build_c_array_no_copy_descriptor_view(
            ASR::ttype_t *target_type, ASR::expr_t *source_expr,
            const std::string &source_src,
            ASR::symbol_t *target_type_decl=nullptr,
            bool use_named_stack_view=true) {
        if (!is_c || target_type == nullptr || source_expr == nullptr) {
            return source_src;
        }
        std::string source_src_copy = source_src;
        ASR::expr_t *source_view_expr = unwrap_c_lvalue_expr(source_expr);
        ASR::ArraySection_t *source_section = nullptr;
        ASR::expr_t *source_lvalue = source_view_expr;
        if (source_view_expr != nullptr
                && ASR::is_a<ASR::ArraySection_t>(*source_view_expr)) {
            source_section = ASR::down_cast<ASR::ArraySection_t>(source_view_expr);
            source_lvalue = unwrap_c_lvalue_expr(source_section->m_v);
        }
        if (source_lvalue == nullptr
                || !(ASR::is_a<ASR::Var_t>(*source_lvalue)
                    || ASR::is_a<ASR::StructInstanceMember_t>(*source_lvalue))) {
            return source_src_copy;
        }
        ASR::ttype_t *source_type = ASRUtils::expr_type(source_view_expr);
        ASR::ttype_t *source_base_type = ASRUtils::expr_type(source_lvalue);
        ASR::ttype_t *source_array_type =
            ASRUtils::type_get_past_allocatable_pointer(source_type);
        ASR::ttype_t *source_base_array_type =
            ASRUtils::type_get_past_allocatable_pointer(source_base_type);
        if (ASR::is_a<ASR::Var_t>(*source_lvalue)) {
            ASR::symbol_t *source_sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(source_lvalue)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*source_sym)) {
                source_src_copy = CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(source_sym));
            }
        }
        if (!have_compatible_c_array_wrapper_element_type(target_type, source_type)
                || is_data_only_array_expr(source_expr)
                || is_fixed_size_array_storage_expr(source_expr)) {
            return source_src_copy;
        }
        if (ASRUtils::is_allocatable(target_type) || ASRUtils::is_pointer(target_type)) {
            return source_src_copy;
        }

        ASR::ttype_t *target_array_type =
            ASRUtils::type_get_past_allocatable_pointer(target_type);
        ASR::dimension_t *target_dims = nullptr;
        int target_rank = ASRUtils::extract_dimensions_from_ttype(
            target_array_type, target_dims);
        ASR::dimension_t *source_dims = nullptr;
        int source_rank = ASRUtils::extract_dimensions_from_ttype(
            source_array_type, source_dims);
        if (target_rank <= 0 || target_rank != source_rank) {
            return source_src_copy;
        }
        std::string target_wrapper =
            get_c_declared_array_wrapper_type_name(
                target_array_type, target_type_decl);
        if (target_wrapper.empty()) {
            return source_src_copy;
        }
        if (source_section == nullptr
                && is_c_array_section_association_temp_expr(source_lvalue)) {
            std::string source_wrapper =
                get_c_declared_array_wrapper_type_name(source_array_type);
            if (source_wrapper == target_wrapper) {
                return source_src_copy;
            }
        }

        if (source_section != nullptr) {
            ASR::dimension_t *source_base_dims = nullptr;
            int source_base_rank = ASRUtils::extract_dimensions_from_ttype(
                source_base_array_type, source_base_dims);
            if ((int) source_section->n_args != source_base_rank) {
                return source_src_copy;
            }

            std::string saved_src = src;
            std::string setup;
            auto section_bound_src = [&](ASR::expr_t *expr,
                    const std::string &fallback) -> std::string {
                if (expr == nullptr) {
                    return fallback;
                }
                self().visit_expr(*expr);
                std::string expr_src = src;
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(expr_src);
                return expr_src;
            };

            std::vector<std::string> dim_inits;
            dim_inits.reserve(target_rank);
            std::vector<std::string> offset_terms;
            offset_terms.push_back(source_src_copy + "->offset");
            int target_dim = 0;
            for (int source_dim = 0; source_dim < source_base_rank; source_dim++) {
                ASR::array_index_t idx = source_section->m_args[source_dim];
                std::string base_lb = source_src_copy + "->dims["
                    + std::to_string(source_dim) + "].lower_bound";
                std::string base_ub = "(" + base_lb + " + "
                    + source_src_copy + "->dims[" + std::to_string(source_dim)
                    + "].length - 1)";
                std::string base_stride = source_src_copy + "->dims["
                    + std::to_string(source_dim) + "].stride";
                bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
                if (!is_slice) {
                    std::string item = section_bound_src(idx.m_right, base_lb);
                    offset_terms.push_back(base_stride + " * ((" + item
                        + ") - " + base_lb + ")");
                    continue;
                }

                if (target_dim >= target_rank) {
                    return source_src_copy;
                }
                std::string left = section_bound_src(idx.m_left, base_lb);
                std::string right = section_bound_src(idx.m_right, base_ub);
                std::string step = section_bound_src(idx.m_step, "1");
                std::string lower_bound = "1";
                if (target_dims[target_dim].m_start != nullptr) {
                    lower_bound = section_bound_src(
                        target_dims[target_dim].m_start, "1");
                }
                std::string length = "((((" + right + ") - (" + left
                    + ")) / (" + step + ")) + 1)";
                std::string stride = "(" + base_stride + " * (" + step + "))";
                dim_inits.push_back("{" + lower_bound + ", " + length
                    + ", " + stride + "}");
                offset_terms.push_back(base_stride + " * ((" + left
                    + ") - " + base_lb + ")");
                target_dim++;
            }
            src = saved_src;
            if (target_dim != target_rank) {
                return source_src_copy;
            }

            std::string dims_init;
            for (size_t i = 0; i < dim_inits.size(); i++) {
                if (i > 0) {
                    dims_init += ", ";
                }
                dims_init += dim_inits[i];
            }
            std::string offset = "(";
            for (size_t i = 0; i < offset_terms.size(); i++) {
                if (i > 0) {
                    offset += " + ";
                }
                offset += offset_terms[i];
            }
            offset += ")";
            std::string view_init = "{ .data = " + source_src_copy
                + "->data, .dims = {" + dims_init + "}, .n_dims = "
                + std::to_string(target_rank) + ", .offset = " + offset
                + ", .is_allocated = " + source_src_copy + "->is_allocated }";
            if (!setup.empty()) {
                tmp_buffer_src.push_back(setup);
            }
            if (!use_named_stack_view) {
                return "(&(" + target_wrapper + ")" + view_init + ")";
            }
            std::string indent(indentation_level * indentation_spaces, ' ');
            std::string view_name = get_unique_local_name("__lfortran_array_view");
            tmp_buffer_src.push_back(indent + target_wrapper + " " + view_name
                + " = " + view_init + ";\n");
            return "&" + view_name;
        }

        std::string saved_src = src;
        std::string dims_init;
        for (int i = 0; i < target_rank; i++) {
            std::string lower_bound = "1";
            if (target_dims[i].m_start != nullptr) {
                self().visit_expr(*target_dims[i].m_start);
                lower_bound = src;
            }
            if (i > 0) {
                dims_init += ", ";
            }
            dims_init += "{"
                + lower_bound + ", "
                + source_src_copy + "->dims[" + std::to_string(i) + "].length, "
                + source_src_copy + "->dims[" + std::to_string(i) + "].stride}";
        }
        src = saved_src;

        std::string view_name = get_unique_local_name("__lfortran_array_view");
        std::string data_ptr = get_c_array_data_expr(source_expr, source_src_copy);
        std::string offset = get_c_array_offset_expr(source_expr, source_src_copy);
        std::string is_allocated = get_c_array_is_allocated_expr(source_expr, source_src_copy);
        if (!use_named_stack_view) {
            return "(&(" + target_wrapper + "){ .data = " + data_ptr
                + ", .dims = {" + dims_init + "}, .n_dims = "
                + std::to_string(target_rank)
                + ", .offset = " + offset
                + ", .is_allocated = " + is_allocated + " })";
        }
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string view_decl = indent + target_wrapper + " " + view_name
            + " = { .data = " + data_ptr
            + ", .dims = {" + dims_init + "}, .n_dims = "
            + std::to_string(target_rank)
            + ", .offset = " + offset
            + ", .is_allocated = " + is_allocated + " };\n";
        tmp_buffer_src.push_back(view_decl);
        return "&" + view_name;
    }

    CArrayExprLoweringPlan plan_c_array_argument_expr(
            ASR::expr_t *call_arg, ASR::ttype_t *param_type) {
        CArrayExprLoweringPlan plan;
        if (!is_c || call_arg == nullptr || param_type == nullptr) {
            return plan;
        }
        ASR::expr_t *source_arg = call_arg;
        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*call_arg)) {
            ASR::ArrayPhysicalCast_t *cast =
                ASR::down_cast<ASR::ArrayPhysicalCast_t>(call_arg);
            if ((cast->m_new != ASR::array_physical_typeType::DescriptorArray
                        && cast->m_new != ASR::array_physical_typeType::PointerArray)
                    || (cast->m_old != ASR::array_physical_typeType::DescriptorArray
                        && cast->m_old != ASR::array_physical_typeType::FixedSizeArray)) {
                return plan;
            }
            source_arg = cast->m_arg;
        }
        if (get_c_array_wrapper_base_type(param_type) == nullptr) {
            return plan;
        }
        plan.kind = CArrayExprLoweringKind::NoCopyDescriptorView;
        plan.target_expr = source_arg;
        return plan;
    }

    bool try_build_c_array_no_copy_descriptor_view_arg(
            ASR::expr_t *call_arg, ASR::ttype_t *param_type,
            ASR::symbol_t *param_type_decl, std::string &arg_src,
            bool use_named_stack_view=true) {
        CArrayExprLoweringPlan plan =
            plan_c_array_argument_expr(call_arg, param_type);
        if (plan.kind != CArrayExprLoweringKind::NoCopyDescriptorView
                || plan.target_expr == nullptr) {
            return false;
        }
        std::string view_src = build_c_array_no_copy_descriptor_view(
            param_type, plan.target_expr, arg_src, param_type_decl,
            use_named_stack_view);
        if (view_src == arg_src) {
            return false;
        }
        arg_src = view_src;
        src = view_src;
        return true;
    }

    std::string get_c_descriptor_member_base_expr(const std::string &expr) {
        size_t begin = expr.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            return expr;
        }
        if (expr[begin] == '&') {
            return "(" + expr.substr(begin) + ")";
        }
        return expr;
    }

    std::string build_c_char_array_descriptor_to_raw_arg(
            const std::string &descriptor_src, std::string &setup) {
        std::string base = get_c_descriptor_member_base_expr(descriptor_src);
        std::string len_name = get_unique_local_name("__lfortran_cchar_raw_len");
        std::string data_name = get_unique_local_name("__lfortran_cchar_raw_data");
        std::string idx_name = get_unique_local_name("__lfortran_cchar_raw_i");
        std::string elem_name = get_unique_local_name("__lfortran_cchar_raw_elem");
        std::string indent(indentation_level * indentation_spaces, ' ');
        setup += indent + "int64_t " + len_name + " = " + base + "->dims[0].length;\n";
        setup += indent + "char *" + data_name
            + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
            + "(" + len_name + " + 1));\n";
        setup += indent + "for (int64_t " + idx_name + " = 0; "
            + idx_name + " < " + len_name + "; " + idx_name + "++) {\n";
        setup += indent + std::string(indentation_spaces, ' ')
            + "char *" + elem_name + " = " + base + "->data[" + base
            + "->offset + " + idx_name + " * " + base + "->dims[0].stride];\n";
        setup += indent + std::string(indentation_spaces, ' ')
            + data_name + "[" + idx_name + "] = (" + elem_name
            + " != NULL) ? " + elem_name + "[0] : '\\0';\n";
        setup += indent + "}\n";
        setup += indent + data_name + "[" + len_name + "] = '\\0';\n";
        return data_name;
    }

    bool try_emit_scalar_to_char_array_bitcast_expr(ASR::expr_t *expr, std::string &out_expr) {
        if (!is_c || expr == nullptr || !ASR::is_a<ASR::BitCast_t>(*expr)) {
            return false;
        }
        ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(expr);
        ASR::ttype_t *target_type = bitcast->m_type;
        if (!is_len_one_character_array_type(target_type)) {
            return false;
        }
        ASR::expr_t *scalar_source = nullptr;
        if (bitcast->m_value
                && (ASR::is_a<ASR::ArrayItem_t>(*bitcast->m_value)
                    || !ASRUtils::is_array(ASRUtils::expr_type(bitcast->m_value)))) {
            scalar_source = bitcast->m_value;
        } else if (bitcast->m_source
                && (ASR::is_a<ASR::ArrayItem_t>(*bitcast->m_source)
                    || !ASRUtils::is_array(ASRUtils::expr_type(bitcast->m_source)))) {
            scalar_source = bitcast->m_source;
        }
        if (scalar_source == nullptr) {
            return false;
        }
        ASR::ttype_t *source_type = ASRUtils::expr_type(scalar_source);
        if (ASR::is_a<ASR::ArrayItem_t>(*scalar_source)) {
            ASR::ArrayItem_t *array_item = ASR::down_cast<ASR::ArrayItem_t>(scalar_source);
            source_type = ASRUtils::type_get_past_array(ASRUtils::expr_type(array_item->m_v));
        }
        if (!(ASRUtils::is_integer(*source_type)
                || ASRUtils::is_unsigned_integer(*source_type)
                || ASRUtils::is_real(*source_type)
                || ASRUtils::is_logical(*source_type))) {
            return false;
        }
        size_t nbytes = ASRUtils::get_fixed_size_of_array(target_type);
        std::string target_code = CUtils::get_c_type_code(target_type, true, false);
        std::string target_type_name = get_c_array_wrapper_type_name(target_type);
        std::string source_type_name = CUtils::get_c_type_from_ttype_t(source_type);
        std::string source_code = CUtils::get_c_type_code(source_type, false, false);
        self().visit_expr(*scalar_source);
        out_expr = c_utils_functions->get_bitcast_scalar_to_char_array(
            target_type_name, source_type_name, source_code, target_code, nbytes) + "(" + src + ")";
        return true;
    }

    bool normalize_optional_alloc_scalar_ref_actual(ASR::expr_t *call_arg,
            bool wants_raw_pointer_actual, std::string &arg_src) {
        if (!is_c || !wants_raw_pointer_actual) {
            return false;
        }
        ASR::expr_t *raw_arg = unwrap_c_lvalue_expr(call_arg);
        if (raw_arg && ASR::is_a<ASR::Var_t>(*raw_arg)) {
            ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(raw_arg)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                ASR::Variable_t *arg_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                if (is_scalar_allocatable_storage_type(arg_var->m_type)) {
                    arg_src = get_c_var_storage_name(arg_var);
                    return true;
                }
                if (is_scalar_pointer_storage_type(arg_var->m_type)) {
                    arg_src = CUtils::get_c_variable_name(*arg_var);
                    return true;
                }
            }
        }
        if (raw_arg && ASR::is_a<ASR::StructInstanceMember_t>(*raw_arg)) {
            ASR::StructInstanceMember_t *member_arg =
                ASR::down_cast<ASR::StructInstanceMember_t>(raw_arg);
            ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_arg->m_m);
            if (ASR::is_a<ASR::Variable_t>(*member_sym)) {
                ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
                if (is_scalar_allocatable_storage_type(member_var->m_type)
                        || is_scalar_pointer_storage_type(member_var->m_type)) {
                    arg_src = get_struct_instance_member_expr(*member_arg, false);
                    return true;
                }
            }
        }
        if (arg_src.size() > 3
                && arg_src.rfind("(*", 0) == 0
                && arg_src.back() == ')') {
            arg_src = arg_src.substr(2, arg_src.size() - 3);
            return true;
        }
        return false;
    }

    std::string canonicalize_raw_pointer_actual_src(const std::string &arg_src) {
        if (arg_src.size() > 3
                && arg_src.rfind("(*", 0) == 0
                && arg_src.back() == ')') {
            return arg_src.substr(2, arg_src.size() - 3);
        }
        return arg_src;
    }

    bool is_pointer_slot_dispatch_receiver(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::Variable_t *var = nullptr;
        if (ASR::is_a<ASR::Var_t>(*expr)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(expr)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                var = ASR::down_cast<ASR::Variable_t>(sym);
            }
        } else if (ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            ASR::StructInstanceMember_t *member =
                ASR::down_cast<ASR::StructInstanceMember_t>(expr);
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(member->m_m);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                var = ASR::down_cast<ASR::Variable_t>(sym);
            }
        }
        if (var == nullptr) {
            return false;
        }
        ASR::ttype_t *value_type =
            ASRUtils::type_get_past_allocatable_pointer(var->m_type);
        return ASRUtils::is_allocatable(var->m_type)
            || ASRUtils::is_pointer(var->m_type)
            || (value_type != nullptr && ASRUtils::is_class_type(value_type));
    }

    std::string get_deferred_dispatch_receiver_pointer_src(ASR::expr_t *raw_receiver,
            const std::string &receiver_src) {
        if (receiver_src.size() > 3
                && receiver_src.rfind("(*", 0) == 0
                && receiver_src.back() == ')'
                && is_pointer_slot_dispatch_receiver(raw_receiver)) {
            return receiver_src;
        }
        return canonicalize_raw_pointer_actual_src(receiver_src);
    }

    bool is_compiler_created_scalar_storage_temp(const std::string &arg_src) {
        return arg_src.rfind("(*__libasr_created_variable_", 0) == 0
            && arg_src.rfind("(*__libasr_created_variable_pointer_", 0) != 0
            && arg_src.back() == ')';
    }

    bool is_plain_pointer_backed_local_aggregate_actual(ASR::Variable_t *actual_var,
            bool pointer_backed_aggregate_actual) {
        if (!is_c || actual_var == nullptr || !pointer_backed_aggregate_actual) {
            return false;
        }
        return !ASRUtils::is_arg_dummy(actual_var->m_intent)
            && !ASRUtils::is_pointer(actual_var->m_type)
            && !ASRUtils::is_allocatable(actual_var->m_type)
            && is_struct_or_class_type(actual_var->m_type);
    }

    std::string get_addressable_call_arg_src(ASR::expr_t *shape_call_arg,
            const std::string &arg_src) {
        if (shape_call_arg == nullptr) {
            return arg_src;
        }
        bool needs_unwrapped_lvalue = arg_src.size() > 3
            && arg_src.rfind("((", 0) == 0;
        if (!needs_unwrapped_lvalue) {
            return arg_src;
        }
        if (!(ASR::is_a<ASR::Var_t>(*shape_call_arg)
                || ASR::is_a<ASR::ArrayItem_t>(*shape_call_arg)
                || ASR::is_a<ASR::StructInstanceMember_t>(*shape_call_arg)
                || ASR::is_a<ASR::UnionInstanceMember_t>(*shape_call_arg))) {
            return arg_src;
        }
        std::string saved_src = src;
        self().visit_expr(*shape_call_arg);
        std::string lvalue_src = src;
        src = saved_src;
        return lvalue_src;
    }

    std::string emit_move_alloc_source_reset(ASR::expr_t *source_expr,
            const std::string &indent) {
        if (!is_c || source_expr == nullptr) {
            return "";
        }
        ASR::expr_t *raw_source_expr = unwrap_c_lvalue_expr(source_expr);
        ASR::expr_t *shape_source_expr = raw_source_expr ? raw_source_expr : source_expr;
        ASR::ttype_t *source_type = ASRUtils::expr_type(source_expr);
        std::string saved_src = src;
        self().visit_expr(*source_expr);
        std::string source_src = src;
        src = saved_src;
        std::string source_lvalue = get_addressable_call_arg_src(
            shape_source_expr, source_src);
        bool reset_as_descriptor = ASRUtils::is_array(source_type);
        if (reset_as_descriptor) {
            headers.insert("string.h");
            return indent + "memset(" + source_lvalue + ", 0, sizeof(*"
                + source_lvalue + "));\n";
        }
        return indent + source_lvalue + " = NULL;\n";
    }

    bool is_scalar_pointer_storage_type(ASR::ttype_t *type) {
        if (type == nullptr || !ASRUtils::is_pointer(type) || ASRUtils::is_array(type)) {
            return false;
        }
        ASR::ttype_t *value_type = ASRUtils::type_get_past_pointer(type);
        return value_type != nullptr
            && !ASRUtils::is_aggregate_type(value_type)
            && !ASRUtils::is_character(*value_type)
            && !ASR::is_a<ASR::FunctionType_t>(*value_type);
    }

    bool is_scalar_allocatable_dummy_slot_type(ASR::Variable_t *var) {
        if (!is_c || var == nullptr || !ASRUtils::is_arg_dummy(var->m_intent)
                || ASRUtils::is_array(var->m_type)) {
            return false;
        }
        if (var->m_intent == ASRUtils::intent_in) {
            return false;
        }
        return is_scalar_allocatable_storage_type(var->m_type);
    }

    bool is_pointer_dummy_slot_type(const ASR::Variable_t *var) {
        if (!is_c || var == nullptr || !ASRUtils::is_arg_dummy(var->m_intent)
                || ASRUtils::is_array(var->m_type)
                || !ASRUtils::is_pointer(var->m_type)) {
            return false;
        }
        ASR::ttype_t *value_type = ASRUtils::type_get_past_pointer(var->m_type);
        return value_type != nullptr
            && !ASR::is_a<ASR::FunctionType_t>(*value_type);
    }

    bool is_procedure_pointer_dummy_slot_type(const ASR::Variable_t *var) {
        if (!is_c || var == nullptr || !ASRUtils::is_arg_dummy(var->m_intent)
                || ASRUtils::is_array(var->m_type)
                || !(var->m_intent == ASRUtils::intent_out
                    || var->m_intent == ASRUtils::intent_inout)
                || !ASRUtils::is_pointer(var->m_type)) {
            return false;
        }
        ASR::ttype_t *value_type = ASRUtils::type_get_past_pointer(var->m_type);
        return value_type != nullptr
            && ASR::is_a<ASR::FunctionType_t>(*value_type);
    }

    bool is_aggregate_dummy_slot_type(ASR::Variable_t *var) {
        if (!is_c || var == nullptr || !ASRUtils::is_arg_dummy(var->m_intent)
                || ASRUtils::is_array(var->m_type)) {
            return false;
        }
        if (var->m_intent == ASRUtils::intent_in) {
            return false;
        }
        ASR::ttype_t *value_type =
            ASRUtils::type_get_past_allocatable_pointer(var->m_type);
        if (value_type == nullptr || ASRUtils::is_character(*value_type)) {
            return false;
        }
        return ASRUtils::is_aggregate_type(value_type)
            && (ASRUtils::is_pointer(var->m_type) || ASRUtils::is_allocatable(var->m_type));
    }

    bool is_plain_aggregate_dummy_pointee_type(const ASR::Variable_t *var) {
        if (!is_c || var == nullptr || !ASRUtils::is_arg_dummy(var->m_intent)
                || ASRUtils::is_array(var->m_type)) {
            return false;
        }
        if (!(var->m_intent == ASRUtils::intent_out
                || var->m_intent == ASRUtils::intent_inout)) {
            return false;
        }
        if (ASRUtils::is_pointer(var->m_type) || ASRUtils::is_allocatable(var->m_type)) {
            return false;
        }
        if (ASR::is_a<ASR::CPtr_t>(*var->m_type)) {
            return false;
        }
        ASR::ttype_t *value_type =
            ASRUtils::type_get_past_allocatable_pointer(var->m_type);
        if (value_type == nullptr || ASRUtils::is_character(*value_type)) {
            return false;
        }
        return ASRUtils::is_aggregate_type(value_type);
    }

    bool emits_plain_aggregate_dummy_pointee_value(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        if (!ASR::is_a<ASR::Variable_t>(*sym)) {
            return false;
        }
        return is_plain_aggregate_dummy_pointee_type(
            ASR::down_cast<ASR::Variable_t>(sym));
    }

    bool is_unlimited_polymorphic_storage_type(ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_allocatable_pointer(type);
        if (type == nullptr) {
            return false;
        }
        if (ASRUtils::is_array(type)) {
            type = ASRUtils::type_get_past_array(type);
        }
        return type != nullptr
            && ASR::is_a<ASR::StructType_t>(*type)
            && ASR::down_cast<ASR::StructType_t>(type)->m_is_unlimited_polymorphic;
    }

    bool is_unlimited_polymorphic_dummy_slot_type(ASR::Variable_t *var) {
        if (!is_c || var == nullptr || !ASRUtils::is_arg_dummy(var->m_intent)) {
            return false;
        }
        if (!(var->m_intent == ASRUtils::intent_out
                || var->m_intent == ASRUtils::intent_inout)) {
            return false;
        }
        return is_unlimited_polymorphic_storage_type(var->m_type);
    }

    bool is_struct_or_class_type(ASR::ttype_t *type) {
        if (type == nullptr) {
            return false;
        }
        type = ASRUtils::type_get_past_allocatable_pointer(type);
        return type != nullptr
            && (ASR::is_a<ASR::StructType_t>(*type) || ASRUtils::is_class_type(type));
    }

    std::string cast_aggregate_pointer_actual_to_param_type(ASR::Variable_t *param,
            const std::string &ptr_expr) {
        if (!is_c || param == nullptr || param->m_type_declaration == nullptr) {
            return ptr_expr;
        }
        ASR::symbol_t *type_sym =
            ASRUtils::symbol_get_past_external(param->m_type_declaration);
        if (type_sym == nullptr || !ASR::is_a<ASR::Struct_t>(*type_sym)) {
            return ptr_expr;
        }
        return "((struct " + CUtils::get_c_symbol_name(type_sym) + "*)(" + ptr_expr + "))";
    }

    bool param_expects_raw_pointer_actual(ASR::Variable_t *param,
            ASR::ttype_t *param_type, ASR::ttype_t *param_type_unwrapped) {
        if (!is_c || param == nullptr || param_type == nullptr) {
            return false;
        }
        if (get_c_array_wrapper_base_type(param_type) != nullptr) {
            return false;
        }
        if (is_pointer_dummy_slot_type(param)) {
            return false;
        }
        bool optional_alloc_scalar_ref = param->m_presence == ASR::presenceType::Optional
            && ASR::is_a<ASR::Allocatable_t>(*param_type)
            && param_type_unwrapped
            && !ASRUtils::is_array(param_type)
            && !ASRUtils::is_aggregate_type(param_type)
            && !ASRUtils::is_character(*param_type_unwrapped);
        return (ASRUtils::is_pointer(param_type) && !ASRUtils::is_array(param_type))
            || optional_alloc_scalar_ref
            || is_aggregate_dummy_slot_type(param);
    }

    bool is_scalar_character_pointer_type(ASR::ttype_t *type) {
        if (!is_c || type == nullptr || !ASRUtils::is_pointer(type)
                || ASRUtils::is_array(type)) {
            return false;
        }
        ASR::ttype_t *pointee_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::type_get_past_pointer(type));
        return pointee_type != nullptr
            && ASRUtils::is_character(*pointee_type)
            && !ASRUtils::is_array(pointee_type);
    }

    bool is_scalar_character_dummy_type(ASR::ttype_t *type, ASR::ttype_t *type_unwrapped) {
        return is_c && type != nullptr && type_unwrapped != nullptr
            && !ASRUtils::is_array(type)
            && ASRUtils::is_character(*type_unwrapped);
    }

    bool is_scalar_base_array_type(ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_allocatable_pointer(type);
        if (!ASRUtils::is_array(type)) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(type);
        return !ASRUtils::is_array(element_type)
            && !ASRUtils::is_aggregate_type(element_type)
            && !ASR::is_a<ASR::FunctionType_t>(*element_type);
    }

    ASR::ttype_t *get_inline_struct_member_array_storage_type(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            return nullptr;
        }
        ASR::StructInstanceMember_t *member = ASR::down_cast<ASR::StructInstanceMember_t>(expr);
        ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member->m_m);
        ASR::ttype_t *member_type =
            ASRUtils::type_get_past_allocatable_pointer(ASRUtils::symbol_type(member_sym));
        if (member_type == nullptr || !ASRUtils::is_array(member_type)
                || !ASRUtils::is_fixed_size_array(member_type)) {
            return nullptr;
        }
        if (!ASR::is_a<ASR::Variable_t>(*member_sym)) {
            return nullptr;
        }
        ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
        ASR::asr_t *owner = member_var->m_parent_symtab
            ? member_var->m_parent_symtab->asr_owner : nullptr;
        if (owner == nullptr || !ASR::is_a<ASR::Struct_t>(*owner)) {
            return nullptr;
        }
        return member_type;
    }

    bool is_fixed_size_array_storage_expr(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            return false;
        }
        ASR::ttype_t *expr_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        return expr_type != nullptr
            && ASRUtils::is_array(expr_type)
            && ASRUtils::is_fixed_size_array(expr_type);
    }

    bool is_data_only_array_section_expr(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return false;
        }
        ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
        return is_data_only_array_expr(section->m_v)
            || is_fixed_size_array_storage_expr(section->m_v);
    }

    bool is_data_only_array_expr(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        if (ASR::is_a<ASR::ArrayConstant_t>(*expr)) {
            return true;
        }
        if (get_inline_struct_member_array_storage_type(expr) != nullptr) {
            return true;
        }
        ASR::ttype_t *expr_type = ASRUtils::expr_type(expr);
        if (!ASRUtils::is_array(expr_type)) {
            return false;
        }
        ASR::dimension_t *dims = nullptr;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(expr_type, dims);
        if (!ASRUtils::is_fixed_size_array(dims, n_dims)
                && !ASRUtils::is_simd_array(expr)) {
            return false;
        }
        if (ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            ASR::StructInstanceMember_t *member =
                ASR::down_cast<ASR::StructInstanceMember_t>(expr);
            ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member->m_m);
            if (ASR::is_a<ASR::Variable_t>(*member_sym)) {
                ASR::asr_t *owner = ASR::down_cast<ASR::Variable_t>(member_sym)->m_parent_symtab
                    ? ASR::down_cast<ASR::Variable_t>(member_sym)->m_parent_symtab->asr_owner
                    : nullptr;
                return owner != nullptr && ASR::is_a<ASR::Struct_t>(*owner);
            }
        }
        ASR::symbol_t *owner = ASRUtils::get_asr_owner(expr);
        return owner != nullptr && ASR::is_a<ASR::Struct_t>(*owner);
    }

    bool is_c_array_valued_expr(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::expr_t *unwrapped_expr = ASRUtils::expr_value(expr);
        if (unwrapped_expr == nullptr) {
            unwrapped_expr = expr;
        }
        return ASRUtils::is_array(
                   ASRUtils::type_get_past_allocatable_pointer(
                       ASRUtils::expr_type(expr)))
            || ASR::is_a<ASR::ArrayConstant_t>(*unwrapped_expr)
            || ASR::is_a<ASR::ArrayConstructor_t>(*unwrapped_expr)
            || ASR::is_a<ASR::ArrayReshape_t>(*unwrapped_expr)
            || ASR::is_a<ASR::ArrayBroadcast_t>(*unwrapped_expr);
    }

    std::string get_c_array_data_expr(ASR::expr_t *expr, const std::string &expr_src) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return expr_src + "->data";
        }
        ASR::expr_t *unwrapped_expr = ASRUtils::expr_value(expr);
        if (unwrapped_expr == nullptr) {
            unwrapped_expr = expr;
        }
        if (ASR::is_a<ASR::ArrayConstant_t>(*unwrapped_expr)) {
            return expr_src + "->data";
        }
        if (is_data_only_array_expr(unwrapped_expr)
                || is_fixed_size_array_storage_expr(unwrapped_expr)) {
            return expr_src;
        }
        return expr_src + "->data";
    }

    std::string get_c_array_data_pointer_expr(ASR::expr_t *expr,
            const std::string &expr_src) {
        std::string data = get_c_array_data_expr(expr, expr_src);
        std::string offset = get_c_array_offset_expr(expr, expr_src);
        if (offset == "0") {
            return data;
        }
        return "(" + data + " + " + offset + ")";
    }

    std::string get_c_array_offset_expr(ASR::expr_t *expr, const std::string &expr_src) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return expr_src + "->offset";
        }
        ASR::expr_t *unwrapped_expr = ASRUtils::expr_value(expr);
        if (unwrapped_expr == nullptr) {
            unwrapped_expr = expr;
        }
        if (ASR::is_a<ASR::ArrayConstant_t>(*unwrapped_expr)) {
            return expr_src + "->offset";
        }
        if (is_data_only_array_expr(unwrapped_expr)
                || is_fixed_size_array_storage_expr(unwrapped_expr)) {
            return "0";
        }
        return expr_src + "->offset";
    }

    std::string get_c_array_stride_expr(ASR::expr_t *expr, const std::string &expr_src) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return expr_src + "->dims[0].stride";
        }
        ASR::expr_t *unwrapped_expr = ASRUtils::expr_value(expr);
        if (unwrapped_expr == nullptr) {
            unwrapped_expr = expr;
        }
        if (ASR::is_a<ASR::ArrayConstant_t>(*unwrapped_expr)
                || (!is_data_only_array_expr(unwrapped_expr)
                    && !is_fixed_size_array_storage_expr(unwrapped_expr))) {
            return expr_src + "->dims[0].stride";
        }
        return "1";
    }

    std::string get_c_array_length_expr(ASR::expr_t *expr, const std::string &expr_src) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr) {
            return expr_src + "->dims[0].length";
        }
        ASR::expr_t *unwrapped_expr = ASRUtils::expr_value(expr);
        if (unwrapped_expr == nullptr) {
            unwrapped_expr = expr;
        }
        if (ASR::is_a<ASR::ArrayConstant_t>(*unwrapped_expr)
                || (!is_data_only_array_expr(unwrapped_expr)
                    && !is_fixed_size_array_storage_expr(unwrapped_expr))) {
            return expr_src + "->dims[0].length";
        }
        return std::to_string(
            ASRUtils::get_fixed_size_of_array(ASRUtils::expr_type(unwrapped_expr)));
    }

    std::string get_c_scalar_index_expr(ASR::expr_t *expr) {
        self().visit_expr(*expr);
        std::string idx_expr = src;
        if (!ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return idx_expr;
        }
        return get_c_array_data_expr(expr, idx_expr) + "["
            + get_c_array_offset_expr(expr, idx_expr) + "]";
    }

    bool is_vector_subscript_expr(ASR::expr_t *expr) {
        if (expr == nullptr) {
            return false;
        }
        ASR::expr_t *value = ASRUtils::expr_value(expr);
        if (value == nullptr) {
            value = expr;
        }
        return ASRUtils::is_array(ASRUtils::expr_type(expr))
            || ASR::is_a<ASR::ArrayConstant_t>(*value)
            || ASR::is_a<ASR::ArrayConstructor_t>(*value)
            || ASR::is_a<ASR::ArrayReshape_t>(*value)
            || ASR::is_a<ASR::ArrayBroadcast_t>(*value);
    }

    int get_vector_subscript_dim(const ASR::ArrayItem_t &target_item) {
        for (size_t i = 0; i < target_item.n_args; i++) {
            ASR::expr_t *idx_expr = get_array_index_expr(target_item.m_args[i]);
            if (is_vector_subscript_expr(idx_expr)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    bool is_vector_subscript_scalar_array_item(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::ArrayItem_t>(*expr)) {
            return false;
        }
        ASR::ArrayItem_t *target_item = ASR::down_cast<ASR::ArrayItem_t>(expr);
        ASR::ttype_t *target_expr_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (!ASRUtils::is_array(target_expr_type)) {
            return false;
        }
        ASR::ttype_t *base_array_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(target_item->m_v));
        if (base_array_type == nullptr || !ASRUtils::is_array(base_array_type)
                || !is_c_scalarizable_element_type(ASRUtils::expr_type(expr))) {
            return false;
        }
        return get_vector_subscript_dim(*target_item) >= 0;
    }

    bool try_emit_vector_subscript_scalar_array_assignment(
            const ASR::Assignment_t &x,
            ASR::expr_t *target_expr) {
        if (!is_c || !is_vector_subscript_scalar_array_item(target_expr)) {
            return false;
        }
        ASR::ArrayItem_t *target_item = ASR::down_cast<ASR::ArrayItem_t>(target_expr);
        if (!is_c_array_valued_expr(x.m_value)) {
            return false;
        }
        int vector_dim = get_vector_subscript_dim(*target_item);
        if (vector_dim < 0) {
            return false;
        }
        if (is_c_scalarizable_array_expr(x.m_value)
                && !c_expr_references_root_symbol(x.m_value,
                    get_c_array_assignment_root_symbol(target_expr))) {
            std::string setup;
            std::string loop_var = get_unique_local_name("__lfortran_i");
            std::string target_value;
            std::string target_length;
            if (get_c_rank1_array_element_expr(target_expr, "__lfortran_lhs",
                        loop_var, setup, target_value, target_length)) {
                std::string value_expr;
                if (get_c_scalarized_array_expr(x.m_value, loop_var, setup, value_expr)) {
                    std::string indent(indentation_level * indentation_spaces, ' ');
                    src = check_tmp_buffer();
                    src += indent + "{\n";
                    indentation_level++;
                    std::string inner_indent(indentation_level * indentation_spaces, ' ');
                    src += setup;
                    src += inner_indent + "for (int64_t " + loop_var + " = 0; "
                        + loop_var + " < " + target_length + "; " + loop_var + "++) {\n";
                    indentation_level++;
                    std::string loop_indent(indentation_level * indentation_spaces, ' ');
                    src += loop_indent + target_value + " = " + value_expr + ";\n";
                    indentation_level--;
                    src += inner_indent + "}\n";
                    indentation_level--;
                    src += indent + "}\n";
                    from_std_vector_helper.clear();
                    return true;
                }
            }
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        self().visit_expr(*target_item->m_v);
        std::string base_array = src;
        self().visit_expr(*x.m_value);
        std::string value_src = src;

        ASR::expr_t *vector_expr = get_array_index_expr(target_item->m_args[vector_dim]);
        LCOMPILERS_ASSERT(vector_expr != nullptr);
        self().visit_expr(*vector_expr);
        std::string vector_src = src;

        std::string vec_data = get_c_array_data_expr(vector_expr, vector_src);
        std::string vec_offset = get_c_array_offset_expr(vector_expr, vector_src);
        std::string vec_stride = get_c_array_stride_expr(vector_expr, vector_src);
        std::string vec_length = get_c_array_length_expr(vector_expr, vector_src);
        std::string val_data = get_c_array_data_expr(x.m_value, value_src);
        std::string val_offset = get_c_array_offset_expr(x.m_value, value_src);
        std::string val_stride = get_c_array_stride_expr(x.m_value, value_src);
        std::string loop_var = get_unique_local_name("vec_assign_i");

        std::vector<std::string> indices;
        indices.reserve(target_item->n_args);
        for (size_t i = 0; i < target_item->n_args; i++) {
            if (static_cast<int>(i) == vector_dim) {
                indices.push_back(vec_data + "[" + vec_offset + " + (" + loop_var
                    + " * " + vec_stride + ")]");
            } else {
                ASR::expr_t *idx_expr = get_array_index_expr(target_item->m_args[i]);
                LCOMPILERS_ASSERT(idx_expr != nullptr);
                indices.push_back(get_c_scalar_index_expr(idx_expr));
            }
        }

        std::string target_index = cmo_convertor_single_element(
            base_array, indices, static_cast<int>(target_item->n_args), false);
        src = check_tmp_buffer();
        src += indent + "for (int32_t " + loop_var + " = 0; " + loop_var + " < "
            + vec_length + "; " + loop_var + "++) {\n";
        src += indent + std::string(indentation_spaces, ' ')
            + base_array + "->data[" + target_index + "] = "
            + val_data + "[" + val_offset + " + (" + loop_var
            + " * " + val_stride + ")];\n";
        src += indent + "}\n";
        from_std_vector_helper.clear();
        return true;
    }

    bool is_addressable_fortran_external_actual(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        return expr != nullptr
            && (ASR::is_a<ASR::Var_t>(*expr)
                || ASR::is_a<ASR::ArrayItem_t>(*expr)
                || ASR::is_a<ASR::StructInstanceMember_t>(*expr)
                || ASR::is_a<ASR::UnionInstanceMember_t>(*expr));
    }

    std::string get_fortran_external_scalar_actual(
            ASR::Variable_t *param, ASR::expr_t *call_arg,
            ASR::expr_t *shape_call_arg, const std::string &arg_src) {
        ASR::ttype_t *param_type = param ? param->m_type : ASRUtils::expr_type(call_arg);
        ASR::ttype_t *param_type_unwrapped =
            ASRUtils::type_get_past_allocatable_pointer(param_type);
        if (param_type_unwrapped != nullptr
                && ASRUtils::is_character(*param_type_unwrapped)) {
            return arg_src;
        }
        if (is_addressable_fortran_external_actual(shape_call_arg)) {
            return "&" + get_addressable_call_arg_src(shape_call_arg, arg_src);
        }
        std::string type_name = get_fortran_external_value_type(param_type);
        return "&((" + type_name + "){" + arg_src + "})";
    }

    std::string construct_fortran_external_call_args(ASR::Function_t* f,
            size_t start_idx, size_t n_args, ASR::call_arg_t* m_args) {
        bracket_open++;
        std::string args;
        for (size_t i = start_idx; i < n_args; i++) {
            ASR::expr_t *call_arg = m_args[i].m_value;
            ASR::expr_t *raw_call_arg = unwrap_c_lvalue_expr(call_arg);
            ASR::expr_t *shape_call_arg = raw_call_arg ? raw_call_arg : call_arg;
            visit_expr_without_c_pow_cache(*call_arg);
            std::string arg_src = src;
            std::string arg_setup = drain_tmp_buffer();
            arg_setup += extract_stmt_setup_from_expr(arg_src);
            if (!arg_setup.empty()) {
                tmp_buffer_src.push_back(arg_setup);
            }
            ASR::Variable_t *param = i < f->n_args ? ASRUtils::EXPR2VAR(f->m_args[i]) : nullptr;
            ASR::ttype_t *param_type = param ? param->m_type : ASRUtils::expr_type(call_arg);
            if (param_type != nullptr && ASRUtils::is_array(param_type)) {
                args += get_c_array_data_pointer_expr(call_arg, arg_src);
            } else {
                args += get_fortran_external_scalar_actual(
                    param, call_arg, shape_call_arg, arg_src);
            }
            if (i < n_args - 1) {
                args += ", ";
            }
        }
        bracket_open--;
        return args;
    }

    std::string construct_call_args(ASR::Function_t* f, size_t n_args,
            ASR::call_arg_t* m_args, bool force_generated_abi=false,
            bool allow_no_copy_descriptor_views=true,
            bool use_named_no_copy_descriptor_views=true) {
        if (!force_generated_abi && f != nullptr
                && is_fortran_external_interface_function(*f)) {
            return construct_fortran_external_call_args(f, 0, n_args, m_args);
        }
        bracket_open++;
        bool saved_reuse_array_compare_temps_in_call_args =
            reuse_array_compare_temps_in_call_args;
        auto saved_array_compare_temp_cache = std::move(array_compare_temp_cache);
        reuse_array_compare_temps_in_call_args = true;
        array_compare_temp_cache.clear();
        bool is_count_callee = is_c && f != nullptr
            && std::string(f->m_name).find("count") != std::string::npos;
        std::string count_mask_arg_src;
        std::string args = "";
        std::string call_arg_setup;
        std::string no_copy_hidden_base_name;
        std::string no_copy_hidden_shape_src;
        int no_copy_hidden_rank = 0;
        int no_copy_hidden_dim = 0;
        size_t override_arg_index = static_cast<size_t>(-1);
        std::string override_arg_value;
        for (size_t i=0; i<n_args; i++) {
            if (i == override_arg_index) {
                args += override_arg_value;
                if (i < n_args - 1) args += ", ";
                continue;
            }
            ASR::expr_t* call_arg = m_args[i].m_value;
            ASR::Variable_t *param = i < f->n_args ? ASRUtils::EXPR2VAR(f->m_args[i]) : nullptr;
            if (!no_copy_hidden_shape_src.empty() && no_copy_hidden_dim < no_copy_hidden_rank
                    && param != nullptr
                    && is_pass_array_by_data_hidden_arg_name(
                        std::string(param->m_name), no_copy_hidden_base_name)) {
                args += "((int32_t) " + no_copy_hidden_shape_src + "->dims["
                    + std::to_string(no_copy_hidden_dim) + "].length)";
                no_copy_hidden_dim++;
                if (i < n_args - 1) args += ", ";
                continue;
            }
            if (is_count_callee && i == 1 && !count_mask_arg_src.empty()
                    && (ASR::is_a<ASR::ArrayBound_t>(*call_arg)
                        || ASR::is_a<ASR::ArraySize_t>(*call_arg))) {
                args += "((int32_t) " + count_mask_arg_src + "->dims[0].length + "
                    + count_mask_arg_src + "->dims[0].lower_bound - 1)";
                if (i < n_args - 1) args += ", ";
                continue;
            }
            ASR::expr_t* raw_call_arg = unwrap_c_lvalue_expr(call_arg);
            ASR::expr_t* shape_call_arg = raw_call_arg ? raw_call_arg : call_arg;
            auto address_of_src = [&](const std::string &current_src) -> std::string {
                return "&" + get_addressable_call_arg_src(shape_call_arg, current_src);
            };
            ASR::ttype_t* type = ASRUtils::expr_type(call_arg);
            ASR::ttype_t *type_unwrapped = ASRUtils::type_get_past_allocatable_pointer(type);
            ASR::ttype_t *param_type = i < f->n_args ? ASRUtils::expr_type(f->m_args[i]) : nullptr;
            ASR::Variable_t *actual_var = nullptr;
            ASR::ttype_t *param_type_unwrapped = param_type ?
                ASRUtils::type_get_past_allocatable_pointer(param_type) : nullptr;
            bool raw_pointer_actual = false;
            bool pointer_dummy_slot_actual = false;
            bool aggregate_dummy_slot_actual = false;
            bool scalar_alloc_dummy_slot_actual = false;
            bool param_is_optional_alloc_scalar_ref = is_c && param
                && param->m_presence == ASR::presenceType::Optional
                && param_type
                && ASR::is_a<ASR::Allocatable_t>(*param_type)
                && param_type_unwrapped
                && !ASRUtils::is_array(param_type)
                && !ASRUtils::is_aggregate_type(param_type)
                && !ASRUtils::is_character(*param_type_unwrapped);
            bool wants_raw_pointer_actual = param_expects_raw_pointer_actual(
                param, param_type, param_type_unwrapped);
            bool wants_pointer_dummy_slot_actual = is_pointer_dummy_slot_type(param);
            bool wants_aggregate_dummy_slot_actual = is_aggregate_dummy_slot_type(param);
            bool wants_unlimited_polymorphic_dummy_slot_actual =
                is_unlimited_polymorphic_dummy_slot_type(param);
            bool wants_scalar_alloc_dummy_slot_actual =
                is_scalar_allocatable_dummy_slot_type(param);
            bool wants_aggregate_pointer_actual = is_c && i < f->n_args
                && is_struct_or_class_type(param_type_unwrapped)
                && is_struct_or_class_type(type_unwrapped)
                && !wants_pointer_dummy_slot_actual
                && !wants_aggregate_dummy_slot_actual;
            std::string arg_src, arg_setup;
            if (!try_get_c_scalar_string_call_arg_view(
                    call_arg, param, param_type, arg_src, arg_setup)) {
                bool saved_force_storage_expr_in_call_args = force_storage_expr_in_call_args;
                force_storage_expr_in_call_args = is_c && wants_raw_pointer_actual;
                self().visit_expr(*call_arg);
                arg_src = src;
                arg_setup = drain_tmp_buffer();
                arg_setup += extract_stmt_setup_from_expr(arg_src);
                force_storage_expr_in_call_args = saved_force_storage_expr_in_call_args;
            }
            src = arg_src;
            if (is_c && raw_call_arg && ASR::is_a<ASR::Var_t>(*raw_call_arg)) {
                ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(raw_call_arg)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                    actual_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                }
            }
            bool param_defines_actual = param
                && (param->m_intent == ASRUtils::intent_out
                    || param->m_intent == ASRUtils::intent_inout
                    || (std::string(param->m_name) == "result"
                        && f != nullptr
                        && std::string(f->m_name).find("lcompilers_matmul")
                            != std::string::npos));
            if (is_c && param_defines_actual) {
                std::string lazy_setup =
                    self().emit_c_lazy_automatic_array_temp_allocation(
                        call_arg, arg_src);
                if (!lazy_setup.empty()) {
                    call_arg_setup += arg_setup;
                    arg_setup.clear();
                    call_arg_setup += lazy_setup;
                }
            }
            bool actual_is_raw_c_char_array_before_view =
                is_raw_c_char_array_dummy(actual_var);
            bool no_copy_descriptor_view_actual = allow_no_copy_descriptor_views
                && !actual_is_raw_c_char_array_before_view
                && try_build_c_array_no_copy_descriptor_view_arg(
                        call_arg, param_type,
                        param ? param->m_type_declaration : nullptr, arg_src,
                        use_named_no_copy_descriptor_views);
            if (no_copy_descriptor_view_actual) {
                call_arg_setup += drain_tmp_buffer();
                if (param != nullptr) {
                    no_copy_hidden_base_name = param ? std::string(param->m_name) : "";
                    no_copy_hidden_shape_src = get_c_descriptor_member_base_expr(arg_src);
                    no_copy_hidden_rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::type_get_past_allocatable_pointer(param_type));
                    no_copy_hidden_dim = 0;
                }
            }
            if (!no_copy_descriptor_view_actual && !arg_setup.empty()) {
                tmp_buffer_src.push_back(arg_setup);
            }
            if (is_c && is_compiler_created_scalar_storage_temp(arg_src)) {
                src = canonicalize_raw_pointer_actual_src(arg_src);
                arg_src = src;
            }
            bool pointer_backed_aggregate_actual = is_c
                && is_struct_or_class_type(type_unwrapped)
                && is_pointer_backed_struct_expr(call_arg);
            bool actual_emits_aggregate_value = wants_aggregate_pointer_actual
                && !pointer_backed_aggregate_actual;
            if (is_c && raw_call_arg && ASR::is_a<ASR::Var_t>(*raw_call_arg) && i < f->n_args) {
                ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(raw_call_arg)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                    ASR::Variable_t *arg_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                    actual_var = arg_var;
                    aggregate_dummy_slot_actual =
                        is_aggregate_dummy_slot_type(arg_var);
                    bool arg_is_scalar_alloc_storage =
                        is_scalar_allocatable_storage_type(arg_var->m_type);
                    bool arg_is_scalar_pointer_storage =
                        is_scalar_pointer_storage_type(arg_var->m_type);
                    if (wants_pointer_dummy_slot_actual
                            && ASRUtils::is_pointer(type)
                            && !ASRUtils::is_array(type)) {
                        src = CUtils::get_c_variable_name(*arg_var);
                        pointer_dummy_slot_actual = is_pointer_dummy_slot_type(arg_var);
                    } else if (wants_scalar_alloc_dummy_slot_actual && arg_is_scalar_alloc_storage) {
                        src = get_c_var_storage_name(arg_var);
                        scalar_alloc_dummy_slot_actual =
                            is_scalar_allocatable_dummy_slot_type(arg_var);
                    } else if (wants_raw_pointer_actual
                            && (arg_is_scalar_alloc_storage
                                || arg_is_scalar_pointer_storage)) {
                        src = arg_is_scalar_alloc_storage
                            ? get_c_var_storage_name(arg_var)
                            : CUtils::get_c_variable_name(*arg_var);
                        raw_pointer_actual = true;
                    } else if (param_is_optional_alloc_scalar_ref
                            && ASRUtils::is_pointer(type)
                            && !ASRUtils::is_array(type)) {
                        src = CUtils::get_c_variable_name(*arg_var);
                        raw_pointer_actual = true;
                    }
                }
                if (!wants_pointer_dummy_slot_actual
                        && ASRUtils::is_pointer(param_type)
                        && !ASRUtils::is_array(param_type)
                        && ASRUtils::is_pointer(type)
                        && !ASRUtils::is_array(type)) {
                    if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                        src = CUtils::get_c_variable_name(
                            *ASR::down_cast<ASR::Variable_t>(arg_sym));
                        raw_pointer_actual = true;
                    }
                }
            }
            if (normalize_optional_alloc_scalar_ref_actual(call_arg,
                    wants_raw_pointer_actual, src)) {
                raw_pointer_actual = true;
            }
            bool plain_pointer_backed_local_aggregate_actual =
                is_plain_pointer_backed_local_aggregate_actual(
                    actual_var, pointer_backed_aggregate_actual);
            if (is_c && param != nullptr
                    && param->m_intent == ASRUtils::intent_out
                    && actual_var != nullptr) {
                register_c_intent_out_local_struct_descriptor_cleanup(*actual_var);
            }
            if (!raw_pointer_actual
                    && param_is_optional_alloc_scalar_ref
                    && ASRUtils::is_pointer(type)
                    && !ASRUtils::is_array(type)) {
                src = canonicalize_raw_pointer_actual_src(arg_src);
                raw_pointer_actual = true;
            }
            if (raw_pointer_actual || wants_raw_pointer_actual) {
                src = canonicalize_raw_pointer_actual_src(src);
                arg_src = src;
            }
            if (no_copy_descriptor_view_actual) {
                src = arg_src;
            }
            ASR::expr_t *array_like_arg = call_arg;
            if (ASR::is_a<ASR::Cast_t>(*array_like_arg)) {
                ASR::Cast_t *cast_arg = ASR::down_cast<ASR::Cast_t>(array_like_arg);
                if (cast_arg->m_kind == ASR::cast_kindType::StringToArray) {
                    array_like_arg = cast_arg->m_arg;
                }
            }
            bool param_is_c_array_wrapper = get_c_array_wrapper_base_type(param_type) != nullptr;
            bool param_is_raw_c_char_array = is_raw_c_char_array_dummy(param);
            bool actual_is_raw_c_char_array = is_raw_c_char_array_dummy(actual_var);
            bool scalar_string_to_cchar = is_c
                && param_type && CUtils::is_len_one_character_array_type(param_type)
                && ASR::is_a<ASR::StringPhysicalCast_t>(*array_like_arg);
            if (is_c && param_type && CUtils::is_len_one_character_array_type(param_type)
                    && (actual_is_raw_c_char_array
                        || (type && CUtils::is_len_one_character_array_type(type))
                        || scalar_string_to_cchar)) {
                if (ASR::is_a<ASR::StringPhysicalCast_t>(*array_like_arg)) {
                    array_like_arg = ASR::down_cast<ASR::StringPhysicalCast_t>(array_like_arg)->m_arg;
                }
                if (!param_is_raw_c_char_array
                        && (actual_is_raw_c_char_array || scalar_string_to_cchar
                            || !ASRUtils::is_array(ASRUtils::expr_type(array_like_arg)))) {
                    std::string actual_len = "strlen(" + arg_src + ")";
                    ASR::Variable_t *next_param = (i + 1 < f->n_args)
                        ? ASRUtils::EXPR2VAR(f->m_args[i + 1]) : nullptr;
                    bool next_is_hidden_len = is_hidden_char_length_param(next_param);
                    size_t len_arg_index = i + 1;
                    if (next_is_hidden_len && i + 2 < n_args) {
                        len_arg_index = i + 2;
                    }
                    if (len_arg_index < n_args && m_args[len_arg_index].m_value) {
                        ASR::ttype_t *len_arg_type = ASRUtils::expr_type(m_args[len_arg_index].m_value);
                        len_arg_type = ASRUtils::type_get_past_allocatable_pointer(len_arg_type);
                        if (len_arg_type
                                && (ASRUtils::is_integer(*len_arg_type)
                                    || ASRUtils::is_unsigned_integer(*len_arg_type))) {
                            self().visit_expr(*m_args[len_arg_index].m_value);
                            actual_len = src;
                        }
                    }
                    if (next_is_hidden_len && i + 1 < n_args) {
                        override_arg_index = i + 1;
                        override_arg_value = actual_len;
                    }
                    std::string wrapper_type = get_c_array_wrapper_type_name(
                        param_type, param ? param->m_type_declaration : nullptr);
                    std::string cchar_len = get_unique_local_name("__lfortran_cchar_len");
                    std::string cchar_data = get_unique_local_name("__lfortran_cchar_data");
                    std::string cchar_i = get_unique_local_name("__lfortran_cchar_i");
                    std::string indent(indentation_level * indentation_spaces, ' ');
                    call_arg_setup += indent + "int64_t " + cchar_len + " = "
                        + actual_len + ";\n";
                    call_arg_setup += indent + "char *" + cchar_data + "[("
                        + cchar_len + " > 0) ? " + cchar_len + " : 1];\n";
                    call_arg_setup += indent + "for (int64_t " + cchar_i + " = 0; "
                        + cchar_i + " < " + cchar_len + "; " + cchar_i + "++) {\n";
                    call_arg_setup += indent + std::string(indentation_spaces, ' ')
                        + cchar_data + "[" + cchar_i + "] = " + arg_src + " + "
                        + cchar_i + ";\n";
                    call_arg_setup += indent + "}\n";
                    args += "(&(" + wrapper_type + "){ .data = " + cchar_data
                        + ", .dims = {{1, " + cchar_len + ", 1}}, .n_dims = 1, .offset = 0, .is_allocated = false })";
                } else if (param_is_raw_c_char_array
                        && (scalar_string_to_cchar || !ASRUtils::is_array(ASRUtils::expr_type(array_like_arg)))) {
                    args += arg_src;
                } else if (param_is_raw_c_char_array && !is_data_only_array_expr(call_arg)) {
                    args += build_c_char_array_descriptor_to_raw_arg(
                        arg_src, call_arg_setup);
                } else if (param_is_raw_c_char_array && is_data_only_array_expr(call_arg)) {
                    args += arg_src;
                } else {
                    args += param_is_raw_c_char_array
                        ? arg_src + "->data"
                        : cast_c_array_wrapper_ptr_to_target_type(
                            param_type, type, arg_src,
                            param ? param->m_type_declaration : nullptr,
                            actual_var ? actual_var->m_type_declaration : nullptr);
                }
                if (i < n_args - 1) args += ", ";
                continue;
            }
            auto remember_array_wrapper_hidden_shape =
                    [&](const std::string &wrapper_arg) {
                if (param != nullptr && param_is_c_array_wrapper) {
                    no_copy_hidden_base_name = std::string(param->m_name);
                    no_copy_hidden_shape_src =
                        get_c_descriptor_member_base_expr(wrapper_arg);
                    no_copy_hidden_rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::type_get_past_allocatable_pointer(param_type));
                    no_copy_hidden_dim = 0;
                }
            };
            auto pass_wrapper_arg = [&](const std::string &value_expr) -> std::string {
                std::map<std::string, std::string> dim_expr_overrides;
                std::string saved_src = src;
                for (size_t j = 0; j < n_args && j < f->n_args; j++) {
                    if (j == i || m_args[j].m_value == nullptr) {
                        continue;
                    }
                    ASR::Variable_t *dim_param = ASRUtils::EXPR2VAR(f->m_args[j]);
                    if (!dim_param || ASRUtils::is_array(dim_param->m_type)) {
                        continue;
                    }
                    self().visit_expr(*m_args[j].m_value);
                    dim_expr_overrides[CUtils::get_c_variable_name(*dim_param)] = src;
                }
                src = saved_src;
                if (is_c && ASR::is_a<ASR::ArrayPhysicalCast_t>(*call_arg)) {
                    ASR::ArrayPhysicalCast_t *cast =
                        ASR::down_cast<ASR::ArrayPhysicalCast_t>(call_arg);
                    ASR::ttype_t *target_array_type =
                        ASRUtils::type_get_past_allocatable_pointer(param_type);
                    ASR::ttype_t *source_array_type =
                        ASRUtils::type_get_past_allocatable_pointer(
                            ASRUtils::expr_type(cast->m_arg));
                    int target_rank = target_array_type
                        ? ASRUtils::extract_n_dims_from_ttype(target_array_type) : 0;
                    int source_rank = source_array_type
                        ? ASRUtils::extract_n_dims_from_ttype(source_array_type) : 0;
                    if (target_rank > 0 && source_rank > 0
                            && target_rank > source_rank) {
                        std::string wrapper_arg = build_c_array_wrapper_from_cast_target(
                            param_type, cast->m_arg, value_expr,
                            &dim_expr_overrides);
                        remember_array_wrapper_hidden_shape(wrapper_arg);
                        return wrapper_arg;
                    }
                    remember_array_wrapper_hidden_shape(value_expr);
                    return value_expr;
                }
                std::string wrapper_arg = build_c_array_wrapper_from_cast_target(
                    param_type, call_arg, value_expr, &dim_expr_overrides);
                remember_array_wrapper_hidden_shape(wrapper_arg);
                return wrapper_arg;
            };
            bool pass_data_only_array_wrapper = is_c && param_is_c_array_wrapper
                && (is_data_only_array_expr(call_arg)
                    || is_fixed_size_array_storage_expr(call_arg)
                    || is_data_only_array_section_expr(call_arg));
            if (pass_data_only_array_wrapper) {
                std::string wrapper_arg = pass_wrapper_arg(src);
                args += wrapper_arg;
                if (param != nullptr) {
                    no_copy_hidden_base_name = std::string(param->m_name);
                    no_copy_hidden_shape_src =
                        get_c_descriptor_member_base_expr(wrapper_arg);
                    no_copy_hidden_rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::type_get_past_allocatable_pointer(param_type));
                    no_copy_hidden_dim = 0;
                }
                if (is_count_callee && i == 0) {
                    count_mask_arg_src = src;
                }
                if (i < n_args - 1) args += ", ";
                continue;
            }
            if (shape_call_arg && ASR::is_a<ASR::Var_t>(*shape_call_arg)
                && ASR::is_a<ASR::Variable_t>(
                    *ASRUtils::symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(shape_call_arg)->m_v))) {
                if (wants_aggregate_pointer_actual) {
                    std::string ptr_actual = actual_emits_aggregate_value
                        ? address_of_src(src) : src;
                    args += cast_aggregate_pointer_actual_to_param_type(param, ptr_actual);
                } else if (wants_pointer_dummy_slot_actual) {
                    if (pointer_dummy_slot_actual) {
                        args += src;
                    } else {
                        args += "&" + src;
                    }
                } else if (wants_aggregate_dummy_slot_actual) {
                    if (aggregate_dummy_slot_actual) {
                        args += canonicalize_raw_pointer_actual_src(src);
                    } else if (plain_pointer_backed_local_aggregate_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if (wants_unlimited_polymorphic_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (wants_scalar_alloc_dummy_slot_actual) {
                    if (scalar_alloc_dummy_slot_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if (raw_pointer_actual) {
                    args += src;
                } else if( (is_c && !param_is_c_array_wrapper
                    && (param->m_intent == ASRUtils::intent_inout
                    || param->m_intent == ASRUtils::intent_out)
                    && (!ASRUtils::is_aggregate_type(param->m_type)
                        || (ASRUtils::is_pointer(param->m_type)
                            && !ASRUtils::is_array(param->m_type))))) {
                    args += address_of_src(src);
                } else if (param->m_intent == ASRUtils::intent_out) {
                    if (ASR::is_a<ASR::List_t>(*param->m_type) || 
                        ASR::is_a<ASR::Dict_t>(*param->m_type) || 
                        ASR::is_a<ASR::Tuple_t>(*param->m_type)) {
                        args += address_of_src(src);
                    } else {
                        args += pass_wrapper_arg(src);
                    }
                } else {
                    args += pass_wrapper_arg(src);
                }
            } else if (shape_call_arg && (ASR::is_a<ASR::ArrayItem_t>(*shape_call_arg)
                    || ASR::is_a<ASR::StructInstanceMember_t>(*shape_call_arg)
                    || ASR::is_a<ASR::UnionInstanceMember_t>(*shape_call_arg))) {
                if (is_c && ASR::is_a<ASR::StructInstanceMember_t>(*shape_call_arg)
                        && wants_raw_pointer_actual) {
                    ASR::StructInstanceMember_t *member_arg =
                        ASR::down_cast<ASR::StructInstanceMember_t>(shape_call_arg);
                    ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_arg->m_m);
                    if (ASR::is_a<ASR::Variable_t>(*member_sym)
                            && is_scalar_allocatable_storage_type(
                                ASR::down_cast<ASR::Variable_t>(member_sym)->m_type)) {
                        src = get_struct_instance_member_expr(*member_arg, false);
                        raw_pointer_actual = true;
                    }
                }
                if (!raw_pointer_actual
                        && wants_raw_pointer_actual
                        && ASRUtils::is_pointer(type)
                        && !ASRUtils::is_array(type)) {
                    raw_pointer_actual = true;
                }
                if (wants_aggregate_pointer_actual) {
                    std::string ptr_actual = actual_emits_aggregate_value
                        ? address_of_src(src) : src;
                    args += cast_aggregate_pointer_actual_to_param_type(param, ptr_actual);
                } else if (wants_pointer_dummy_slot_actual) {
                    if (raw_pointer_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if (wants_aggregate_dummy_slot_actual) {
                    if (raw_pointer_actual) {
                        args += canonicalize_raw_pointer_actual_src(src);
                    } else {
                        args += address_of_src(src);
                    }
                } else if (wants_unlimited_polymorphic_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (wants_scalar_alloc_dummy_slot_actual) {
                    if (is_c && ASR::is_a<ASR::StructInstanceMember_t>(*shape_call_arg)) {
                        ASR::StructInstanceMember_t *member_arg =
                            ASR::down_cast<ASR::StructInstanceMember_t>(shape_call_arg);
                        ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_arg->m_m);
                        if (ASR::is_a<ASR::Variable_t>(*member_sym)
                                && is_scalar_allocatable_storage_type(
                                    ASR::down_cast<ASR::Variable_t>(member_sym)->m_type)) {
                            src = get_struct_instance_member_expr(*member_arg, false);
                        }
                    }
                    if (scalar_alloc_dummy_slot_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if ((is_c && !param_is_c_array_wrapper
                        && (param->m_intent == ASRUtils::intent_inout
                        || param->m_intent == ASRUtils::intent_out)
                        && (!ASRUtils::is_aggregate_type(param->m_type)
                            || (ASRUtils::is_pointer(param->m_type)
                                && !ASRUtils::is_array(param->m_type))))
                    || ASR::is_a<ASR::StructType_t>(*type_unwrapped)) {
                    if (raw_pointer_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else {
                    args += pass_wrapper_arg(src);
                }
            } else {
                if (wants_aggregate_pointer_actual) {
                    args += cast_aggregate_pointer_actual_to_param_type(
                        param, address_of_src(src));
                } else if (wants_aggregate_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (wants_scalar_alloc_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (ASR::is_a<ASR::StructType_t>(*type_unwrapped)) {
                    args += address_of_src(src);
                } else {
                    args += pass_wrapper_arg(src);
                }
            }
            if (is_count_callee && i == 0) {
                count_mask_arg_src = get_c_descriptor_member_base_expr(src);
            }
            if (i < n_args-1) args += ", ";
        }
        bracket_open--;
        if (!call_arg_setup.empty()) {
            tmp_buffer_src.push_back(call_arg_setup);
        }
        array_compare_temp_cache = std::move(saved_array_compare_temp_cache);
        reuse_array_compare_temps_in_call_args =
            saved_reuse_array_compare_temps_in_call_args;
        return args;
    }

    std::string construct_call_args_from_index(ASR::Function_t* f, size_t start_idx,
            size_t n_args, ASR::call_arg_t* m_args, bool force_generated_abi=false,
            bool allow_no_copy_descriptor_views=true,
            bool use_named_no_copy_descriptor_views=true) {
        if (!force_generated_abi && f != nullptr
                && is_fortran_external_interface_function(*f)) {
            return construct_fortran_external_call_args(f, start_idx, n_args, m_args);
        }
        bracket_open++;
        bool saved_reuse_array_compare_temps_in_call_args =
            reuse_array_compare_temps_in_call_args;
        auto saved_array_compare_temp_cache = std::move(array_compare_temp_cache);
        reuse_array_compare_temps_in_call_args = true;
        array_compare_temp_cache.clear();
        bool is_count_callee = is_c && f != nullptr
            && std::string(f->m_name).find("count") != std::string::npos;
        std::string count_mask_arg_src;
        std::string args = "";
        std::string call_arg_setup;
        std::string no_copy_hidden_base_name;
        std::string no_copy_hidden_shape_src;
        int no_copy_hidden_rank = 0;
        int no_copy_hidden_dim = 0;
        size_t override_arg_index = static_cast<size_t>(-1);
        std::string override_arg_value;
        for (size_t i=start_idx; i<n_args; i++) {
            if (i == override_arg_index) {
                args += override_arg_value;
                if (i < n_args - 1) args += ", ";
                continue;
            }
            ASR::expr_t* call_arg = m_args[i].m_value;
            ASR::Variable_t *param = i < f->n_args ? ASRUtils::EXPR2VAR(f->m_args[i]) : nullptr;
            if (!no_copy_hidden_shape_src.empty() && no_copy_hidden_dim < no_copy_hidden_rank
                    && param != nullptr
                    && is_pass_array_by_data_hidden_arg_name(
                        std::string(param->m_name), no_copy_hidden_base_name)) {
                args += "((int32_t) " + no_copy_hidden_shape_src + "->dims["
                    + std::to_string(no_copy_hidden_dim) + "].length)";
                no_copy_hidden_dim++;
                if (i < n_args - 1) args += ", ";
                continue;
            }
            if (is_count_callee && i == start_idx + 1 && !count_mask_arg_src.empty()
                    && (ASR::is_a<ASR::ArrayBound_t>(*call_arg)
                        || ASR::is_a<ASR::ArraySize_t>(*call_arg))) {
                args += "((int32_t) " + count_mask_arg_src + "->dims[0].length + "
                    + count_mask_arg_src + "->dims[0].lower_bound - 1)";
                if (i < n_args - 1) args += ", ";
                continue;
            }
            ASR::expr_t* raw_call_arg = unwrap_c_lvalue_expr(call_arg);
            ASR::expr_t* shape_call_arg = raw_call_arg ? raw_call_arg : call_arg;
            auto address_of_src = [&](const std::string &current_src) -> std::string {
                return "&" + get_addressable_call_arg_src(shape_call_arg, current_src);
            };
            ASR::ttype_t* type = ASRUtils::expr_type(call_arg);
            ASR::ttype_t *type_unwrapped = ASRUtils::type_get_past_allocatable_pointer(type);
            ASR::ttype_t *param_type = i < f->n_args ? ASRUtils::expr_type(f->m_args[i]) : nullptr;
            ASR::Variable_t *actual_var = nullptr;
            ASR::ttype_t *param_type_unwrapped = param_type ?
                ASRUtils::type_get_past_allocatable_pointer(param_type) : nullptr;
            bool raw_pointer_actual = false;
            bool pointer_dummy_slot_actual = false;
            bool aggregate_dummy_slot_actual = false;
            bool scalar_alloc_dummy_slot_actual = false;
            bool param_is_optional_alloc_scalar_ref = is_c && param
                && param->m_presence == ASR::presenceType::Optional
                && param_type
                && ASR::is_a<ASR::Allocatable_t>(*param_type)
                && param_type_unwrapped
                && !ASRUtils::is_array(param_type)
                && !ASRUtils::is_aggregate_type(param_type)
                && !ASRUtils::is_character(*param_type_unwrapped);
            bool wants_raw_pointer_actual = param_expects_raw_pointer_actual(
                param, param_type, param_type_unwrapped);
            bool wants_pointer_dummy_slot_actual = is_pointer_dummy_slot_type(param);
            bool wants_aggregate_dummy_slot_actual = is_aggregate_dummy_slot_type(param);
            bool wants_unlimited_polymorphic_dummy_slot_actual =
                is_unlimited_polymorphic_dummy_slot_type(param);
            bool wants_scalar_alloc_dummy_slot_actual =
                is_scalar_allocatable_dummy_slot_type(param);
            bool wants_aggregate_pointer_actual = is_c && i < f->n_args
                && is_struct_or_class_type(param_type_unwrapped)
                && is_struct_or_class_type(type_unwrapped)
                && !wants_pointer_dummy_slot_actual
                && !wants_aggregate_dummy_slot_actual;
            std::string arg_src, arg_setup;
            if (!try_get_c_scalar_string_call_arg_view(
                    call_arg, param, param_type, arg_src, arg_setup)) {
                bool saved_force_storage_expr_in_call_args = force_storage_expr_in_call_args;
                force_storage_expr_in_call_args = is_c && wants_raw_pointer_actual;
                self().visit_expr(*call_arg);
                arg_src = src;
                arg_setup = drain_tmp_buffer();
                arg_setup += extract_stmt_setup_from_expr(arg_src);
                force_storage_expr_in_call_args = saved_force_storage_expr_in_call_args;
            }
            src = arg_src;
            if (is_c && raw_call_arg && ASR::is_a<ASR::Var_t>(*raw_call_arg)) {
                ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(raw_call_arg)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                    actual_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                }
            }
            bool param_defines_actual = param
                && (param->m_intent == ASRUtils::intent_out
                    || param->m_intent == ASRUtils::intent_inout
                    || (std::string(param->m_name) == "result"
                        && f != nullptr
                        && std::string(f->m_name).find("lcompilers_matmul")
                            != std::string::npos));
            if (is_c && param_defines_actual) {
                std::string lazy_setup =
                    self().emit_c_lazy_automatic_array_temp_allocation(
                        call_arg, arg_src);
                if (!lazy_setup.empty()) {
                    call_arg_setup += arg_setup;
                    arg_setup.clear();
                    call_arg_setup += lazy_setup;
                }
            }
            bool actual_is_raw_c_char_array_before_view =
                is_raw_c_char_array_dummy(actual_var);
            bool no_copy_descriptor_view_actual = allow_no_copy_descriptor_views
                && !actual_is_raw_c_char_array_before_view
                && try_build_c_array_no_copy_descriptor_view_arg(
                        call_arg, param_type,
                        param ? param->m_type_declaration : nullptr, arg_src,
                        use_named_no_copy_descriptor_views);
            if (no_copy_descriptor_view_actual) {
                call_arg_setup += drain_tmp_buffer();
                if (param != nullptr) {
                    no_copy_hidden_base_name = param ? std::string(param->m_name) : "";
                    no_copy_hidden_shape_src = get_c_descriptor_member_base_expr(arg_src);
                    no_copy_hidden_rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::type_get_past_allocatable_pointer(param_type));
                    no_copy_hidden_dim = 0;
                }
            }
            if (!no_copy_descriptor_view_actual && !arg_setup.empty()) {
                tmp_buffer_src.push_back(arg_setup);
            }
            if (is_c && is_compiler_created_scalar_storage_temp(arg_src)) {
                src = canonicalize_raw_pointer_actual_src(arg_src);
                arg_src = src;
            }
            bool pointer_backed_aggregate_actual = is_c
                && is_struct_or_class_type(type_unwrapped)
                && is_pointer_backed_struct_expr(call_arg);
            bool actual_emits_aggregate_value = wants_aggregate_pointer_actual
                && !pointer_backed_aggregate_actual;
            if (is_c && raw_call_arg && ASR::is_a<ASR::Var_t>(*raw_call_arg) && i < f->n_args) {
                ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(raw_call_arg)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                    ASR::Variable_t *arg_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                    actual_var = arg_var;
                    aggregate_dummy_slot_actual =
                        is_aggregate_dummy_slot_type(arg_var);
                    bool arg_is_scalar_alloc_storage =
                        is_scalar_allocatable_storage_type(arg_var->m_type);
                    bool arg_is_scalar_pointer_storage =
                        is_scalar_pointer_storage_type(arg_var->m_type);
                    if (wants_pointer_dummy_slot_actual
                            && ASRUtils::is_pointer(type)
                            && !ASRUtils::is_array(type)) {
                        src = CUtils::get_c_variable_name(*arg_var);
                        pointer_dummy_slot_actual = is_pointer_dummy_slot_type(arg_var);
                    } else if (wants_scalar_alloc_dummy_slot_actual && arg_is_scalar_alloc_storage) {
                        src = get_c_var_storage_name(arg_var);
                        scalar_alloc_dummy_slot_actual =
                            is_scalar_allocatable_dummy_slot_type(arg_var);
                    } else if (wants_raw_pointer_actual
                            && (arg_is_scalar_alloc_storage
                                || arg_is_scalar_pointer_storage)) {
                        src = arg_is_scalar_alloc_storage
                            ? get_c_var_storage_name(arg_var)
                            : CUtils::get_c_variable_name(*arg_var);
                        raw_pointer_actual = true;
                    } else if (param_is_optional_alloc_scalar_ref
                            && ASRUtils::is_pointer(type)
                            && !ASRUtils::is_array(type)) {
                        src = CUtils::get_c_variable_name(*arg_var);
                        raw_pointer_actual = true;
                    }
                }
                if (!wants_pointer_dummy_slot_actual
                        && ASRUtils::is_pointer(param_type)
                        && !ASRUtils::is_array(param_type)
                        && ASRUtils::is_pointer(type)
                        && !ASRUtils::is_array(type)) {
                    if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                        src = CUtils::get_c_variable_name(
                            *ASR::down_cast<ASR::Variable_t>(arg_sym));
                        raw_pointer_actual = true;
                    }
                }
            }
            if (normalize_optional_alloc_scalar_ref_actual(call_arg,
                    wants_raw_pointer_actual, src)) {
                raw_pointer_actual = true;
            }
            bool plain_pointer_backed_local_aggregate_actual =
                is_plain_pointer_backed_local_aggregate_actual(
                    actual_var, pointer_backed_aggregate_actual);
            if (!raw_pointer_actual
                    && param_is_optional_alloc_scalar_ref
                    && ASRUtils::is_pointer(type)
                    && !ASRUtils::is_array(type)) {
                src = canonicalize_raw_pointer_actual_src(arg_src);
                raw_pointer_actual = true;
            }
            if (raw_pointer_actual || wants_raw_pointer_actual) {
                src = canonicalize_raw_pointer_actual_src(src);
                arg_src = src;
            }
            if (no_copy_descriptor_view_actual) {
                src = arg_src;
            }
            ASR::expr_t *array_like_arg = call_arg;
            if (ASR::is_a<ASR::Cast_t>(*array_like_arg)) {
                ASR::Cast_t *cast_arg = ASR::down_cast<ASR::Cast_t>(array_like_arg);
                if (cast_arg->m_kind == ASR::cast_kindType::StringToArray) {
                    array_like_arg = cast_arg->m_arg;
                }
            }
            bool param_is_c_array_wrapper = get_c_array_wrapper_base_type(param_type) != nullptr;
            bool param_is_raw_c_char_array = is_raw_c_char_array_dummy(param);
            bool actual_is_raw_c_char_array = is_raw_c_char_array_dummy(actual_var);
            bool scalar_string_to_cchar = is_c
                && param_type && CUtils::is_len_one_character_array_type(param_type)
                && ASR::is_a<ASR::StringPhysicalCast_t>(*array_like_arg);
            if (is_c && param_type && CUtils::is_len_one_character_array_type(param_type)
                    && (actual_is_raw_c_char_array
                        || (type && CUtils::is_len_one_character_array_type(type))
                        || scalar_string_to_cchar)) {
                if (ASR::is_a<ASR::StringPhysicalCast_t>(*array_like_arg)) {
                    array_like_arg = ASR::down_cast<ASR::StringPhysicalCast_t>(array_like_arg)->m_arg;
                }
                if (!param_is_raw_c_char_array
                        && (actual_is_raw_c_char_array || scalar_string_to_cchar
                            || !ASRUtils::is_array(ASRUtils::expr_type(array_like_arg)))) {
                    std::string actual_len = "strlen(" + arg_src + ")";
                    ASR::Variable_t *next_param = (i + 1 < f->n_args)
                        ? ASRUtils::EXPR2VAR(f->m_args[i + 1]) : nullptr;
                    bool next_is_hidden_len = is_hidden_char_length_param(next_param);
                    size_t len_arg_index = i + 1;
                    if (next_is_hidden_len && i + 2 < n_args) {
                        len_arg_index = i + 2;
                    }
                    if (len_arg_index < n_args && m_args[len_arg_index].m_value) {
                        ASR::ttype_t *len_arg_type = ASRUtils::expr_type(m_args[len_arg_index].m_value);
                        len_arg_type = ASRUtils::type_get_past_allocatable_pointer(len_arg_type);
                        if (len_arg_type
                                && (ASRUtils::is_integer(*len_arg_type)
                                    || ASRUtils::is_unsigned_integer(*len_arg_type))) {
                            self().visit_expr(*m_args[len_arg_index].m_value);
                            actual_len = src;
                        }
                    }
                    if (next_is_hidden_len && i + 1 < n_args) {
                        override_arg_index = i + 1;
                        override_arg_value = actual_len;
                    }
                    std::string wrapper_type = get_c_array_wrapper_type_name(
                        param_type, param ? param->m_type_declaration : nullptr);
                    std::string cchar_len = get_unique_local_name("__lfortran_cchar_len");
                    std::string cchar_data = get_unique_local_name("__lfortran_cchar_data");
                    std::string cchar_i = get_unique_local_name("__lfortran_cchar_i");
                    std::string indent(indentation_level * indentation_spaces, ' ');
                    call_arg_setup += indent + "int64_t " + cchar_len + " = "
                        + actual_len + ";\n";
                    call_arg_setup += indent + "char *" + cchar_data + "[("
                        + cchar_len + " > 0) ? " + cchar_len + " : 1];\n";
                    call_arg_setup += indent + "for (int64_t " + cchar_i + " = 0; "
                        + cchar_i + " < " + cchar_len + "; " + cchar_i + "++) {\n";
                    call_arg_setup += indent + std::string(indentation_spaces, ' ')
                        + cchar_data + "[" + cchar_i + "] = " + arg_src + " + "
                        + cchar_i + ";\n";
                    call_arg_setup += indent + "}\n";
                    args += "(&(" + wrapper_type + "){ .data = " + cchar_data
                        + ", .dims = {{1, " + cchar_len + ", 1}}, .n_dims = 1, .offset = 0, .is_allocated = false })";
                } else if (param_is_raw_c_char_array
                        && (scalar_string_to_cchar || !ASRUtils::is_array(ASRUtils::expr_type(array_like_arg)))) {
                    args += arg_src;
                } else if (param_is_raw_c_char_array && !is_data_only_array_expr(call_arg)) {
                    args += build_c_char_array_descriptor_to_raw_arg(
                        arg_src, call_arg_setup);
                } else if (param_is_raw_c_char_array && is_data_only_array_expr(call_arg)) {
                    args += arg_src;
                } else {
                    args += param_is_raw_c_char_array
                        ? arg_src + "->data"
                        : cast_c_array_wrapper_ptr_to_target_type(
                            param_type, type, arg_src,
                            param ? param->m_type_declaration : nullptr,
                            actual_var ? actual_var->m_type_declaration : nullptr);
                }
                if (i < n_args - 1) args += ", ";
                continue;
            }
            auto pass_wrapper_arg = [&](const std::string &value_expr) -> std::string {
                std::map<std::string, std::string> dim_expr_overrides;
                std::string saved_src = src;
                for (size_t j = start_idx; j < n_args && j < f->n_args; j++) {
                    if (j == i || m_args[j].m_value == nullptr) {
                        continue;
                    }
                    ASR::Variable_t *dim_param = ASRUtils::EXPR2VAR(f->m_args[j]);
                    if (!dim_param || ASRUtils::is_array(dim_param->m_type)) {
                        continue;
                    }
                    self().visit_expr(*m_args[j].m_value);
                    dim_expr_overrides[CUtils::get_c_variable_name(*dim_param)] = src;
                }
                src = saved_src;
                if (is_c && ASR::is_a<ASR::ArrayPhysicalCast_t>(*call_arg)) {
                    ASR::ArrayPhysicalCast_t *cast =
                        ASR::down_cast<ASR::ArrayPhysicalCast_t>(call_arg);
                    ASR::ttype_t *target_array_type =
                        ASRUtils::type_get_past_allocatable_pointer(param_type);
                    ASR::ttype_t *source_array_type =
                        ASRUtils::type_get_past_allocatable_pointer(
                            ASRUtils::expr_type(cast->m_arg));
                    int target_rank = target_array_type
                        ? ASRUtils::extract_n_dims_from_ttype(target_array_type) : 0;
                    int source_rank = source_array_type
                        ? ASRUtils::extract_n_dims_from_ttype(source_array_type) : 0;
                    if (target_rank > 0 && source_rank > 0
                            && target_rank > source_rank) {
                        return build_c_array_wrapper_from_cast_target(
                            param_type, cast->m_arg, value_expr,
                            &dim_expr_overrides);
                    }
                    return value_expr;
                }
                return build_c_array_wrapper_from_cast_target(
                    param_type, call_arg, value_expr, &dim_expr_overrides);
            };
            bool pass_data_only_array_wrapper = is_c && param_is_c_array_wrapper
                && (is_data_only_array_expr(call_arg)
                    || is_fixed_size_array_storage_expr(call_arg)
                    || is_data_only_array_section_expr(call_arg));
            if (pass_data_only_array_wrapper) {
                std::string wrapper_arg = pass_wrapper_arg(src);
                args += wrapper_arg;
                if (param != nullptr) {
                    no_copy_hidden_base_name = std::string(param->m_name);
                    no_copy_hidden_shape_src =
                        get_c_descriptor_member_base_expr(wrapper_arg);
                    no_copy_hidden_rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::type_get_past_allocatable_pointer(param_type));
                    no_copy_hidden_dim = 0;
                }
                if (is_count_callee && i == start_idx) {
                    count_mask_arg_src = src;
                }
                if (i < n_args - 1) args += ", ";
                continue;
            }
            if (shape_call_arg && ASR::is_a<ASR::Var_t>(*shape_call_arg)
                && ASR::is_a<ASR::Variable_t>(
                    *ASRUtils::symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(shape_call_arg)->m_v))) {
                if (wants_aggregate_pointer_actual) {
                    std::string ptr_actual = actual_emits_aggregate_value
                        ? address_of_src(src) : src;
                    args += cast_aggregate_pointer_actual_to_param_type(param, ptr_actual);
                } else if (wants_pointer_dummy_slot_actual) {
                    if (pointer_dummy_slot_actual) {
                        args += src;
                    } else {
                        args += "&" + src;
                    }
                } else if (wants_aggregate_dummy_slot_actual) {
                    if (aggregate_dummy_slot_actual) {
                        args += canonicalize_raw_pointer_actual_src(src);
                    } else if (plain_pointer_backed_local_aggregate_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if (wants_unlimited_polymorphic_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (wants_scalar_alloc_dummy_slot_actual) {
                    if (scalar_alloc_dummy_slot_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if (raw_pointer_actual) {
                    args += src;
                } else if( (is_c && !param_is_c_array_wrapper
                    && (param->m_intent == ASRUtils::intent_inout
                    || param->m_intent == ASRUtils::intent_out)
                    && (!ASRUtils::is_aggregate_type(param->m_type)
                        || (ASRUtils::is_pointer(param->m_type)
                            && !ASRUtils::is_array(param->m_type))))) {
                    args += address_of_src(src);
                } else if (param->m_intent == ASRUtils::intent_out) {
                    if (ASR::is_a<ASR::List_t>(*param->m_type) || 
                        ASR::is_a<ASR::Dict_t>(*param->m_type) || 
                        ASR::is_a<ASR::Tuple_t>(*param->m_type)) {
                        args += address_of_src(src);
                    } else {
                        args += pass_wrapper_arg(src);
                    }
                } else {
                    args += pass_wrapper_arg(src);
                }
            } else if (shape_call_arg && (ASR::is_a<ASR::ArrayItem_t>(*shape_call_arg)
                    || ASR::is_a<ASR::StructInstanceMember_t>(*shape_call_arg)
                    || ASR::is_a<ASR::UnionInstanceMember_t>(*shape_call_arg))) {
                if (is_c && ASR::is_a<ASR::StructInstanceMember_t>(*shape_call_arg)
                        && wants_raw_pointer_actual) {
                    ASR::StructInstanceMember_t *member_arg =
                        ASR::down_cast<ASR::StructInstanceMember_t>(shape_call_arg);
                    ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_arg->m_m);
                    if (ASR::is_a<ASR::Variable_t>(*member_sym)
                            && is_scalar_allocatable_storage_type(
                                ASR::down_cast<ASR::Variable_t>(member_sym)->m_type)) {
                        src = get_struct_instance_member_expr(*member_arg, false);
                        raw_pointer_actual = true;
                    }
                }
                if (!raw_pointer_actual
                        && wants_raw_pointer_actual
                        && ASRUtils::is_pointer(type)
                        && !ASRUtils::is_array(type)) {
                    raw_pointer_actual = true;
                }
                if (wants_aggregate_pointer_actual) {
                    std::string ptr_actual = actual_emits_aggregate_value
                        ? address_of_src(src) : src;
                    args += cast_aggregate_pointer_actual_to_param_type(param, ptr_actual);
                } else if (wants_pointer_dummy_slot_actual) {
                    if (raw_pointer_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if (wants_aggregate_dummy_slot_actual) {
                    if (raw_pointer_actual) {
                        args += canonicalize_raw_pointer_actual_src(src);
                    } else {
                        args += address_of_src(src);
                    }
                } else if (wants_unlimited_polymorphic_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (wants_scalar_alloc_dummy_slot_actual) {
                    if (is_c && ASR::is_a<ASR::StructInstanceMember_t>(*shape_call_arg)) {
                        ASR::StructInstanceMember_t *member_arg =
                            ASR::down_cast<ASR::StructInstanceMember_t>(shape_call_arg);
                        ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_arg->m_m);
                        if (ASR::is_a<ASR::Variable_t>(*member_sym)
                                && is_scalar_allocatable_storage_type(
                                    ASR::down_cast<ASR::Variable_t>(member_sym)->m_type)) {
                            src = get_struct_instance_member_expr(*member_arg, false);
                        }
                    }
                    if (scalar_alloc_dummy_slot_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else if ((is_c && !param_is_c_array_wrapper
                        && (param->m_intent == ASRUtils::intent_inout
                        || param->m_intent == ASRUtils::intent_out)
                        && (!ASRUtils::is_aggregate_type(param->m_type)
                            || (ASRUtils::is_pointer(param->m_type)
                                && !ASRUtils::is_array(param->m_type))))
                    || ASR::is_a<ASR::StructType_t>(*type_unwrapped)) {
                    if (raw_pointer_actual) {
                        args += src;
                    } else {
                        args += address_of_src(src);
                    }
                } else {
                    args += pass_wrapper_arg(src);
                }
            } else {
                if (wants_aggregate_pointer_actual) {
                    args += cast_aggregate_pointer_actual_to_param_type(
                        param, address_of_src(src));
                } else if (wants_aggregate_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (wants_scalar_alloc_dummy_slot_actual) {
                    args += address_of_src(src);
                } else if (ASR::is_a<ASR::StructType_t>(*type_unwrapped)) {
                    args += address_of_src(src);
                } else {
                    args += pass_wrapper_arg(src);
                }
            }
            if (is_count_callee && i == start_idx) {
                count_mask_arg_src = get_c_descriptor_member_base_expr(src);
            }
            if (i < n_args-1) args += ", ";
        }
        bracket_open--;
        if (!call_arg_setup.empty()) {
            tmp_buffer_src.push_back(call_arg_setup);
        }
        array_compare_temp_cache = std::move(saved_array_compare_temp_cache);
        reuse_array_compare_temps_in_call_args =
            saved_reuse_array_compare_temps_in_call_args;
        return args;
    }

    std::string get_runtime_dispatch_self_arg(ASR::Variable_t *param,
            ASR::expr_t *dispatch_call_arg, ASR::expr_t *raw_dispatch_call_arg,
            const std::string &self_pointer_expr) {
        ASR::expr_t *call_arg = dispatch_call_arg;
        ASR::expr_t *shape_call_arg = raw_dispatch_call_arg ? raw_dispatch_call_arg : call_arg;
        if (is_pointer_dummy_slot_type(param)) {
            if (raw_dispatch_call_arg && ASR::is_a<ASR::Var_t>(*raw_dispatch_call_arg)) {
                ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(raw_dispatch_call_arg)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                    ASR::Variable_t *arg_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                    if (is_pointer_dummy_slot_type(arg_var)) {
                        return "(void*)" + CUtils::get_c_variable_name(*arg_var);
                    }
                }
            }
            return "(void*)&" + get_addressable_call_arg_src(shape_call_arg, self_pointer_expr);
        }
        return "(void*)" + self_pointer_expr;
    }

    std::string emit_c_tbp_parent_registration(const ASR::Struct_t &x) {
        if (!is_c || x.m_parent == nullptr) {
            return "";
        }
        ensure_c_backend_constructor_macro_decl();
        ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(x.m_parent);
        if (parent_sym == nullptr || !ASR::is_a<ASR::Struct_t>(*parent_sym)) {
            return "";
        }
        ASR::Struct_t *parent_struct = ASR::down_cast<ASR::Struct_t>(parent_sym);
        std::vector<ASR::StructMethodDeclaration_t*> parent_methods;
        collect_direct_concrete_struct_methods(parent_struct, parent_methods);
        std::string force_link_decls;
        std::string force_link_calls;
        for (ASR::StructMethodDeclaration_t *method: parent_methods) {
            std::string anchor_name = get_c_tbp_force_link_anchor_name(parent_struct, method);
            force_link_decls += "extern void " + anchor_name + "(void);\n";
            force_link_calls += "    " + anchor_name + "();\n";
        }
        int64_t child_type_id = get_struct_runtime_type_id(
            reinterpret_cast<ASR::symbol_t*>(const_cast<ASR::Struct_t*>(&x)));
        std::string registrar_name = get_unique_local_name(
            "__lfortran_register_type_parent_"
            + CUtils::sanitize_c_identifier(x.m_name)
            + "_x" + std::to_string(static_cast<uint64_t>(child_type_id)), false);
        return force_link_decls
            + "LFORTRAN_C_BACKEND_CONSTRUCTOR LFORTRAN_C_BACKEND_WEAK void "
            + registrar_name + "(void)\n{\n"
            "    _lfortran_register_c_type_parent("
            + std::to_string(child_type_id)
            + ", "
            + std::to_string(get_struct_runtime_type_id(parent_sym))
            + ");\n"
            + force_link_calls
            + "}\n\n";
    }

    std::string emit_c_tbp_registration_wrapper(const ASR::Function_t &x) {
        if (!is_c || x.n_args == 0) {
            return "";
        }
        ensure_c_backend_constructor_macro_decl();
        ASR::Struct_t *owner_struct = nullptr;
        ASR::StructMethodDeclaration_t *method = nullptr;
        if (!get_concrete_struct_method_binding(
                reinterpret_cast<ASR::symbol_t*>(const_cast<ASR::Function_t*>(&x)),
                owner_struct, method)) {
            return "";
        }
        if (method == nullptr || owner_struct == nullptr || method->m_is_deferred) {
            return "";
        }
        if (!can_emit_c_tbp_registration_wrapper(const_cast<ASR::Function_t*>(&x))) {
            return "";
        }
        std::string wrapper_name = get_unique_local_name(
            "__lfortran_tbp_wrapper_" + CUtils::sanitize_c_identifier(x.m_name), false);
        std::string ret_type = x.m_return_var
            ? get_return_var_type(ASRUtils::EXPR2VAR(x.m_return_var))
            : "void ";
        std::string wrapper_decl = ret_type + wrapper_name + "(void* __lfortran_self";
        for (size_t i = 1; i < x.n_args; i++) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                return "";
            }
            ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
            CDeclarationOptions c_decl_options;
            c_decl_options.pre_initialise_derived_type = false;
            wrapper_decl += ", " + self().convert_variable_decl(*arg, &c_decl_options);
        }
        wrapper_decl += ")";
        std::string derived_type = "struct "
            + CUtils::get_c_symbol_name(reinterpret_cast<ASR::symbol_t*>(owner_struct));
        ASR::Variable_t *self_param = ASRUtils::EXPR2VAR(x.m_args[0]);
        std::string self_forward_arg = is_pointer_dummy_slot_type(self_param)
            ? "((" + derived_type + "**)(__lfortran_self))"
            : "((" + derived_type + "*)(__lfortran_self))";
        std::string call_args = self_forward_arg;
        for (size_t i = 1; i < x.n_args; i++) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(x.m_args[i])->m_v);
            ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
            call_args += ", " + CUtils::get_c_variable_name(*arg);
        }
        std::string wrapper_src = "static " + wrapper_decl + "\n{\n";
        if (x.m_return_var) {
            wrapper_src += "    return " + get_emitted_function_name(x) + "(" + call_args + ");\n";
        } else {
            wrapper_src += "    " + get_emitted_function_name(x) + "(" + call_args + ");\n";
        }
        wrapper_src += "}\n\n";
        std::string registration_call = "_lfortran_register_c_tbp_impl(\""
            + escape_c_string_literal(std::string(method->m_name)) + "\", "
            + std::to_string(get_struct_runtime_type_id(
                reinterpret_cast<ASR::symbol_t*>(owner_struct)))
            + ", (lfortran_c_tbp_func_ptr)" + wrapper_name + ");";
        std::string anchor_name = get_c_tbp_force_link_anchor_name(owner_struct, method);
        std::string registrar_name = get_unique_local_name(
            "__lfortran_register_tbp_" + CUtils::sanitize_c_identifier(x.m_name), false);
        wrapper_src += "LFORTRAN_C_BACKEND_CONSTRUCTOR static void " + registrar_name
            + "(void)\n{\n"
            + "    " + registration_call + "\n}\n\n";
        wrapper_src += "void " + anchor_name + "(void)\n{\n"
            + "    " + registrar_name + "();\n}\n";
        return wrapper_src;
    }

    bool build_deferred_struct_method_dispatch(ASR::symbol_t *callee_sym, size_t n_args,
            ASR::call_arg_t *m_args, std::string &out, bool is_subroutine) {
        ASR::symbol_t *base_sym = ASRUtils::symbol_get_past_external(callee_sym);
        if (n_args == 0) {
            return false;
        }
        ASR::expr_t *dispatch_call_arg = m_args[0].m_value;
        ASR::expr_t *raw_dispatch_call_arg = unwrap_c_lvalue_expr(dispatch_call_arg);
        if (raw_dispatch_call_arg == nullptr) {
            return false;
        }
        ASR::ttype_t *recv_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(raw_dispatch_call_arg));
        bool receiver_is_class = ASRUtils::is_class_type(recv_type);
        ASR::StructMethodDeclaration_t *base_method = nullptr;
        ASR::symbol_t *owner_sym = nullptr;
        if (ASR::is_a<ASR::StructMethodDeclaration_t>(*base_sym)) {
            base_method = ASR::down_cast<ASR::StructMethodDeclaration_t>(base_sym);
            owner_sym = ASRUtils::symbol_get_past_external(
                ASRUtils::get_asr_owner(reinterpret_cast<ASR::symbol_t*>(base_method)));
        } else if (ASR::is_a<ASR::Function_t>(*base_sym)) {
            if (!(ASR::is_a<ASR::StructType_t>(*ASRUtils::extract_type(recv_type))
                    || ASRUtils::is_class_type(recv_type))) {
                return false;
            }
            owner_sym = ASRUtils::symbol_get_past_external(
                ASRUtils::get_struct_sym_from_struct_expr(raw_dispatch_call_arg));
            if (owner_sym == nullptr || !ASR::is_a<ASR::Struct_t>(*owner_sym)) {
                return false;
            }
            base_method = find_struct_method_by_proc_or_name(
                ASR::down_cast<ASR::Struct_t>(owner_sym), base_sym,
                ASRUtils::symbol_name(base_sym));
        } else {
            return false;
        }
        if (base_method == nullptr) {
            return false;
        }
        if (base_method->m_is_deferred && !receiver_is_class) {
            ASR::symbol_t *recv_struct_sym = ASRUtils::symbol_get_past_external(
                ASRUtils::get_struct_sym_from_struct_expr(raw_dispatch_call_arg));
            if (recv_struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*recv_struct_sym)) {
                ASR::StructMethodDeclaration_t *concrete_method = find_concrete_struct_method(
                    ASR::down_cast<ASR::Struct_t>(recv_struct_sym), base_method->m_name);
                if (concrete_method != nullptr && concrete_method->m_proc != nullptr) {
                    ASR::symbol_t *proc_sym = ASRUtils::symbol_get_past_external(
                        concrete_method->m_proc);
                    if (ASR::is_a<ASR::Function_t>(*proc_sym)) {
                        ASR::Function_t *concrete_fn = ASR::down_cast<ASR::Function_t>(proc_sym);
                        record_forward_decl_for_function(*concrete_fn);
                        std::string call_args =
                            construct_call_args(concrete_fn, n_args, m_args,
                                false, true, is_subroutine);
                        std::string call_setup = drain_tmp_buffer();
                        std::string call_expr = get_c_function_target_name(*concrete_fn)
                            + "(" + call_args + ")";
                        if (is_subroutine) {
                            out = call_setup + get_current_indent() + call_expr + ";\n";
                            return true;
                        }
                        if (concrete_fn->m_return_var == nullptr) {
                            return false;
                        }
                        out = call_setup + call_expr;
                        return true;
                    }
                }
            }
            return false;
        }
        if (!base_method->m_is_deferred && !receiver_is_class) {
            return false;
        }
        if (!ASR::is_a<ASR::Struct_t>(*owner_sym)) {
            return false;
        }
        self().visit_expr(*dispatch_call_arg);
        std::string self_expr = src;
        if (self_expr.empty()) {
            return false;
        }
        std::string self_pointer_expr = get_deferred_dispatch_receiver_pointer_src(
            raw_dispatch_call_arg, self_expr);

        ASR::Function_t *iface_fn = get_procedure_interface_function(callee_sym);
        if (iface_fn == nullptr || iface_fn->n_args == 0) {
            return false;
        }
        ASR::Variable_t *self_param = ASRUtils::EXPR2VAR(iface_fn->m_args[0]);
        std::string dispatch_self = get_runtime_dispatch_self_arg(
            self_param, dispatch_call_arg, raw_dispatch_call_arg, self_pointer_expr);
        std::string runtime_tag_expr = get_runtime_type_tag_expr(self_pointer_expr, true);
        std::string lookup_var = get_unique_local_name("__lfortran_tbp_impl");
        bool has_typevar = false;
        std::string wrapper_type = get_function_pointer_type(*iface_fn, has_typevar, true);
        if (has_typevar || wrapper_type.empty()) {
            return false;
        }
        std::string tail_args = construct_call_args_from_index(iface_fn, 1,
            n_args, m_args, false, true, is_subroutine);
        std::string tail_setup = drain_tmp_buffer();
        std::string call_expr = "((" + wrapper_type + ")" + lookup_var + ")(" + dispatch_self;
        if (!tail_args.empty()) {
            call_expr += ", " + tail_args;
        }
        call_expr += ")";
        std::string method_name = escape_c_string_literal(std::string(base_method->m_name));
        std::string method_hash = "UINT64_C("
            + std::to_string(get_stable_string_hash(std::string(base_method->m_name))) + ")";
        std::string force_link_src = emit_c_tbp_force_link_calls(
            ASR::down_cast<ASR::Struct_t>(owner_sym), base_method);

        if (is_subroutine) {
            std::string indent = get_current_indent();
            out = force_link_src
                + indent + "lfortran_c_tbp_func_ptr " + lookup_var
                + " = _lfortran_get_c_tbp_impl_by_hash_or_die(\"" + method_name + "\", "
                + method_hash + ", " + runtime_tag_expr + ");\n"
                + tail_setup
                + indent + call_expr + ";\n";
            return true;
        }

        if (iface_fn->m_return_var == nullptr) {
            return false;
        }
        ASR::ttype_t *ret_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(iface_fn->m_return_var));
        if (ASRUtils::is_aggregate_type(ret_type)) {
            return false;
        }
        std::string lookup_call = "_lfortran_get_c_tbp_impl_by_hash_or_die(\"" + method_name
            + "\", " + method_hash + ", " + runtime_tag_expr + ")";
        out = tail_setup + "((" + wrapper_type + ")" + lookup_call + ")("
            + dispatch_self;
        if (!tail_args.empty()) {
            out += ", " + tail_args;
        }
        out += ")";
        return true;
    }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        ASR::symbol_t *callee_sym = ASRUtils::symbol_get_past_external(x.m_name);
        if (is_c) {
            std::string deferred_dispatch;
            if (build_deferred_struct_method_dispatch(x.m_name, x.n_args, x.m_args,
                    deferred_dispatch, false)) {
                src = check_tmp_buffer() + deferred_dispatch;
                last_expr_precedence = 16;
                return;
            }
        }
        ASR::Function_t *fn = get_procedure_interface_function(x.m_name);
        std::string fn_name;
        if (ASR::is_a<ASR::Variable_t>(*callee_sym)) {
            if (is_c && x.m_dt != nullptr) {
                fn_name = get_procedure_component_callee_expr(x.m_dt, callee_sym);
            } else {
                fn_name = CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(callee_sym));
            }
        } else if (fn) {
            record_forward_decl_for_function(*fn);
            fn_name = get_c_function_target_name(*fn);
        } else {
            throw CodeGenError("Unsupported function call target", x.base.base.loc);
        }
        if (sym_info[get_hash((ASR::asr_t*)fn)].intrinsic_function) {
            if (fn_name == "size") {
                LCOMPILERS_ASSERT(x.n_args > 0);
                self().visit_expr(*x.m_args[0].m_value);
                std::string var_name = src;
                std::string args;
                if (x.n_args == 1) {
                    args = "0";
                } else {
                    for (size_t i=1; i<x.n_args; i++) {
                        self().visit_expr(*x.m_args[i].m_value);
                        args += src + "-1";
                        if (i < x.n_args-1) args += ", ";
                    }
                }
                src = var_name + ".extent(" + args + ")";
            } else if (fn_name == "int") {
                LCOMPILERS_ASSERT(x.n_args > 0);
                self().visit_expr(*x.m_args[0].m_value);
                src = "(int)" + src;
            } else if (fn_name == "not") {
                LCOMPILERS_ASSERT(x.n_args > 0);
                self().visit_expr(*x.m_args[0].m_value);
                src = "!(" + src + ")";
            } else {
                throw CodeGenError("Intrinsic function '" + fn_name
                        + "' not implemented");
            }
        } else {
            if (is_c) {
                std::string inline_call;
                if (try_emit_c_inline_dot_product_call(x, fn, fn_name, inline_call)) {
                    src = check_tmp_buffer() + inline_call;
                    last_expr_precedence = 2;
                    return;
                }
            }
            if (fn_name == "main") {
                fn_name = "_xx_lcompilers_changed_main_xx";
            }
            bool callee_is_procedure_variable = ASR::is_a<ASR::Variable_t>(*callee_sym);
            std::string call_args = construct_call_args(
                fn, x.n_args, x.m_args, callee_is_procedure_variable,
                true, false);
            src = drain_tmp_buffer() + fn_name + "(" + call_args + ")";
        }
        last_expr_precedence = 2;
        if( ASR::is_a<ASR::List_t>(*x.m_type) ) {
            ASR::List_t* list_type = ASR::down_cast<ASR::List_t>(x.m_type);
            const_name += std::to_string(const_vars_count);
            const_vars_count += 1;
            const_name = get_unique_local_name(const_name);
            std::string indent(indentation_level*indentation_spaces, ' ');
            tmp_buffer_src.push_back(check_tmp_buffer() + indent + c_ds_api->get_list_type(list_type) + " " +
                                const_name + " = " + src + ";\n");
            src = const_name;
            return;
        } else if( ASR::is_a<ASR::Dict_t>(*x.m_type) ) {
            ASR::Dict_t* dict_type = ASR::down_cast<ASR::Dict_t>(x.m_type);
            const_name += std::to_string(const_vars_count);
            const_vars_count += 1;
            const_name = get_unique_local_name(const_name);
            std::string indent(indentation_level*indentation_spaces, ' ');
            tmp_buffer_src.push_back(check_tmp_buffer() + indent + c_ds_api->get_dict_type(dict_type) +
                                " " + const_name + " = " + src + ";\n");
            src = const_name;
            return;
        }
        src = check_tmp_buffer() + src;
    }

    void visit_SizeOfType(const ASR::SizeOfType_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        std::string c_type = CUtils::get_c_type_from_ttype_t(x.m_arg);
        src = "sizeof(" + c_type + ")";
    }

    void visit_StringSection(const ASR::StringSection_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        std::string arg, left, right, step, left_present, rig_present;
        arg = src;
        std::string raw_left, raw_right;
        if (x.m_start) {
            self().visit_expr(*x.m_start);
            left = src;
            raw_left = left;
            left_present = "true";
        } else {
            left = "0";
            left_present = "false";
        }
        if (x.m_end) {
            self().visit_expr(*x.m_end);
            right = src;
            raw_right = right;
            rig_present = "true";
        } else {
            right = "0";
            rig_present = "false";
        }
        if (x.m_step) {
            self().visit_expr(*x.m_step);
            step = src;
        } else {
            step = "1";
        }
        if (left_present == "true") {
            left = "((" + left + ") - 1)";
        }
        if (rig_present == "true" && x.m_step) {
            right = "((" + step + ") > 0 ? (" + right + ") : ((" + right + ") - 2))";
        }
        if (is_c && left_present == "true" && rig_present == "true"
                && step == "1" && raw_left == raw_right) {
            src = "_lfortran_str_item(" + arg + ", _lfortran_str_len(" + arg + "), "
                + raw_left + ", (char[2]){0})";
            return;
        }
        src = "_lfortran_str_slice_alloc(_lfortran_get_default_allocator(), "
            + arg + ", _lfortran_str_len(" + arg + "), " + left + ", " + right + ", "
            + step + ", " + left_present + ", " + rig_present + ")";
    }

    void visit_StringChr(const ASR::StringChr_t& x) {
        if (!is_c) {
            CHECK_FAST_C_CPP(compiler_options, x)
        }
        self().visit_expr(*x.m_arg);
        src = "_lfortran_str_chr_alloc(_lfortran_get_default_allocator(), " + src + ")";
    }

    void visit_StringOrd(const ASR::StringOrd_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        if (ASR::is_a<ASR::StringConstant_t>(*x.m_arg)) {
            src = "(int)" + src + "[0]";
        } else {
            src = "_lfortran_str_ord_c(" + src + ")";
        }
    }

    void visit_StringRepeat(const ASR::StringRepeat_t &x) {
        if (!is_c) {
            CHECK_FAST_C_CPP(compiler_options, x)
        }
        self().visit_expr(*x.m_left);
        std::string s = src;
        std::string setup;
        std::string s_len = get_string_length_expr(x.m_left, s, setup);
        self().visit_expr(*x.m_right);
        std::string n = src;
        src = setup + "_lfortran_strrepeat_c_len_alloc(_lfortran_get_default_allocator(), "
            + s + ", " + s_len + ", " + n + ")";
    }

    void visit_Assignment(const ASR::Assignment_t &x) {
        invalidate_c_pow_cache_for_expr(x.m_target);
        auto is_unlimited_polymorphic_storage_type = [&](ASR::ttype_t *type) -> bool {
            type = ASRUtils::type_get_past_allocatable_pointer(type);
            if (type == nullptr) {
                return false;
            }
            if (ASRUtils::is_array(type)) {
                type = ASRUtils::type_get_past_array(type);
            }
            return type != nullptr
                && ASR::is_a<ASR::StructType_t>(*type)
                && ASR::down_cast<ASR::StructType_t>(type)->m_is_unlimited_polymorphic;
        };
        auto is_unlimited_polymorphic_dummy_slot_type = [&](ASR::Variable_t *var) -> bool {
            return var != nullptr
                && is_aggregate_dummy_slot_type(var)
                && is_unlimited_polymorphic_storage_type(var->m_type);
        };
        std::string target;
        ASR::ttype_t* m_target_type = ASRUtils::expr_type(x.m_target);
        ASR::ttype_t* m_value_type = ASRUtils::expr_type(x.m_value);
        bool is_target_list = ASR::is_a<ASR::List_t>(*m_target_type);
        bool is_value_list = ASR::is_a<ASR::List_t>(*m_value_type);
        bool is_target_tup = ASR::is_a<ASR::Tuple_t>(*m_target_type);
        bool is_value_tup = ASR::is_a<ASR::Tuple_t>(*m_value_type);
        bool is_target_dict = ASR::is_a<ASR::Dict_t>(*m_target_type);
        bool is_value_dict = ASR::is_a<ASR::Dict_t>(*m_value_type);
        bool alloc_return_var = false;
        bool target_is_unlimited_polymorphic_storage = false;
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string expr_setup;
        ASR::expr_t *unwrapped_target_expr = unwrap_c_lvalue_expr(x.m_target);
        if (is_c && current_function
                && std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                && is_unlimited_polymorphic_storage_type(m_target_type)) {
            self().visit_expr(*x.m_target);
            std::string move_target = src;
            self().visit_expr(*x.m_value);
            std::string move_value = src;
            src = check_tmp_buffer();
            src += indent + move_target + " = (void*)(" + move_value + ");\n";
            return;
        }
        if (is_c && x.m_move_allocation && current_function
                && std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                && ASRUtils::is_array(m_target_type)) {
            self().visit_expr(*x.m_target);
            std::string move_target = src;
            self().visit_expr(*x.m_value);
            std::string move_value = src;
            headers.insert("string.h");
            src = check_tmp_buffer();
            src += indent + "if (" + move_target + " == NULL) {\n";
            src += indent + std::string(indentation_spaces, ' ')
                + move_target + " = _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof(*"
                + move_target + "));\n";
            src += indent + std::string(indentation_spaces, ' ')
                + "memset(" + move_target + ", 0, sizeof(*" + move_target + "));\n";
            src += indent + "}\n";
            src += indent + "memcpy(" + move_target + ", " + move_value
                + ", sizeof(*" + move_target + "));\n";
            return;
        }
        if (is_c && is_unlimited_polymorphic_storage_type(m_target_type)
                && ASRUtils::is_array(m_value_type)) {
            self().visit_expr(*x.m_target);
            std::string poly_target = src;
            self().visit_expr(*x.m_value);
            std::string poly_value = src;
            headers.insert("string.h");
            src = check_tmp_buffer();
            src += indent + "if (" + poly_target + " == NULL) {\n";
            src += indent + std::string(indentation_spaces, ' ')
                + poly_target + " = _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof(*"
                + poly_value + "));\n";
            src += indent + "}\n";
            src += indent + "memcpy(" + poly_target + ", " + poly_value
                + ", sizeof(*" + poly_value + "));\n";
            return;
        }
        CArrayExprLoweringPlan array_expr_plan = plan_c_array_expr_assignment(
            x, unwrapped_target_expr);
        if (try_emit_c_array_expr_assignment_plan(array_expr_plan)) {
            return;
        }
        if (try_emit_vector_subscript_scalar_array_assignment(x, unwrapped_target_expr)) {
            return;
        }
        if (try_emit_c_char_array_bitcast_assignment(x, unwrapped_target_expr)) {
            return;
        }
        if (try_emit_c_char_array_bitcast_to_numeric_array_assignment(
                x, unwrapped_target_expr)) {
            return;
        }
        if (try_emit_c_array_section_reshape_assignment(x, unwrapped_target_expr)) {
            return;
        }
        ASR::ttype_t *bitcast_target_type = nullptr;
        if (ASR::is_a<ASR::BitCast_t>(*x.m_value)) {
            bitcast_target_type = ASR::down_cast<ASR::BitCast_t>(x.m_value)->m_type;
        }
        bool target_is_len_one_char_array = is_len_one_character_array_type(m_target_type)
            || is_len_one_character_array_type(bitcast_target_type);
        std::string scalar_char_bitcast_value_expr;
        bool have_scalar_char_bitcast_value =
            try_emit_scalar_to_char_array_bitcast_expr(x.m_value, scalar_char_bitcast_value_expr);
        if (!have_scalar_char_bitcast_value && is_c
                && target_is_len_one_char_array
                && ASR::is_a<ASR::BitCast_t>(*x.m_value)) {
            ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(x.m_value);
            ASR::expr_t *scalar_source = nullptr;
            if (bitcast->m_value
                    && (ASR::is_a<ASR::ArrayItem_t>(*bitcast->m_value)
                        || !ASRUtils::is_array(ASRUtils::expr_type(bitcast->m_value)))) {
                scalar_source = bitcast->m_value;
            } else if (bitcast->m_source
                    && (ASR::is_a<ASR::ArrayItem_t>(*bitcast->m_source)
                        || !ASRUtils::is_array(ASRUtils::expr_type(bitcast->m_source)))) {
                scalar_source = bitcast->m_source;
            }
            if (scalar_source != nullptr) {
                ASR::ttype_t *source_type = ASRUtils::expr_type(scalar_source);
                if (ASR::is_a<ASR::ArrayItem_t>(*scalar_source)) {
                    ASR::ArrayItem_t *array_item = ASR::down_cast<ASR::ArrayItem_t>(scalar_source);
                    source_type = ASRUtils::type_get_past_array(ASRUtils::expr_type(array_item->m_v));
                }
                if (ASRUtils::is_integer(*source_type)
                        || ASRUtils::is_unsigned_integer(*source_type)
                        || ASRUtils::is_real(*source_type)
                        || ASRUtils::is_logical(*source_type)) {
                    size_t nbytes = ASRUtils::get_fixed_size_of_array(m_target_type);
                    std::string target_code = CUtils::get_c_type_code(m_target_type, true, false);
                    std::string target_type_name = get_c_array_wrapper_type_name(m_target_type);
                    std::string source_type_name = CUtils::get_c_type_from_ttype_t(source_type);
                    std::string source_code = CUtils::get_c_type_code(source_type, false, false);
                    self().visit_expr(*scalar_source);
                    scalar_char_bitcast_value_expr = c_utils_functions->get_bitcast_scalar_to_char_array(
                        target_type_name, source_type_name, source_code, target_code, nbytes)
                        + "(" + src + ")";
                    have_scalar_char_bitcast_value = true;
                }
            }
        }
        if (is_c && target_is_len_one_char_array
                && (have_scalar_char_bitcast_value
                    || (!ASRUtils::is_array(m_value_type)
                        && (ASRUtils::is_integer(*m_value_type)
                            || ASRUtils::is_unsigned_integer(*m_value_type)
                            || ASRUtils::is_real(*m_value_type)
                            || ASRUtils::is_logical(*m_value_type))))) {
            self().visit_expr(*x.m_target);
            std::string target_expr = src;
            std::string value_expr = scalar_char_bitcast_value_expr;
            if (!have_scalar_char_bitcast_value) {
                size_t nbytes = ASRUtils::get_fixed_size_of_array(m_target_type);
                std::string target_code = CUtils::get_c_type_code(m_target_type, true, false);
                std::string target_type_name = get_c_array_wrapper_type_name(m_target_type);
                std::string source_type_name = CUtils::get_c_type_from_ttype_t(m_value_type);
                std::string source_code = CUtils::get_c_type_code(m_value_type, false, false);
                self().visit_expr(*x.m_value);
                value_expr = c_utils_functions->get_bitcast_scalar_to_char_array(
                    target_type_name, source_type_name, source_code, target_code, nbytes)
                    + "(" + src + ")";
            }
            src = check_tmp_buffer();
            src += indent + c_ds_api->get_deepcopy(m_target_type, value_expr, target_expr) + "\n";
            return;
        }
        auto get_string_storage_length = [&](ASR::expr_t *expr,
                                            const std::string &expr_src) -> std::string {
            ASR::String_t *str_type = ASRUtils::get_string_type(expr);
            if (str_type && str_type->m_len) {
                self().visit_expr(*str_type->m_len);
                return src;
            }
            return get_c_string_runtime_length_expr(expr_src);
        };
        if (is_c && ASR::is_a<ASR::StringItem_t>(*x.m_target)) {
            ASR::StringItem_t *si = ASR::down_cast<ASR::StringItem_t>(x.m_target);
            self().visit_expr(*si->m_arg);
            std::string string_arg = src;
            std::string string_len = get_string_storage_length(si->m_arg, string_arg);
            std::string value_len, value_setup, value_cleanup;
            std::string string_value;
            if (!try_get_unit_step_string_section_view(
                    x.m_value, string_value, value_len, value_setup)) {
                bool materialized = try_materialize_c_intrinsic_string_expr(
                        x.m_value, string_value, value_len, value_setup, value_cleanup);
                if (!materialized) {
                    self().visit_expr(*x.m_value);
                    string_value = src;
                    value_setup += drain_tmp_buffer();
                    value_setup += extract_stmt_setup_from_expr(string_value);
                    materialize_allocating_string_expr(
                        x.m_value, string_value, value_len, value_setup, value_cleanup);
                }
            }
            self().visit_expr(*si->m_idx);
            std::string idx = src;
            headers.insert("string.h");
            src = check_tmp_buffer();
            src += value_setup;
            std::string updated_value_name = get_unique_local_name("__lfortran_string_update");
            std::string updated_value =
                "_lfortran_str_slice_assign_alloc(_lfortran_get_default_allocator(), " +
                string_arg + ", " + string_len + ", " +
                string_value + ", " + value_len + ", " +
                idx + ", " + idx + ", 1, true, true)";
            src += indent + "char* " + updated_value_name + " = " + updated_value + ";\n";
            src += indent + "_lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &"
                + string_arg + ", NULL, true, true, " + updated_value_name
                + ", " + string_len + ");\n";
            src += indent + "if (" + updated_value_name + " != " + string_arg + ") {\n";
            src += indent + std::string(indentation_spaces, ' ')
                + "_lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + updated_value_name + ");\n";
            src += indent + "}\n";
            src += value_cleanup;
            return;
        }
        if (is_c && ASR::is_a<ASR::StringSection_t>(*x.m_target)) {
            ASR::StringSection_t *ss = ASR::down_cast<ASR::StringSection_t>(x.m_target);
            self().visit_expr(*ss->m_arg);
            std::string string_arg = src;
            std::string string_len = get_string_storage_length(ss->m_arg, string_arg);
            std::string value_len, value_setup, value_cleanup;
            std::string string_value;
            if (!try_get_unit_step_string_section_view(
                    x.m_value, string_value, value_len, value_setup)) {
                bool materialized = try_materialize_c_intrinsic_string_expr(
                        x.m_value, string_value, value_len, value_setup, value_cleanup);
                if (!materialized) {
                    self().visit_expr(*x.m_value);
                    string_value = src;
                    value_setup += drain_tmp_buffer();
                    value_setup += extract_stmt_setup_from_expr(string_value);
                    materialize_allocating_string_expr(
                        x.m_value, string_value, value_len, value_setup, value_cleanup);
                }
            }
            std::string left, right, step, left_present, right_present;
            if (ss->m_start) {
                self().visit_expr(*ss->m_start);
                left = src;
                left_present = "true";
            } else {
                left = "0";
                left_present = "false";
            }
            if (ss->m_end) {
                self().visit_expr(*ss->m_end);
                right = src;
                right_present = "true";
            } else {
                right = "0";
                right_present = "false";
            }
            if (ss->m_step) {
                self().visit_expr(*ss->m_step);
                step = src;
            } else {
                step = "1";
            }
            headers.insert("string.h");
            src = check_tmp_buffer();
            src += value_setup;
            std::string updated_value_name = get_unique_local_name("__lfortran_string_update");
            std::string updated_value =
                "_lfortran_str_slice_assign_alloc(_lfortran_get_default_allocator(), " +
                string_arg + ", " + string_len + ", " +
                string_value + ", " + value_len + ", " +
                left + ", " + right + ", " + step + ", " +
                left_present + ", " + right_present + ")";
            src += indent + "char* " + updated_value_name + " = " + updated_value + ";\n";
            src += indent + "_lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &"
                + string_arg + ", NULL, true, true, " + updated_value_name
                + ", " + string_len + ");\n";
            src += indent + "if (" + updated_value_name + " != " + string_arg + ") {\n";
            src += indent + std::string(indentation_spaces, ' ')
                + "_lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + updated_value_name + ");\n";
            src += indent + "}\n";
            src += value_cleanup;
            return;
        }
        if (ASRUtils::is_simd_array(x.m_target)) {
            this->visit_expr(*x.m_target);
            target = src;
            if (ASR::is_a<ASR::Var_t>(*x.m_value) ||
                    ASR::is_a<ASR::ArraySection_t>(*x.m_value)) {
                std::string arr_element_type = CUtils::get_c_type_from_ttype_t(
                    ASRUtils::expr_type(x.m_value));
                std::string size = std::to_string(ASRUtils::get_fixed_size_of_array(
                    ASRUtils::expr_type(x.m_target)));
                std::string value;
                if (ASR::is_a<ASR::ArraySection_t>(*x.m_value)) {
                    ASR::ArraySection_t *arr = ASR::down_cast<ASR::ArraySection_t>(x.m_value);
                    this->visit_expr(*arr->m_v);
                    value = src;
                    if(!ASR::is_a<ASR::ArrayBound_t>(*arr->m_args->m_left)) {
                        this->visit_expr(*arr->m_args->m_left);
                        int n_dims = ASRUtils::extract_n_dims_from_ttype(arr->m_type) - 1;
                        value += "->data + (" + src + " - "+ value +"->dims["
                            + std::to_string(n_dims) +"].lower_bound)";
                    } else {
                        value += "->data";
                    }
                } else if (ASR::is_a<ASR::Var_t>(*x.m_value)) {
                    this->visit_expr(*x.m_value);
                    value = src + "->data";
                }
                src = indent + "memcpy(&"+ target +", "+ value +", sizeof("
                    + arr_element_type + ") * "+ size +");\n";
                return;
            }
        } else if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
            ASR::Var_t* x_m_target = ASR::down_cast<ASR::Var_t>(x.m_target);
            ASR::symbol_t *target_sym = ASRUtils::symbol_get_past_external(x_m_target->m_v);
            bool target_is_pointer_return_var = false;
            bool target_is_pointer_dummy_slot = false;
            bool target_is_aggregate_dummy_slot = false;
            bool target_is_unlimited_polymorphic_dummy_slot = false;
            bool target_is_cptr_dummy_slot = false;
            bool target_is_scalar_alloc_dummy_slot = false;
            bool target_is_scalar_alloc_storage = false;
            if (current_function && current_function->m_return_var
                    && ASR::is_a<ASR::Var_t>(*current_function->m_return_var)
                    && ASRUtils::is_pointer(m_target_type)
                    && !ASRUtils::is_array(m_target_type)) {
                ASR::symbol_t *ret_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(current_function->m_return_var)->m_v);
                target_is_pointer_return_var = (ret_sym == target_sym);
            }
            if (is_c && ASR::is_a<ASR::Variable_t>(*target_sym)) {
                ASR::Variable_t *target_var = ASR::down_cast<ASR::Variable_t>(target_sym);
                target_is_unlimited_polymorphic_storage =
                    is_unlimited_polymorphic_storage_type(target_var->m_type);
                target_is_pointer_dummy_slot = ASRUtils::is_arg_dummy(target_var->m_intent)
                    && ASRUtils::is_pointer(target_var->m_type)
                    && !ASRUtils::is_array(target_var->m_type)
                    && !ASR::is_a<ASR::FunctionType_t>(
                        *ASRUtils::type_get_past_pointer(target_var->m_type));
                target_is_aggregate_dummy_slot =
                    is_aggregate_dummy_slot_type(target_var);
                target_is_unlimited_polymorphic_dummy_slot =
                    is_unlimited_polymorphic_dummy_slot_type(target_var);
                target_is_cptr_dummy_slot =
                    (target_var->m_intent == ASRUtils::intent_inout
                        || target_var->m_intent == ASRUtils::intent_out)
                    && ASR::is_a<ASR::CPtr_t>(*target_var->m_type)
                    && !ASRUtils::is_array(target_var->m_type);
                target_is_scalar_alloc_dummy_slot =
                    is_scalar_allocatable_dummy_slot_type(target_var);
                target_is_scalar_alloc_storage = is_scalar_allocatable_storage_type(target_var->m_type);
            }
            if (target_is_pointer_return_var) {
                target = CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(target_sym));
            } else if (target_is_unlimited_polymorphic_dummy_slot) {
                target = "(*((void**)" + CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(target_sym)) + "))";
            } else if (target_is_cptr_dummy_slot) {
                target = "(*((void**)" + CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(target_sym)) + "))";
            } else if (target_is_aggregate_dummy_slot) {
                target = "(*" + CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(target_sym)) + ")";
            } else if (target_is_pointer_dummy_slot) {
                target = "(*" + CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(target_sym)) + ")";
            } else if (target_is_scalar_alloc_dummy_slot) {
                target = "(*" + get_c_var_storage_name(
                    ASR::down_cast<ASR::Variable_t>(target_sym)) + ")";
            } else if (target_is_scalar_alloc_storage) {
                target = get_c_var_storage_name(
                    ASR::down_cast<ASR::Variable_t>(target_sym));
            } else {
                visit_Var(*x_m_target);
                target = src;
            }
            if (!is_c && ASRUtils::is_array(ASRUtils::expr_type(x.m_target))) {
                target += "->data";
            }
            if (target == "_lpython_return_variable" && ASRUtils::is_character(*m_target_type)) {
                // ASR assigns return variable only once at the end of function
                alloc_return_var = true;
            }
        } else if (ASR::is_a<ASR::ArrayItem_t>(*x.m_target)) {
            self().visit_ArrayItem(*ASR::down_cast<ASR::ArrayItem_t>(x.m_target));
            target = src;
        } else if (ASR::is_a<ASR::StructInstanceMember_t>(*x.m_target)) {
            ASR::StructInstanceMember_t *member_target =
                ASR::down_cast<ASR::StructInstanceMember_t>(x.m_target);
            ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_target->m_m);
            if (is_c && ASR::is_a<ASR::Variable_t>(*member_sym)
                    && is_scalar_allocatable_storage_type(
                        ASR::down_cast<ASR::Variable_t>(member_sym)->m_type)) {
                target = get_struct_instance_member_expr(*member_target, false);
            } else {
                visit_StructInstanceMember(*member_target);
                target = src;
            }
            if (is_c && ASR::is_a<ASR::Variable_t>(*member_sym)) {
                target_is_unlimited_polymorphic_storage =
                    is_unlimited_polymorphic_storage_type(
                        ASR::down_cast<ASR::Variable_t>(member_sym)->m_type);
            }
        } else if (ASR::is_a<ASR::UnionInstanceMember_t>(*x.m_target)) {
            visit_UnionInstanceMember(*ASR::down_cast<ASR::UnionInstanceMember_t>(x.m_target));
            target = src;
        } else if (ASR::is_a<ASR::ListItem_t>(*x.m_target)) {
            self().visit_ListItem(*ASR::down_cast<ASR::ListItem_t>(x.m_target));
            target = src;
        } else if (ASR::is_a<ASR::TupleItem_t>(*x.m_target)) {
            self().visit_TupleItem(*ASR::down_cast<ASR::TupleItem_t>(x.m_target));
            target = src;
        } else if (ASR::is_a<ASR::TupleConstant_t>(*x.m_target)) {
            ASR::TupleConstant_t *tup_c = ASR::down_cast<ASR::TupleConstant_t>(x.m_target);
            std::string src_tmp = "", val_name = "";
            if (ASR::is_a<ASR::TupleConstant_t>(*x.m_value)) {
                ASR::TupleConstant_t *tup_const = ASR::down_cast<ASR::TupleConstant_t>(x.m_value);
                self().visit_TupleConstant(*tup_const);
                val_name = const_var_names[get_hash((ASR::asr_t*)tup_const)];
            } else if (ASR::is_a<ASR::FunctionCall_t>(*x.m_value)) {
                self().visit_FunctionCall(*ASR::down_cast<ASR::FunctionCall_t>(x.m_value));
                ASR::Tuple_t* t = ASR::down_cast<ASR::Tuple_t>(tup_c->m_type);
                std::string tuple_type_c = c_ds_api->get_tuple_type(t);
                const_name += std::to_string(const_vars_count);
                const_vars_count += 1;
                const_name = get_unique_local_name(const_name);
                src_tmp += indent + tuple_type_c + " " + const_name + " = " + src + ";\n";
                val_name = const_name;
            } else {
                visit_Var(*ASR::down_cast<ASR::Var_t>(x.m_value));
                val_name = src;
            }
            for (size_t i=0; i<tup_c->n_elements; i++) {
                self().visit_expr(*tup_c->m_elements[i]);
                ASR::ttype_t *t = ASRUtils::expr_type(tup_c->m_elements[i]);
                src_tmp += indent + c_ds_api->get_deepcopy(t,
                        val_name + ".element_" + std::to_string(i), src) + "\n";
            }
            src = check_tmp_buffer() + src_tmp;
            return;
        } else if (ASR::is_a<ASR::DictItem_t>(*x.m_target)) {
            self().visit_DictItem(*ASR::down_cast<ASR::DictItem_t>(x.m_target));
            target = src;
        } else if (ASR::is_a<ASR::Cast_t>(*x.m_target)) {
            ASR::Cast_t *cast_target = ASR::down_cast<ASR::Cast_t>(x.m_target);
            switch (cast_target->m_kind) {
                case ASR::cast_kindType::ClassToStruct:
                case ASR::cast_kindType::ClassToClass:
                case ASR::cast_kindType::ClassToIntrinsic: {
                    self().visit_expr(*cast_target->m_arg);
                    target = src;
                    break;
                }
                default: {
                    throw CodeGenError("Assignment target cast kind not implemented in C backend: " +
                        std::to_string(static_cast<int>(cast_target->m_kind)), x.base.base.loc);
                }
            }
        } else {
            throw CodeGenError("Assignment target not implemented in C backend for ASR node type " +
                std::to_string(static_cast<int>(x.m_target->type)), x.base.base.loc);
        }
        expr_setup += drain_tmp_buffer();
        expr_setup += extract_stmt_setup_from_expr(target);
        if (is_c) {
            expr_setup += self().emit_c_lazy_automatic_array_temp_allocation(
                x.m_target, target);
        }
        auto emit_c_allocatable_function_result_slot_setup =
                [&](ASR::expr_t *target_expr, ASR::expr_t *value_expr,
                    const std::string &target_src) -> std::string {
            if (!is_c
                    || !ASR::is_a<ASR::FunctionCall_t>(*value_expr)
                    || !ASR::is_a<ASR::Allocatable_t>(*m_target_type)
                    || ASRUtils::is_array(m_target_type)) {
                return "";
            }
            ASR::ttype_t *target_value_type =
                ASRUtils::type_get_past_allocatable_pointer(m_target_type);
            ASR::ttype_t *value_value_type =
                ASRUtils::type_get_past_allocatable_pointer(m_value_type);
            if (target_value_type == nullptr || value_value_type == nullptr
                    || !ASR::is_a<ASR::StructType_t>(*target_value_type)
                    || !ASR::is_a<ASR::StructType_t>(*value_value_type)
                    || ASR::down_cast<ASR::StructType_t>(
                        target_value_type)->m_is_unlimited_polymorphic) {
                return "";
            }
            ASR::symbol_t *value_struct_sym = ASRUtils::symbol_get_past_external(
                ASRUtils::get_struct_sym_from_struct_expr(value_expr));
            if (value_struct_sym == nullptr
                    || !ASR::is_a<ASR::Struct_t>(*value_struct_sym)) {
                return "";
            }
            ASR::symbol_t *target_type_decl =
                get_expr_type_declaration_symbol(target_expr);
            if (target_type_decl == nullptr) {
                target_type_decl = value_struct_sym;
            }
            std::string target_concrete_type = get_c_concrete_type_from_ttype_t(
                target_value_type, target_type_decl);
            std::string allocation_type = "struct "
                + CUtils::get_c_symbol_name(value_struct_sym);
            if (target_concrete_type.empty() || target_concrete_type == "void*"
                    || allocation_type.empty()) {
                return "";
            }
            headers.insert("string.h");
            std::string setup;
            std::string result_type_id =
                std::to_string(get_struct_runtime_type_id(value_struct_sym));
            std::string old_type_id_name =
                get_unique_local_name("__lfortran_result_slot_type_id");
            setup += indent + "if (" + target_src + " != NULL) {\n";
            setup += indent + std::string(indentation_spaces, ' ')
                + "int64_t " + old_type_id_name + " = "
                + get_runtime_type_tag_expr(target_src, true) + ";\n";
            setup += indent + std::string(indentation_spaces, ' ')
                + "_lfortran_cleanup_c_struct(" + old_type_id_name
                + ", (void*) " + target_src + ");\n";
            setup += indent + std::string(indentation_spaces, ' ')
                + "if (" + old_type_id_name + " != " + result_type_id + ") {\n";
            setup += indent + std::string(2 * indentation_spaces, ' ')
                + "_lfortran_free_alloc(_lfortran_get_default_allocator(), (char*) "
                + target_src + ");\n";
            setup += indent + std::string(2 * indentation_spaces, ' ')
                + target_src + " = NULL;\n";
            setup += indent + std::string(indentation_spaces, ' ') + "}\n";
            setup += indent + "}\n";
            setup += indent + "if (" + target_src + " == NULL) {\n";
            setup += indent + std::string(indentation_spaces, ' ')
                + target_src + " = (" + target_concrete_type
                + "*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof("
                + allocation_type + "));\n";
            setup += indent + std::string(indentation_spaces, ' ')
                + "memset(" + target_src + ", 0, sizeof(" + allocation_type + "));\n";
            setup += indent + std::string(indentation_spaces, ' ')
                + get_runtime_type_tag_expr(target_src, true) + " = "
                + result_type_id + ";\n";
            setup += indent + "}\n";
            return setup;
        };
        expr_setup += emit_c_allocatable_function_result_slot_setup(
            x.m_target, x.m_value, target);
        from_std_vector_helper.clear();
        if( ASR::is_a<ASR::UnionConstructor_t>(*x.m_value) ) {
            src = "";
            return ;
        }
        if (is_c && ASR::is_a<ASR::BitCast_t>(*x.m_value)
                && is_len_one_character_array_type(
                    ASR::down_cast<ASR::BitCast_t>(x.m_value)->m_type)) {
            ASR::BitCast_t *bitcast = ASR::down_cast<ASR::BitCast_t>(x.m_value);
            ASR::expr_t *scalar_source = nullptr;
            if (bitcast->m_value
                    && (ASR::is_a<ASR::ArrayItem_t>(*bitcast->m_value)
                        || !ASRUtils::is_array(ASRUtils::expr_type(bitcast->m_value)))) {
                scalar_source = bitcast->m_value;
            } else if (bitcast->m_source
                    && (ASR::is_a<ASR::ArrayItem_t>(*bitcast->m_source)
                        || !ASRUtils::is_array(ASRUtils::expr_type(bitcast->m_source)))) {
                scalar_source = bitcast->m_source;
            }
            if (scalar_source != nullptr) {
                ASR::ttype_t *source_type = ASRUtils::expr_type(scalar_source);
                if (ASR::is_a<ASR::ArrayItem_t>(*scalar_source)) {
                    ASR::ArrayItem_t *array_item = ASR::down_cast<ASR::ArrayItem_t>(scalar_source);
                    source_type = ASRUtils::type_get_past_array(ASRUtils::expr_type(array_item->m_v));
                }
                if (ASRUtils::is_integer(*source_type)
                        || ASRUtils::is_unsigned_integer(*source_type)
                        || ASRUtils::is_real(*source_type)
                        || ASRUtils::is_logical(*source_type)) {
                    headers.insert("string.h");
                    self().visit_expr(*x.m_target);
                    std::string target_expr = src;
                    self().visit_expr(*scalar_source);
                    std::string scalar_expr = src;
                    size_t nbytes = ASRUtils::get_fixed_size_of_array(m_target_type);
                    std::string bytes_name = get_unique_local_name("__lfortran_bitcast_bytes");
                    std::string char_name = get_unique_local_name("__lfortran_bitcast_char");
                    std::string idx_name = get_unique_local_name("__lfortran_bitcast_i");
                    src = check_tmp_buffer();
                    src += indent + "{\n";
                    indentation_level++;
                    std::string inner_indent(indentation_level * indentation_spaces, ' ');
                    src += inner_indent + "unsigned char *" + bytes_name
                        + " = (unsigned char*) &(" + scalar_expr + ");\n";
                    src += inner_indent + "char " + char_name + "[2];\n";
                    src += inner_indent + char_name + "[1] = '\\0';\n";
                    src += inner_indent + "for (int32_t " + idx_name + " = 0; " + idx_name
                        + " < " + std::to_string(nbytes) + "; " + idx_name + "++) {\n";
                    indentation_level++;
                    inner_indent = std::string(indentation_level * indentation_spaces, ' ');
                    src += inner_indent + char_name + "[0] = (char)" + bytes_name + "[" + idx_name + "];\n";
                    src += inner_indent + "_lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &"
                        + target_expr + "->data[" + target_expr + "->offset + " + idx_name + "], NULL, true, true, "
                        + char_name + ", 1);\n";
                    indentation_level--;
                    inner_indent = std::string(indentation_level * indentation_spaces, ' ');
                    src += inner_indent + "}\n";
                    indentation_level--;
                    src += indent + "}\n";
                    return;
                }
            }
        }
        if (try_emit_c_char_array_bitcast_to_string_assignment(
                x, target, m_target_type)) {
            return;
        }
        std::string value;
        std::string value_view_len, value_view_setup;
        bool enable_c_pow_cache_for_value = is_c && current_function
            && is_c_pow_cache_safe_expr(x.m_value);
        ASR::ttype_t *m_target_scalar_type =
            ASRUtils::type_get_past_allocatable_pointer(m_target_type);
        bool target_is_scalar_character = is_c
            && !ASRUtils::is_array(m_target_scalar_type)
            && ASRUtils::is_character(*m_target_scalar_type);
        bool value_is_string_view = target_is_scalar_character
            && try_get_unit_step_string_section_view(
                x.m_value, value, value_view_len, value_view_setup);
        if (value_is_string_view) {
            expr_setup += value_view_setup;
        } else if (!try_emit_scalar_to_char_array_bitcast_expr(x.m_value, value)) {
            if (enable_c_pow_cache_for_value) {
                c_pow_cache_safe_expr_depth++;
            }
            self().visit_expr(*x.m_value);
            if (enable_c_pow_cache_for_value) {
                c_pow_cache_safe_expr_depth--;
            }
            value = src;
        }
        expr_setup += drain_tmp_buffer();
        expr_setup += extract_stmt_setup_from_expr(value);
        if (!expr_setup.empty()) {
            tmp_buffer_src.push_back(expr_setup);
        }
        if (is_c && current_function
                && std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                && target_is_unlimited_polymorphic_storage) {
            src = check_tmp_buffer();
            src += indent + target + " = (void*)(" + value + ");\n";
            return;
        }
        if (is_c && target_is_unlimited_polymorphic_storage) {
            src = check_tmp_buffer();
            src += indent + target + " = (void*)(" + value + ");\n";
            return;
        }
        ASR::ttype_t *target_value_type = ASRUtils::type_get_past_allocatable_pointer(m_target_type);
        if (ASR::is_a<ASR::ArrayItem_t>(*unwrapped_target_expr)) {
            ASR::ArrayItem_t *target_item = ASR::down_cast<ASR::ArrayItem_t>(unwrapped_target_expr);
            target_value_type = ASRUtils::type_get_past_array(
                ASRUtils::type_get_past_allocatable_pointer(ASRUtils::expr_type(target_item->m_v)));
        }
        ASR::ttype_t *target_array_element_type = nullptr;
        if (ASRUtils::is_array(m_target_type)) {
            target_array_element_type = ASRUtils::type_get_past_array(
                ASRUtils::type_get_past_allocatable_pointer(m_target_type));
        }
        if (is_c && target_array_element_type
                && ASR::is_a<ASR::StructType_t>(*target_array_element_type)
                && ASR::down_cast<ASR::StructType_t>(
                    target_array_element_type)->m_is_unlimited_polymorphic) {
            src = check_tmp_buffer();
            src += indent + target + " = (void*)(" + value + ");\n";
            return;
        }
        if (is_c
                && ASRUtils::is_pointer(m_target_type)
                && !ASRUtils::is_array(m_target_type)
                && !ASRUtils::is_pointer(m_value_type)
                && emits_plain_aggregate_dummy_pointee_value(x.m_value)) {
            value = "&(" + value + ")";
        }
        if (is_c && emits_plain_aggregate_dummy_pointee_value(x.m_target)) {
            target = "(*" + target + ")";
        }
        if (is_c && emits_plain_aggregate_dummy_pointee_value(x.m_value)) {
            value = "(*" + value + ")";
        }
        if( ASR::is_a<ASR::StructType_t>(*target_value_type)
                || ASRUtils::is_class_type(target_value_type) ) {
            bool target_is_pointer_backed = is_pointer_backed_struct_expr(x.m_target);
            bool value_is_pointer_backed = is_pointer_backed_struct_expr(x.m_value);
            if (emits_plain_aggregate_dummy_pointee_value(x.m_target)) {
                target_is_pointer_backed = false;
            }
            if (emits_plain_aggregate_dummy_pointee_value(x.m_value)) {
                value_is_pointer_backed = false;
            }
            if (!target_is_pointer_backed && value_is_pointer_backed) {
                value = "(*(" + value + "))";
            } else if (target_is_pointer_backed && !value_is_pointer_backed) {
                ASR::expr_t *value_expr = unwrap_c_lvalue_expr(x.m_value);
                ASR::symbol_t *value_struct_sym = ASRUtils::symbol_get_past_external(
                    ASRUtils::get_struct_sym_from_struct_expr(value_expr));
                std::string target_type_name;
                ASR::symbol_t *target_struct_sym = ASRUtils::symbol_get_past_external(
                    ASRUtils::get_struct_sym_from_struct_expr(
                        unwrap_c_lvalue_expr(x.m_target)));
                if (target_struct_sym) {
                    target_type_name = CUtils::get_c_symbol_name(
                        ASRUtils::symbol_get_past_external(target_struct_sym));
                }
                if ((ASR::is_a<ASR::StructConstructor_t>(*value_expr)
                        || ASR::is_a<ASR::StructConstant_t>(*value_expr))
                        && value_struct_sym
                        && ASR::is_a<ASR::Struct_t>(*value_struct_sym)) {
                    if (!target_type_name.empty()) {
                        value = "((struct " + target_type_name + "*)(&(" + value + ")))";
                    } else {
                        value = "&(" + value + ")";
                    }
                } else {
                    if (!target_type_name.empty()) {
                        value = "((struct " + target_type_name + "*)(&(" + value + ")))";
                    } else {
                        value = "&(" + value + ")";
                    }
                }
            }
            if (is_c && current_function &&
                    std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                    && !ASRUtils::is_array(m_target_type)) {
                headers.insert("string.h");
                src = check_tmp_buffer();
                src += indent + "memcpy(&(" + target + "), &(" + value + "), sizeof("
                    + target + "));\n";
                return;
            }
            if (is_c
                    && ASR::is_a<ASR::Allocatable_t>(*m_target_type)
                    && !ASRUtils::is_array(m_target_type)
                    && ASR::is_a<ASR::StructType_t>(*target_value_type)) {
                ASR::expr_t *target_expr = unwrap_c_lvalue_expr(x.m_target);
                ASR::expr_t *value_expr = unwrap_c_lvalue_expr(x.m_value);
                ASR::symbol_t *target_type_decl = get_expr_type_declaration_symbol(target_expr);
                std::string target_concrete_type = get_c_concrete_type_from_ttype_t(
                    target_value_type, target_type_decl);
                std::string allocation_type = target_concrete_type;
                ASR::expr_t *deepcopy_expr = target_expr;
                bool copy_runtime_type_tag = false;
                ASR::symbol_t *value_struct_sym = ASRUtils::symbol_get_past_external(
                    ASRUtils::get_struct_sym_from_struct_expr(value_expr));
                if (ASRUtils::is_class_type(target_value_type)
                        && !value_is_pointer_backed
                        && value_struct_sym
                        && ASR::is_a<ASR::Struct_t>(*value_struct_sym)) {
                    allocation_type = "struct " + CUtils::get_c_symbol_name(value_struct_sym);
                    deepcopy_expr = value_expr;
                    copy_runtime_type_tag = true;
                }
                if (!target_concrete_type.empty() && target_concrete_type != "void*") {
                    headers.insert("string.h");
                    src = check_tmp_buffer();
                    src += indent + "if (" + target + " == NULL) {\n";
                    indentation_level++;
                    std::string inner_indent(indentation_level * indentation_spaces, ' ');
                    src += inner_indent + target + " = (" + target_concrete_type
                        + "*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof("
                        + allocation_type + "));\n";
                    src += inner_indent + "memset(" + target + ", 0, sizeof(" + allocation_type + "));\n";
                    indentation_level--;
                    src += indent + "}\n";
                    src += indent + c_ds_api->get_struct_deepcopy(
                        deepcopy_expr, value, target) + "\n";
                    if (copy_runtime_type_tag) {
                        src += indent + get_runtime_type_tag_expr(target, true)
                            + " = " + get_runtime_type_tag_expr(value, true) + ";\n";
                    }
                    from_std_vector_helper.clear();
                    return;
                }
            }
            if (is_c
                    && !ASRUtils::is_allocatable(m_target_type)
                    && !ASRUtils::is_pointer(m_target_type)
                    && !ASRUtils::is_array(m_target_type)
                    && ASR::is_a<ASR::StructType_t>(*target_value_type)
                    && !ASRUtils::is_class_type(target_value_type)
                    && !ASR::down_cast<ASR::StructType_t>(
                        target_value_type)->m_is_unlimited_polymorphic) {
                ASR::expr_t *deepcopy_expr = unwrap_c_lvalue_expr(x.m_target);
                if (deepcopy_expr == nullptr) {
                    deepcopy_expr = x.m_target;
                }
                std::string deepcopy_source = target_is_pointer_backed
                    ? value : "&(" + value + ")";
                std::string deepcopy_target = target_is_pointer_backed
                    ? target : "&(" + target + ")";
                src = check_tmp_buffer();
                src += indent + c_ds_api->get_struct_deepcopy(
                    deepcopy_expr, deepcopy_source, deepcopy_target) + "\n";
                from_std_vector_helper.clear();
                return;
            }
        }
        if (is_c && ASRUtils::is_pointer(m_target_type)
                && !ASRUtils::is_array(m_target_type)
                && !ASRUtils::is_pointer(ASRUtils::expr_type(x.m_value))) {
            ASR::ttype_t *target_pointee_type = ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::type_get_past_pointer(m_target_type));
            ASR::expr_t *value_expr = unwrap_c_lvalue_expr(x.m_value);
            bool value_is_addressable_lvalue = value_expr
                && (ASR::is_a<ASR::Var_t>(*value_expr)
                    || ASR::is_a<ASR::ArrayItem_t>(*value_expr)
                    || ASR::is_a<ASR::StructInstanceMember_t>(*value_expr)
                    || ASR::is_a<ASR::UnionInstanceMember_t>(*value_expr));
            if (value_is_addressable_lvalue
                    && !is_pointer_backed_struct_expr(x.m_value)
                    && (ASR::is_a<ASR::StructType_t>(*target_pointee_type)
                        || ASRUtils::is_class_type(target_pointee_type))) {
                value = "&(" + value + ")";
            }
        }
        if( !from_std_vector_helper.empty() ) {
            src = from_std_vector_helper;
        } else {
            src.clear();
        }
        src = drain_tmp_buffer();
        if (is_c && current_function &&
                std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0) {
            headers.insert("string.h");
            if (ASRUtils::is_array(m_target_type)
                    && is_unlimited_polymorphic_storage_type(m_target_type)) {
                src += indent + target + " = (void*)(" + value + ");\n";
                return;
            }
            if (ASRUtils::is_array(m_target_type)) {
                src += indent + "memcpy(" + target + ", " + value + ", sizeof(*" + target + "));\n";
            } else {
                src += indent + "memcpy(&(" + target + "), &(" + value + "), sizeof(" + target + "));\n";
            }
            return;
        }
        if( is_target_list && is_value_list ) {
            ASR::List_t* list_target = ASR::down_cast<ASR::List_t>(ASRUtils::expr_type(x.m_target));
            std::string list_dc_func = c_ds_api->get_list_deepcopy_func(list_target);
            if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
                ASR::symbol_t *target_sym = ASR::down_cast<ASR::Var_t>(x.m_target)->m_v;
                if (ASR::is_a<ASR::Variable_t>(*target_sym)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(target_sym);
                    if (v->m_intent == ASRUtils::intent_out) {
                        src += indent + list_dc_func + "(&" + value + ", " + target + ");\n\n";
                    } else {
                        src += indent + list_dc_func + "(&" + value + ", &" + target + ");\n\n";
                    }
                }
            } else {
                src += indent + list_dc_func + "(&" + value + ", &" + target + ");\n\n";
            }
        } else if ( is_target_tup && is_value_tup ) {
            ASR::Tuple_t* tup_target = ASR::down_cast<ASR::Tuple_t>(ASRUtils::expr_type(x.m_target));
            std::string dc_func = c_ds_api->get_tuple_deepcopy_func(tup_target);
            if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
                ASR::symbol_t *target_sym = ASR::down_cast<ASR::Var_t>(x.m_target)->m_v;
                if (ASR::is_a<ASR::Variable_t>(*target_sym)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(target_sym);
                    if (v->m_intent == ASRUtils::intent_out) {
                        src += indent + dc_func + "(" + value + ", " + target + ");\n\n";
                    } else {
                        src += indent + dc_func + "(" + value + ", &" + target + ");\n\n";
                    }
                }
            } else {
                src += indent + dc_func + "(" + value + ", &" + target + ");\n\n";
            }

        } else if ( is_target_dict && is_value_dict ) {
            ASR::Dict_t* d_target = ASR::down_cast<ASR::Dict_t>(ASRUtils::expr_type(x.m_target));
            std::string dc_func = c_ds_api->get_dict_deepcopy_func(d_target);
            if (ASR::is_a<ASR::Var_t>(*x.m_target)) {
                ASR::symbol_t *target_sym = ASR::down_cast<ASR::Var_t>(x.m_target)->m_v;
                if (ASR::is_a<ASR::Variable_t>(*target_sym)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(target_sym);
                    if (v->m_intent == ASRUtils::intent_out) {
                        src += indent + dc_func + "(&" + value + ", " + target + ");\n\n";
                    } else {
                        src += indent + dc_func + "(&" + value + ", &" + target + ");\n\n";
                    }
                }
            } else {
                src += indent + dc_func + "(&" + value + ", &" + target + ");\n\n";
            }

        } else {
            if( is_c ) {
                std::string alloc = "";
                if (alloc_return_var) {
                    // char * return variable;
                     alloc = indent + target + " = NULL;\n";
                }
                ASR::ttype_t *m_target_type_unwrapped =
                    ASRUtils::type_get_past_allocatable_pointer(m_target_type);
                ASR::ttype_t *m_value_type_unwrapped =
                    ASRUtils::type_get_past_allocatable_pointer(m_value_type);
                if (value_is_string_view) {
                    src += alloc + indent
                        + "_lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &"
                        + target + ", NULL, true, true, " + value + ", "
                        + value_view_len + ");\n";
                    from_std_vector_helper.clear();
                    return;
                }
                if (!ASRUtils::is_array(m_target_type_unwrapped)
                        && ASRUtils::is_character(*m_target_type_unwrapped)
                        && is_allocating_string_expr(x.m_value)) {
                    std::string value_len, value_setup, value_cleanup;
                    materialize_allocating_string_expr(
                        x.m_value, value, value_len, value_setup, value_cleanup);
                    src += alloc + value_setup;
                    src += indent + c_ds_api->get_deepcopy(m_target_type, value, target) + "\n";
                    src += value_cleanup;
                    from_std_vector_helper.clear();
                    return;
                }
                bool cleanup_c_return_slot_after_scalar_string_copy =
                    !ASRUtils::is_array(m_target_type_unwrapped)
                    && ASRUtils::is_character(*m_target_type_unwrapped)
                    && is_c_compiler_created_return_slot_name(value);
                if( ASRUtils::is_array(m_target_type_unwrapped)
                        && ASRUtils::is_array(m_value_type_unwrapped) ) {
                    ASR::dimension_t* m_target_dims = nullptr;
                    size_t n_target_dims = ASRUtils::extract_dimensions_from_ttype(m_target_type_unwrapped, m_target_dims);
                    ASR::dimension_t* m_value_dims = nullptr;
                    size_t n_value_dims = ASRUtils::extract_dimensions_from_ttype(m_value_type_unwrapped, m_value_dims);
                    bool is_target_data_only_array = ASRUtils::is_fixed_size_array(m_target_dims, n_target_dims) &&
                                                     ASR::is_a<ASR::Struct_t>(*ASRUtils::get_asr_owner(x.m_target));
                    bool is_value_data_only_array = ASRUtils::is_fixed_size_array(m_value_dims, n_value_dims) &&
                                                    ASRUtils::get_asr_owner(x.m_value) && ASR::is_a<ASR::Struct_t>(*ASRUtils::get_asr_owner(x.m_value));
                    if( is_target_data_only_array || is_value_data_only_array ) {
                        int64_t target_size = -1, value_size = -1;
                        if( !is_target_data_only_array ) {
                            target = target + "->data";
                        } else {
                            target_size = ASRUtils::get_fixed_size_of_array(m_target_dims, n_target_dims);
                        }
                        if( !is_value_data_only_array ) {
                            value = value + "->data";
                        } else {
                            value_size = ASRUtils::get_fixed_size_of_array(m_value_dims, n_value_dims);
                        }
                        if( target_size != -1 && value_size != -1 ) {
                            LCOMPILERS_ASSERT(target_size == value_size);
                        }
                        int64_t array_size = -1;
                        if( target_size != -1 ) {
                            array_size = target_size;
                        } else {
                            array_size = value_size;
                        }
                        src += indent + "memcpy(" + target + ", " + value + ", " + std::to_string(array_size) + "*sizeof(" +
                                    CUtils::get_c_type_from_ttype_t(m_target_type) + "));\n";
                    } else {
                        src += alloc + indent + c_ds_api->get_deepcopy(m_target_type, value, target) + "\n";
                    }
                } else {
                    src += alloc + indent + c_ds_api->get_deepcopy(m_target_type, value, target) + "\n";
                    if (cleanup_c_return_slot_after_scalar_string_copy) {
                        src += indent + "if (" + value + " != NULL) {\n";
                        src += indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                            + value + ");\n";
                        src += indent + "    " + value + " = NULL;\n";
                        src += indent + "}\n";
                    }
                }
            } else {
                src += indent + c_ds_api->get_deepcopy(m_target_type, value, target) + "\n";
            }
        }
        from_std_vector_helper.clear();
    }

    std::string cmo_convertor_single_element(
        std::string arr, std::vector<std::string>& m_args,
        int n_args, bool check_for_bounds) {
        std::string dim_des_arr_ptr = arr + "->dims";
        std::string idx = "0";
        const CArrayDescriptorCache *cache =
            get_current_function_array_descriptor_cache(arr, n_args);
        for( int r = 0; r < n_args; r++ ) {
            std::string curr_llvm_idx = m_args[r];
            std::string dim_des_ptr = dim_des_arr_ptr + "[" + std::to_string(r) + "]";
            std::string lval = cache ? cache->lower_bounds[r]
                : dim_des_ptr + ".lower_bound";
            curr_llvm_idx = "(" + curr_llvm_idx + " - " + lval + ")";
            if( check_for_bounds ) {
                // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
            }
            std::string stride = cache ? cache->strides[r]
                : dim_des_ptr + ".stride";
            idx = "(" + idx + " + (" + stride + " * " + curr_llvm_idx + "))";
        }
        std::string offset_val = cache ? cache->offset : arr + "->offset";
        return "(" + idx + " + " + offset_val + ")";
    }

    std::string cmo_convertor_single_element_data_only(
        std::vector<std::string>& diminfo, std::vector<std::string>& m_args,
        int n_args, bool check_for_bounds, bool is_unbounded_pointer_to_data) {
        std::string prod = "1";
        std::string idx = "0";
        if (is_unbounded_pointer_to_data) {
            for (int r = 0, r1 = 0; r < n_args; r++, r1 += 2) {
                std::string curr_llvm_idx = m_args[r];
                std::string lval = diminfo[r1];
                curr_llvm_idx = "(" + curr_llvm_idx + " - " + lval + ")";
                if( check_for_bounds ) {
                    // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
                }
                idx = "(" + idx + " + " + "(" + prod + " * " + curr_llvm_idx + ")" + ")";
                if (r + 1 < n_args) {
                    std::string dim_size = diminfo[r1 + 1];
                    prod = "(" + prod + " * " + dim_size + ")";
                }
            }
            return idx;
        }
        for( int r = 0, r1 = 0; r < n_args; r++, r1 += 2) {
            std::string curr_llvm_idx = m_args[r];
            std::string lval = diminfo[r1];
            curr_llvm_idx = "(" + curr_llvm_idx + " - " + lval + ")";
            if( check_for_bounds ) {
                // check_single_element(curr_llvm_idx, arr); TODO: To be implemented
            }
            idx = "(" + idx + " + " + "(" + prod + " * " + curr_llvm_idx + ")" + ")";
            if (r + 1 < n_args) {
                std::string dim_size = diminfo[r1 + 1];
                prod = "(" + prod + " * " + dim_size + ")";
            }
        }
        return idx;
    }

    std::string arr_get_single_element(std::string array,
        std::vector<std::string>& m_args, int n_args, bool data_only,
        bool is_fixed_size, std::vector<std::string>& diminfo, bool is_unbounded_pointer_to_data,
        const std::string &fixed_size_data_name="") {
        std::string tmp = "";
        // TODO: Uncomment later
        // bool check_for_bounds = is_explicit_shape(v);
        bool check_for_bounds = false;
        std::string idx = "";
        if( data_only || is_fixed_size ) {
            LCOMPILERS_ASSERT(diminfo.size() > 0);
            idx = cmo_convertor_single_element_data_only(diminfo, m_args, n_args, check_for_bounds, is_unbounded_pointer_to_data);
            if (is_unbounded_pointer_to_data) {
                tmp = array + "->data[(" + array + "->offset + " + idx + ")]";
            } else if( is_fixed_size ) {
                std::string data_name = fixed_size_data_name.empty()
                    ? array + "->data" : fixed_size_data_name;
                tmp = data_name + "[" + idx + "]" ;
            } else {
                tmp = array + "->data[" + idx + "]";
            }
        } else {
            idx = cmo_convertor_single_element(array, m_args, n_args, check_for_bounds);
            const CArrayDescriptorCache *cache =
                get_current_function_array_descriptor_cache(array, n_args);
            std::string full_array = cache && !cache->data.empty()
                ? cache->data : array + "->data";
            tmp = full_array + "[" + idx + "]";
        }
        return tmp;
    }

    void fill_descriptor_for_array_section_data_only(std::string value_desc, std::string target_desc,
        std::vector<std::string>& lbs, std::vector<std::string>& ubs, std::vector<std::string>& ds, std::vector<std::string>& non_sliced_indices,
        std::vector<std::string>& diminfo, int value_rank, int target_rank) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::vector<std::string> section_first_indices;
            for( int i = 0; i < value_rank; i++ ) {
                if( ds[i] != "" ) {
                    LCOMPILERS_ASSERT(lbs[i] != "");
                    section_first_indices.push_back(lbs[i]);
                } else {
                    LCOMPILERS_ASSERT(non_sliced_indices[i] != "");
                    section_first_indices.push_back(non_sliced_indices[i]);
                }
            }
            std::string target_offset = cmo_convertor_single_element_data_only(
                diminfo, section_first_indices, value_rank, false, false);

            value_desc = "(" + value_desc + " + " + target_offset + ")";
            std::string update_target_desc = "";
            update_target_desc += indent + target_desc + "->data = " + value_desc + ";\n";

            update_target_desc += indent + target_desc + "->offset = 0;\n"; // offset not available yet
            update_target_desc += indent + target_desc + "->is_allocated = true;\n";

            std::string target_dim_des_array = target_desc + "->dims";
            int j = 0;
            std::string stride = "1";
            for( int i = 0; i < value_rank; i++ ) {
                if( ds[i] != "" ) {
                    std::string dim_length = "((( (" + ubs[i] + ") - (" + lbs[i] + ") )" + "/" + ds[i] + ") + 1)";
                    std::string target_dim_des = target_dim_des_array + "[" + std::to_string(j) + "]";
                    update_target_desc += indent + target_dim_des + ".stride = (" + stride + "*" + ds[i] + ");\n";
                    update_target_desc += indent + target_dim_des + ".lower_bound = 1;\n";
                    update_target_desc += indent + target_dim_des + ".length = " + dim_length + ";\n";
                    j++;
                }
                stride = "(" + stride + "*" + diminfo[2*i + 1] + ")";
            }
            LCOMPILERS_ASSERT(j == target_rank);
            update_target_desc += indent + target_desc + "->n_dims = " + std::to_string(target_rank) + ";\n";
            src = update_target_desc;
    }

    void fill_descriptor_for_array_section_descriptor(std::string value_desc, std::string target_desc,
        std::vector<std::string>& lbs, std::vector<std::string>& ubs, std::vector<std::string>& ds,
        std::vector<std::string>& non_sliced_indices, int value_rank, int target_rank) {
        std::string indent(indentation_level * indentation_spaces, ' ');

        std::string target_offset = value_desc + "->offset";
        for (int i = 0; i < value_rank; i++) {
            std::string first_index;
            if (ds[i] != "") {
                LCOMPILERS_ASSERT(lbs[i] != "");
                first_index = lbs[i];
            } else {
                LCOMPILERS_ASSERT(non_sliced_indices[i] != "");
                first_index = non_sliced_indices[i];
            }
            std::string source_dim = value_desc + "->dims[" + std::to_string(i) + "]";
            target_offset = "(" + target_offset + " + (" + source_dim + ".stride * (("
                + first_index + ") - " + source_dim + ".lower_bound)))";
        }

        std::string update_target_desc;
        update_target_desc += indent + target_desc + "->data = " + value_desc + "->data;\n";
        update_target_desc += indent + target_desc + "->offset = " + target_offset + ";\n";
        update_target_desc += indent + target_desc + "->is_allocated = true;\n";

        std::string target_dim_des_array = target_desc + "->dims";
        int j = 0;
        for (int i = 0; i < value_rank; i++) {
            if (ds[i] != "") {
                std::string dim_length = "((( (" + ubs[i] + ") - (" + lbs[i] + ") )" + "/" + ds[i] + ") + 1)";
                std::string source_dim = value_desc + "->dims[" + std::to_string(i) + "]";
                std::string target_dim_des = target_dim_des_array + "[" + std::to_string(j) + "]";
                update_target_desc += indent + target_dim_des + ".stride = (" + source_dim + ".stride * (" + ds[i] + "));\n";
                update_target_desc += indent + target_dim_des + ".lower_bound = 1;\n";
                update_target_desc += indent + target_dim_des + ".length = " + dim_length + ";\n";
                j++;
            }
        }
        LCOMPILERS_ASSERT(j == target_rank);
        update_target_desc += indent + target_desc + "->n_dims = " + std::to_string(target_rank) + ";\n";
        src = update_target_desc;
    }

    void handle_array_section_association_to_pointer(const ASR::Associate_t& x) {
        ASR::ArraySection_t* array_section = ASR::down_cast<ASR::ArraySection_t>(x.m_value);
        self().visit_expr(*array_section->m_v);
        std::string value_desc = src;

        self().visit_expr(*x.m_target);
        std::string target_desc = src;

        int value_rank = array_section->n_args, target_rank = 0;
        ASR::ttype_t *storage_array_type =
            get_inline_struct_member_array_storage_type(array_section->m_v);
        bool value_is_inline_component_array = storage_array_type != nullptr;
        if (storage_array_type == nullptr) {
            storage_array_type = ASRUtils::expr_type(array_section->m_v);
        }
        ASR::dimension_t *storage_dims = nullptr;
        [[maybe_unused]] int storage_rank = ASRUtils::extract_dimensions_from_ttype(
            storage_array_type, storage_dims);
        auto get_storage_dim_expr = [&](int dim, bool want_length) -> std::string {
            ASR::expr_t *dim_expr = want_length
                ? storage_dims[dim].m_length : storage_dims[dim].m_start;
            if (dim_expr == nullptr) {
                return "1";
            }
            self().visit_expr(*dim_expr);
            return src;
        };
        std::vector<std::string> lbs(value_rank);
        std::vector<std::string> ubs(value_rank);
        std::vector<std::string> ds(value_rank);
        std::vector<std::string> non_sliced_indices(value_rank);
        bool value_is_data_only_array = value_is_inline_component_array
            || is_data_only_array_expr(array_section->m_v);
        for( int i = 0; i < value_rank; i++ ) {
            lbs[i] = ""; ubs[i] = ""; ds[i] = "";
            non_sliced_indices[i] = "";
            if( array_section->m_args[i].m_step != nullptr ) {
                if (value_is_data_only_array
                        && array_section->m_args[i].m_left != nullptr
                        && ASR::is_a<ASR::ArrayBound_t>(*array_section->m_args[i].m_left)) {
                    lbs[i] = get_storage_dim_expr(i, false);
                } else {
                    self().visit_expr(*array_section->m_args[i].m_left);
                    lbs[i] = src;
                }
                self().visit_expr(*array_section->m_args[i].m_right);
                ubs[i] = src;
                self().visit_expr(*array_section->m_args[i].m_step);
                ds[i] = src;
                target_rank++;
            } else {
                self().visit_expr(*array_section->m_args[i].m_right);
                non_sliced_indices[i] = src;
            }
        }
        LCOMPILERS_ASSERT(target_rank > 0);
        if (value_is_inline_component_array) {
            std::vector<std::string> diminfo;
            diminfo.reserve(value_rank * 2);
            for (int i = 0; i < value_rank; i++) {
                diminfo.push_back(get_storage_dim_expr(i, false));
                diminfo.push_back(get_storage_dim_expr(i, true));
            }
            fill_descriptor_for_array_section_data_only(value_desc, target_desc,
                lbs, ubs, ds, non_sliced_indices, diminfo, value_rank, target_rank);
            return;
        }

        ASR::ttype_t* array_type = storage_array_type;
        ASR::array_physical_typeType phys_type = ASRUtils::extract_physical_type(array_type);
        if( phys_type == ASR::array_physical_typeType::PointerArray ||
            phys_type == ASR::array_physical_typeType::FixedSizeArray ||
            phys_type == ASR::array_physical_typeType::SIMDArray ||
            phys_type == ASR::array_physical_typeType::DescriptorArray ||
            phys_type == ASR::array_physical_typeType::UnboundedPointerArray ||
            phys_type == ASR::array_physical_typeType::ISODescriptorArray ||
            phys_type == ASR::array_physical_typeType::NumPyArray ) {
            ASR::expr_t *raw_section_base = unwrap_c_lvalue_expr(array_section->m_v);
            bool value_is_array_constant = raw_section_base != nullptr
                && ASR::is_a<ASR::ArrayConstant_t>(*raw_section_base);
            std::string value_data = ((value_is_data_only_array || value_is_inline_component_array)
                    && !value_is_array_constant)
                ? value_desc : value_desc + "->data";
            std::vector<std::string> diminfo;
            diminfo.reserve(value_rank * 2);
            if (!value_is_data_only_array && (phys_type == ASR::array_physical_typeType::PointerArray ||
                phys_type == ASR::array_physical_typeType::DescriptorArray ||
                phys_type == ASR::array_physical_typeType::UnboundedPointerArray ||
                phys_type == ASR::array_physical_typeType::ISODescriptorArray ||
                phys_type == ASR::array_physical_typeType::NumPyArray)) {
                fill_descriptor_for_array_section_descriptor(value_desc, target_desc,
                    lbs, ubs, ds, non_sliced_indices, value_rank, target_rank);
                return;
            } else {
                ASR::dimension_t* m_dims = nullptr;
                [[maybe_unused]] int array_value_rank = ASRUtils::extract_dimensions_from_ttype(array_type, m_dims);
                LCOMPILERS_ASSERT(array_value_rank == value_rank);
                auto fallback_section_length = [&](int dim) -> std::string {
                    if (!ds[dim].empty() && !lbs[dim].empty() && !ubs[dim].empty()) {
                        return "((((" + ubs[dim] + ") - (" + lbs[dim] + ")) / (" + ds[dim] + ")) + 1)";
                    }
                    return "1";
                };
                for( int i = 0; i < value_rank; i++ ) {
                    if (phys_type == ASR::array_physical_typeType::PointerArray) {
                        diminfo.push_back("1");
                        if (m_dims[i].m_length
                                && !ASR::is_a<ASR::ArraySize_t>(*m_dims[i].m_length)
                                && !ASR::is_a<ASR::ArrayBound_t>(*m_dims[i].m_length)) {
                            self().visit_expr(*m_dims[i].m_length);
                            diminfo.push_back(src);
                        } else {
                            diminfo.push_back(fallback_section_length(i));
                        }
                    } else {
                        diminfo.push_back(get_storage_dim_expr(i, false));
                        diminfo.push_back(get_storage_dim_expr(i, true));
                    }
                }
            }
            fill_descriptor_for_array_section_data_only(value_data, target_desc,
                lbs, ubs, ds, non_sliced_indices,
                diminfo, value_rank, target_rank);
        } else {
            throw CodeGenError("Only Pointer to Data Array or Fixed Size array supported for now");
        }
    }

    void handle_array_pointer_remap_association(const ASR::Associate_t& x) {
        ASR::ArraySection_t* target_section = ASR::down_cast<ASR::ArraySection_t>(x.m_target);
        self().visit_expr(*target_section->m_v);
        std::string target_desc = src;
        self().visit_expr(*x.m_value);
        std::string value_desc = src;

        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string update_target_desc;
        update_target_desc += indent + target_desc + "->data = " + value_desc + "->data;\n";
        update_target_desc += indent + target_desc + "->offset = " + value_desc + "->offset;\n";
        update_target_desc += indent + target_desc + "->is_allocated = " + value_desc + "->is_allocated;\n";

        std::string stride = "1";
        for (size_t i = 0; i < target_section->n_args; i++) {
            std::string lb = "1";
            std::string ub = "1";
            std::string step = "1";
            if (target_section->m_args[i].m_left) {
                self().visit_expr(*target_section->m_args[i].m_left);
                lb = src;
            }
            if (target_section->m_args[i].m_right) {
                self().visit_expr(*target_section->m_args[i].m_right);
                ub = src;
            }
            if (target_section->m_args[i].m_step) {
                self().visit_expr(*target_section->m_args[i].m_step);
                step = src;
            }
            std::string length = "(((" + ub + ") - (" + lb + "))/(" + step + ") + 1)";
            std::string dim_desc = target_desc + "->dims[" + std::to_string(i) + "]";
            update_target_desc += indent + dim_desc + ".lower_bound = " + lb + ";\n";
            update_target_desc += indent + dim_desc + ".length = " + length + ";\n";
            update_target_desc += indent + dim_desc + ".stride = (" + stride + ");\n";
            stride = "(" + stride + " * " + length + ")";
        }
        update_target_desc += indent + target_desc + "->n_dims = " + std::to_string(target_section->n_args) + ";\n";
        src = update_target_desc;
    }

    void visit_Associate(const ASR::Associate_t &x) {
        if (ASR::is_a<ASR::ArraySection_t>(*x.m_value)) {
            if (is_c && ASR::is_a<ASR::Var_t>(*x.m_target)
                    && ASRUtils::is_pointer(ASRUtils::expr_type(x.m_target))
                    && ASRUtils::is_array(ASRUtils::expr_type(x.m_target))) {
                ASR::symbol_t *target_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_target)->m_v);
                c_array_section_association_temps.insert(
                    get_hash(reinterpret_cast<ASR::asr_t*>(target_sym)));
            }
            ASR::ArraySection_t *array_section = ASR::down_cast<ASR::ArraySection_t>(x.m_value);
            ASR::expr_t *raw_value = unwrap_c_lvalue_expr(array_section->m_v);
            if (is_c && array_section->n_args == 1 && raw_value != nullptr
                    && ASR::is_a<ASR::StructInstanceMember_t>(*raw_value)
                    && ASRUtils::is_fixed_size_array(ASRUtils::expr_type(raw_value))) {
                self().visit_expr(*raw_value);
                std::string value_desc = src;
                self().visit_expr(*x.m_target);
                std::string target_desc = src;

                ASR::dimension_t *raw_dims = nullptr;
                [[maybe_unused]] int raw_rank = ASRUtils::extract_dimensions_from_ttype(
                    ASRUtils::expr_type(raw_value), raw_dims);
                LCOMPILERS_ASSERT(raw_rank == 1);
                auto get_dim_expr = [&](ASR::expr_t *expr, const std::string &fallback) -> std::string {
                    if (expr == nullptr) {
                        return fallback;
                    }
                    self().visit_expr(*expr);
                    return src;
                };
                std::string base_lb = get_dim_expr(raw_dims[0].m_start, "1");
                std::string base_len = get_dim_expr(raw_dims[0].m_length, "1");
                std::string section_lb;
                if (array_section->m_args[0].m_left != nullptr
                        && ASR::is_a<ASR::ArrayBound_t>(*array_section->m_args[0].m_left)) {
                    section_lb = base_lb;
                } else {
                    section_lb = get_dim_expr(array_section->m_args[0].m_left, base_lb);
                }
                std::string section_ub = get_dim_expr(
                    array_section->m_args[0].m_right,
                    "(" + base_lb + " + " + base_len + " - 1)");
                std::string section_step = get_dim_expr(array_section->m_args[0].m_step, "1");
                std::string indent(indentation_level * indentation_spaces, ' ');
                src = indent + target_desc + "->data = (" + value_desc + " + ((" + section_lb
                    + ") - (" + base_lb + ")));\n";
                src += indent + target_desc + "->offset = 0;\n";
                src += indent + target_desc + "->dims[0].stride = " + section_step + ";\n";
                src += indent + target_desc + "->dims[0].lower_bound = 1;\n";
                src += indent + target_desc + "->dims[0].length = (((((" + section_ub
                    + ") - (" + section_lb + ")) / (" + section_step + ")) + 1));\n";
                src += indent + target_desc + "->n_dims = 1;\n";
                src += indent + target_desc + "->is_allocated = true;\n";
                return;
            }
            handle_array_section_association_to_pointer(x);
        } else if (ASR::is_a<ASR::ArraySection_t>(*x.m_target)
                && ASRUtils::is_array(ASRUtils::expr_type(x.m_target))
                && ASRUtils::is_array(ASRUtils::expr_type(x.m_value))) {
            handle_array_pointer_remap_association(x);
        } else {
            std::string indent(indentation_level*indentation_spaces, ' ');
            ASR::ttype_t *target_type = ASRUtils::expr_type(x.m_target);
            ASR::ttype_t *value_type = ASRUtils::expr_type(x.m_value);
            std::string setup;
            std::string target;
            if (ASR::is_a<ASR::Var_t>(*x.m_target)
                    && ASRUtils::is_pointer(target_type)
                    && !ASRUtils::is_array(target_type)) {
                ASR::symbol_t *target_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_target)->m_v);
                ASR::Variable_t *target_var = ASR::down_cast<ASR::Variable_t>(target_sym);
                if (is_c
                        && (is_procedure_pointer_dummy_slot_type(target_var)
                            || (ASRUtils::is_arg_dummy(target_var->m_intent)
                                && !ASR::is_a<ASR::FunctionType_t>(
                                    *ASRUtils::type_get_past_pointer(target_var->m_type))))) {
                    target = "(*" + CUtils::get_c_variable_name(*target_var) + ")";
                } else {
                    target = CUtils::get_c_variable_name(*target_var);
                }
            } else {
                self().visit_expr(*x.m_target);
                target = src;
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(target);
            }
            self().visit_expr(*x.m_value);
            std::string value = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(value);
            if (ASR::is_a<ASR::Var_t>(*x.m_value)) {
                const ASR::symbol_t *value_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_value)->m_v);
                if (ASR::is_a<ASR::Function_t>(*value_sym)) {
                    value = get_emitted_function_name(*ASR::down_cast<ASR::Function_t>(
                        const_cast<ASR::symbol_t*>(value_sym)));
                }
            }
            ASR::ttype_t *target_proc_pointee_type = nullptr;
            if (ASR::is_a<ASR::Pointer_t>(*target_type)) {
                target_proc_pointee_type = ASR::down_cast<ASR::Pointer_t>(target_type)->m_type;
            } else if (ASR::is_a<ASR::FunctionType_t>(*target_type)) {
                target_proc_pointee_type = target_type;
            }
            if (is_c && target_proc_pointee_type != nullptr
                    && ASR::is_a<ASR::FunctionType_t>(*target_proc_pointee_type)) {
                src = setup + indent + target + " = " + value + ";\n";
                return;
            }
            if (ASRUtils::is_pointer(target_type)
                    && !ASRUtils::is_array(target_type)
                    && !ASRUtils::is_pointer(value_type)) {
                ASR::ttype_t *target_pointee_type = ASRUtils::type_get_past_allocatable_pointer(
                    ASRUtils::type_get_past_pointer(target_type));
                ASR::expr_t *value_expr = unwrap_c_lvalue_expr(x.m_value);
                bool value_is_addressable_lvalue = value_expr
                    && (ASR::is_a<ASR::Var_t>(*value_expr)
                        || ASR::is_a<ASR::ArrayItem_t>(*value_expr)
                        || ASR::is_a<ASR::StructInstanceMember_t>(*value_expr)
                        || ASR::is_a<ASR::UnionInstanceMember_t>(*value_expr));
                bool target_pointee_is_character = ASRUtils::is_character(*target_pointee_type);
                ASR::ttype_t *target_pointer_type = ASRUtils::type_get_past_pointer(target_type);
                bool target_pointee_is_function = target_pointer_type != nullptr
                    && ASR::is_a<ASR::FunctionType_t>(*target_pointer_type);
                bool target_pointee_is_aggregate =
                    ASRUtils::is_aggregate_type(target_pointee_type);
                if ((!target_pointee_is_character && !target_pointee_is_function
                            && !target_pointee_is_aggregate)
                        || (target_pointee_is_aggregate
                            && value_is_addressable_lvalue
                            && !is_pointer_backed_struct_expr(x.m_value))) {
                    value = "&(" + value + ")";
                }
            }
            value = cast_c_array_wrapper_ptr_to_target_type(
                target_type, value_type, value,
                get_expr_type_declaration_symbol(x.m_target),
                get_expr_type_declaration_symbol(x.m_value));
            src = setup + indent + target + " = " + value + ";\n";
        }
    }

    void visit_IntegerConstant(const ASR::IntegerConstant_t &x) {
        src = std::to_string(x.m_n);
        last_expr_precedence = 2;
    }

    void visit_UnsignedIntegerConstant(const ASR::UnsignedIntegerConstant_t &x) {
        src = std::to_string(x.m_n);
        last_expr_precedence = 2;
    }

    void visit_RealConstant(const ASR::RealConstant_t &x) {
        if (std::isnan(x.m_r)) {
            headers.insert("math.h");
            src = "((" + CUtils::get_c_type_from_ttype_t(x.m_type) + ")NAN)";
        } else if (std::isinf(x.m_r)) {
            headers.insert("math.h");
            src = "((" + CUtils::get_c_type_from_ttype_t(x.m_type) + ")"
                + std::string(std::signbit(x.m_r) ? "(-INFINITY)" : "INFINITY") + ")";
        } else {
            // TODO: remove extra spaces from the front of double_to_scientific result
            src = double_to_scientific(x.m_r);
        }
        last_expr_precedence = 2;
    }


    void visit_StringConstant(const ASR::StringConstant_t &x) {
        std::string value(x.m_s);
        int64_t fixed_len = -1;
        ASR::String_t *str_type = ASRUtils::get_string_type(x.m_type);
        if (str_type && str_type->m_len
                && ASRUtils::extract_value(str_type->m_len, fixed_len)
                && fixed_len >= 0) {
            value.assign(x.m_s, x.m_s + fixed_len);
        }
        if (is_c) {
            src = "(char*)\"" + CUtils::escape_c_string_literal(value) + "\"";
        } else {
            src = "\"" + str_escape_c(value) + "\"";
        }
        last_expr_precedence = 2;
    }

    void visit_StringConcat(const ASR::StringConcat_t& x) {
        is_string_concat_present = true;
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        if( is_c ) {
            src = "strcat_(" + left + ", " + right +")";
        } else {
            src = left + " + " + right;
        }
    }

    void visit_ListConstant(const ASR::ListConstant_t& x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string tab(indentation_spaces, ' ');
        const_name += std::to_string(const_vars_count);
        const_vars_count += 1;
        const_name = get_unique_local_name(const_name);
        std::string var_name = const_name;
        const_var_names[get_hash((ASR::asr_t*)&x)] = var_name;
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(x.m_type);
        std::string list_type_c = c_ds_api->get_list_type(t);
        std::string src_tmp = "";
        src_tmp += indent + list_type_c + " " + var_name + ";\n";
        std::string list_init_func = c_ds_api->get_list_init_func(t);
        src_tmp += indent + list_init_func + "(&" + var_name + ", " +
               std::to_string(x.n_args) + ");\n";
        for( size_t i = 0; i < x.n_args; i++ ) {
            self().visit_expr(*x.m_args[i]);
            if( ASR::is_a<ASR::String_t>(*t->m_type) ) {
                src_tmp += indent + var_name + ".data[" + std::to_string(i) +"] = NULL;\n";
            }
            src_tmp += indent + c_ds_api->get_deepcopy(t->m_type, src,
                        var_name + ".data[" + std::to_string(i) +"]") + "\n";
        }
        src_tmp += indent + var_name + ".current_end_point = " + std::to_string(x.n_args) + ";\n";
        src = var_name;
        tmp_buffer_src.push_back(src_tmp);
    }

    void visit_TupleConstant(const ASR::TupleConstant_t& x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string tab(indentation_spaces, ' ');
        const_name += std::to_string(const_vars_count);
        const_vars_count += 1;
        const_name = get_unique_local_name(const_name);
        std::string var_name = const_name;
        const_var_names[get_hash((ASR::asr_t*)&x)] = var_name;
        ASR::Tuple_t* t = ASR::down_cast<ASR::Tuple_t>(x.m_type);
        std::string tuple_type_c = c_ds_api->get_tuple_type(t);
        std::string src_tmp = "";
        src_tmp += indent + tuple_type_c + " " + var_name + ";\n";
        for (size_t i = 0; i < x.n_elements; i++) {
            self().visit_expr(*x.m_elements[i]);
            std::string ele = ".element_" + std::to_string(i);
            if (ASR::is_a<ASR::String_t>(*t->m_type[i])) {
                src_tmp += indent + var_name + ele + " = NULL;\n";
            }
            src_tmp += indent + c_ds_api->get_deepcopy(t->m_type[i], src, var_name + ele) + "\n";
        }
        src_tmp += indent + var_name + ".length" + " = " + std::to_string(x.n_elements) + ";\n";
        src = var_name;
        tmp_buffer_src.push_back(src_tmp);
    }

    void visit_DictConstant(const ASR::DictConstant_t& x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string tab(indentation_spaces, ' ');
        const_name += std::to_string(const_vars_count);
        const_vars_count += 1;
        const_name = get_unique_local_name(const_name);
        std::string var_name = const_name;
        const_var_names[get_hash((ASR::asr_t*)&x)] = var_name;
        ASR::Dict_t* t = ASR::down_cast<ASR::Dict_t>(x.m_type);
        std::string dict_type_c = c_ds_api->get_dict_type(t);
        std::string src_tmp = "";
        src_tmp += indent + dict_type_c + " " + var_name + ";\n";
        std::string dict_init_func = c_ds_api->get_dict_init_func(t);
        std::string dict_ins_func = c_ds_api->get_dict_insert_func(t);
        src_tmp += indent + dict_init_func + "(&" + var_name + ", " +
               std::to_string(x.n_keys) + " + 1);\n";
        for ( size_t i = 0; i < x.n_keys; i++ ) {
            self().visit_expr(*x.m_keys[i]);
            std::string k, v;
            k = std::move(src);
            self().visit_expr(*x.m_values[i]);
            v = std::move(src);
            src_tmp += indent + dict_ins_func + "(&" + var_name + ", " +\
                                k + ", " + v + ");\n";
        }
        src = var_name;
        tmp_buffer_src.push_back(src_tmp);
    }

    void visit_TupleCompare(const ASR::TupleCompare_t& x) {
        ASR::ttype_t* type = ASRUtils::expr_type(x.m_left);
        std::string tup_cmp_func = c_ds_api->get_compare_func(type);
        bracket_open++;
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        bracket_open--;
        std::string indent(indentation_level * indentation_spaces, ' ');
        src = tup_cmp_func + "(" + left + ", " + right + ")";
        if (x.m_op == ASR::cmpopType::NotEq) {
            src = "!" + src;
        }
        src = check_tmp_buffer() + src;
    }

    void visit_DictInsert(const ASR::DictInsert_t& x) {
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_a);
        ASR::Dict_t* t = ASR::down_cast<ASR::Dict_t>(t_ttype);
        std::string dict_insert_fun = c_ds_api->get_dict_insert_func(t);
        self().visit_expr(*x.m_a);
        std::string d_var = std::move(src);
        self().visit_expr(*x.m_key);
        std::string key = std::move(src);
        self().visit_expr(*x.m_value);
        std::string val = std::move(src);
        std::string indent(indentation_level * indentation_spaces, ' ');
        src = indent + dict_insert_fun + "(&" + d_var + ", " + key + ", " + val + ");\n";
    }

    void visit_DictItem(const ASR::DictItem_t& x) {
        ASR::Dict_t* dict_type = ASR::down_cast<ASR::Dict_t>(
                                    ASRUtils::expr_type(x.m_a));
        this->visit_expr(*x.m_a);
        std::string d_var = std::move(src);

        this->visit_expr(*x.m_key);
        std::string k = std::move(src);

        if (x.m_default) {
            this->visit_expr(*x.m_default);
            std::string def_value = std::move(src);
            std::string dict_get_fun = c_ds_api->get_dict_get_func(dict_type,
                                                                    true);
            src = dict_get_fun + "(&" + d_var + ", " + k + ", " + def_value + ")";
        } else {
            std::string dict_get_fun = c_ds_api->get_dict_get_func(dict_type);
            src = dict_get_fun + "(&" + d_var + ", " + k + ")";
        }
    }

    void visit_ListAppend(const ASR::ListAppend_t& x) {
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_a);
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(t_ttype);
        std::string list_append_func = c_ds_api->get_list_append_func(t);
        bracket_open++;
        self().visit_expr(*x.m_a);
        std::string list_var = std::move(src);
        self().visit_expr(*x.m_ele);
        std::string element = std::move(src);
        bracket_open--;
        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer();
        src += indent + list_append_func + "(&" + list_var + ", " + element + ");\n";
    }

    void visit_ListConcat(const ASR::ListConcat_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(x.m_type);
        std::string list_concat_func = c_ds_api->get_list_concat_func(t);
        bracket_open++;
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        self().visit_expr(*x.m_right);
        bracket_open--;
        std::string rig = std::move(src);
        tmp_buffer_src.push_back(check_tmp_buffer());
        src = "(*" + list_concat_func + "(&" + left + ", &" + rig + "))";
    }

    void visit_ListSection(const ASR::ListSection_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        std::string left, right, step, l_present, r_present;
        bracket_open++;
        if (x.m_section.m_left) {
            self().visit_expr(*x.m_section.m_left);
            left = src;
            l_present = "true";
        } else {
            left = "0";
            l_present = "false";
        }
        if (x.m_section.m_right) {
            self().visit_expr(*x.m_section.m_right);
            right = src;
            r_present = "true";
        } else {
            right = "0";
            r_present = "false";
        }
        if (x.m_section.m_step) {
            self().visit_expr(*x.m_section.m_step);
            step = src;
        } else {
            step = "1";
        }
        self().visit_expr(*x.m_a);
        bracket_open--;
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_a);
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(t_ttype);
        std::string list_var = std::move(src);
        std::string list_type_c = c_ds_api->get_list_type(t);
        std::string list_section_func = c_ds_api->get_list_section_func(t);
        std::string indent(indentation_level * indentation_spaces, ' ');
        const_name += std::to_string(const_vars_count);
        const_vars_count += 1;
        const_name = get_unique_local_name(const_name);
        std::string var_name = const_name, tmp_src_gen = "";
        tmp_src_gen = indent + list_type_c + "* " + var_name + " = ";
        tmp_src_gen += list_section_func + "(&" + list_var + ", " + left + ", " +
            right + ", " + step + ", " + l_present + ", " + r_present + ");\n";
        const_var_names[get_hash((ASR::asr_t*)&x)] = var_name;
        tmp_buffer_src.push_back(tmp_src_gen);
        src = "(* " + var_name + ")";
    }

    void visit_ListClear(const ASR::ListClear_t& x) {
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_a);
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(t_ttype);
        std::string list_clear_func = c_ds_api->get_list_clear_func(t);
        bracket_open++;
        self().visit_expr(*x.m_a);
        bracket_open--;
        std::string list_var = std::move(src);
        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer() + indent + list_clear_func + "(&" + list_var + ");\n";
    }

    void visit_ListRepeat(const ASR::ListRepeat_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(x.m_type);
        std::string list_repeat_func = c_ds_api->get_list_repeat_func(t);
        bracket_open++;
        self().visit_expr(*x.m_left);
        std::string list_var = std::move(src);
        self().visit_expr(*x.m_right);
        std::string freq = std::move(src);
        bracket_open--;
        tmp_buffer_src.push_back(check_tmp_buffer());
        src = "(*" + list_repeat_func + "(&" + list_var + ", " + freq + "))";
    }

    void visit_ListCompare(const ASR::ListCompare_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        ASR::ttype_t* type = ASRUtils::expr_type(x.m_left);
        std::string list_cmp_func = c_ds_api->get_compare_func(type);
        bracket_open++;
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        self().visit_expr(*x.m_right);
        bracket_open--;
        std::string right = std::move(src), tmp_gen= "";
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string val = list_cmp_func + "(" + left + ", " + right + ")";
        if (x.m_op == ASR::cmpopType::NotEq) {
            val = "!" + val;
        }
        src = check_tmp_buffer() + val;
    }

    void visit_ListInsert(const ASR::ListInsert_t& x) {
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_a);
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(t_ttype);
        std::string list_insert_func = c_ds_api->get_list_insert_func(t);
        bracket_open++;
        self().visit_expr(*x.m_a);
        std::string list_var = std::move(src);
        self().visit_expr(*x.m_ele);
        std::string element = std::move(src);
        self().visit_expr(*x.m_pos);
        bracket_open--;
        std::string pos = std::move(src);
        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer();
        src += indent + list_insert_func + "(&" + list_var + ", " + pos + ", " + element + ");\n";
    }

    void visit_ListRemove(const ASR::ListRemove_t& x) {
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_a);
        ASR::List_t* t = ASR::down_cast<ASR::List_t>(t_ttype);
        std::string list_remove_func = c_ds_api->get_list_remove_func(t);
        bracket_open++;
        self().visit_expr(*x.m_a);
        std::string list_var = std::move(src);
        self().visit_expr(*x.m_ele);
        bracket_open--;
        std::string element = std::move(src);
        std::string indent(indentation_level * indentation_spaces, ' ');
        src = check_tmp_buffer();
        src += indent + list_remove_func + "(&" + list_var + ", " + element + ");\n";
    }

    void visit_ListLen(const ASR::ListLen_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        src = src + ".current_end_point";
    }

    void visit_TupleLen(const ASR::TupleLen_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        src = src + ".length";
    }

    void visit_DictLen(const ASR::DictLen_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_arg);
        ASR::Dict_t* t = ASR::down_cast<ASR::Dict_t>(t_ttype);
        std::string dict_len_fun = c_ds_api->get_dict_len_func(t);
        bracket_open++;
        self().visit_expr(*x.m_arg);
        src = dict_len_fun + "(&" + src + ")";
        bracket_open--;
    }

    void visit_DictPop(const ASR::DictPop_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        ASR::ttype_t* t_ttype = ASRUtils::expr_type(x.m_a);
        ASR::Dict_t* t = ASR::down_cast<ASR::Dict_t>(t_ttype);
        std::string dict_pop_fun = c_ds_api->get_dict_pop_func(t);
        bracket_open++;
        self().visit_expr(*x.m_a);
        std::string d = std::move(src);
        self().visit_expr(*x.m_key);
        std::string k = std::move(src);
        src = dict_pop_fun + "(&" + d + ", "  + k + ")";
        bracket_open--;
    }

    void visit_ListItem(const ASR::ListItem_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_a);
        std::string list_var = std::move(src);
        self().visit_expr(*x.m_pos);
        std::string pos = std::move(src);
        // TODO: check for out of bound indices
        src = list_var + ".data[" + pos + "]";
    }

    void visit_TupleItem(const ASR::TupleItem_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_a);
        std::string tup_var = std::move(src);
        ASR::expr_t *pos_val = ASRUtils::expr_value(x.m_pos);
        if (pos_val == nullptr) {
            throw CodeGenError("Compile time constant values are supported in Tuple Item yet");
        }
        self().visit_expr(*pos_val);
        std::string pos = std::move(src);
        // TODO: check for out of bound indices
        src = tup_var + ".element_" + pos;
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        if (x.m_value == true) {
            src = "true";
        } else {
            src = "false";
        }
        last_expr_precedence = 2;
    }

    void visit_Var(const ASR::Var_t &x) {
        const ASR::symbol_t *s = ASRUtils::symbol_get_past_external(x.m_v);
        if (ASR::is_a<ASR::Function_t>(*s)) {
            record_forward_decl_for_function(*ASR::down_cast<ASR::Function_t>(
                const_cast<ASR::symbol_t*>(s)));
            src = get_emitted_function_name(*ASR::down_cast<ASR::Function_t>(
                const_cast<ASR::symbol_t*>(s)));
            return;
        }
        ASR::Variable_t* sv = ASR::down_cast<ASR::Variable_t>(s);
        ASR::asr_t *owner = sv->m_parent_symtab ? sv->m_parent_symtab->asr_owner : nullptr;
        if (owner && CUtils::is_symbol_owner<ASR::Enum_t>(owner)) {
            src = CUtils::get_c_enum_member_name(*sv);
            last_expr_precedence = 2;
            return;
        }
        std::string var_name = get_c_var_storage_name(sv);
        if (is_c) {
            if (current_function
                    && std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                    && var_name == "to"
                    && !ASRUtils::is_array(sv->m_type)) {
                src = "(*((void**)" + var_name + "))";
            } else if (force_storage_expr_in_call_args
                && ASRUtils::is_pointer(sv->m_type)
                && !ASRUtils::is_array(sv->m_type)
                && !ASR::is_a<ASR::FunctionType_t>(
                    *ASRUtils::type_get_past_pointer(sv->m_type))) {
                src = var_name;
            } else
            if (is_bindc_optional_scalar_dummy(sv)) {
                src = "(*" + var_name + ")";
            } else if (is_procedure_pointer_dummy_slot_type(sv)) {
                src = "(*" + var_name + ")";
            } else if (is_pointer_dummy_slot_type(sv)) {
                ASR::ttype_t *ptr_target = ASRUtils::type_get_past_pointer(sv->m_type);
                if (ptr_target != nullptr
                        && (ASRUtils::is_integer(*ptr_target)
                            || ASRUtils::is_unsigned_integer(*ptr_target)
                            || ASRUtils::is_real(*ptr_target)
                            || ASRUtils::is_logical(*ptr_target)
                            || ASRUtils::is_complex(*ptr_target))) {
                    src = "(*(*" + var_name + "))";
                } else {
                    src = "(*" + var_name + ")";
                }
            } else
            if (owner && CUtils::is_symbol_owner<ASR::AssociateBlock_t>(owner)
                && ASRUtils::is_pointer(sv->m_type)
                && !ASRUtils::is_array(sv->m_type)) {
                ASR::ttype_t *ptr_target = ASR::down_cast<ASR::Pointer_t>(sv->m_type)->m_type;
                if (!ASRUtils::is_aggregate_type(ptr_target)
                        && !ASRUtils::is_character(*ASRUtils::type_get_past_pointer(sv->m_type))) {
                    src = "(*" + var_name + ")";
                } else {
                    src = var_name;
                }
            } else if (ASRUtils::is_arg_dummy(sv->m_intent)
                && ASRUtils::is_pointer(sv->m_type)
                && !ASRUtils::is_array(sv->m_type)) {
                if (!ASR::is_a<ASR::FunctionType_t>(*ASRUtils::type_get_past_pointer(sv->m_type))) {
                    src = "(*" + var_name + ")";
                } else {
                    src = var_name;
                }
            } else if ((sv->m_intent == ASRUtils::intent_in
                || sv->m_intent == ASRUtils::intent_inout)
                && ASRUtils::is_array(sv->m_type)
                && ASRUtils::is_pointer(sv->m_type)) {
                src = var_name;
            } else if ((sv->m_intent == ASRUtils::intent_inout
                || sv->m_intent == ASRUtils::intent_out)
                && ASRUtils::is_array(sv->m_type)) {
                src = var_name;
            } else if ((sv->m_intent == ASRUtils::intent_inout
                || sv->m_intent == ASRUtils::intent_out)
                && ASR::is_a<ASR::CPtr_t>(*sv->m_type)
                && !ASRUtils::is_array(sv->m_type)) {
                src = "(*((void**)" + var_name + "))";
            } else if ((sv->m_intent == ASRUtils::intent_inout
                || sv->m_intent == ASRUtils::intent_out)
                && ASRUtils::is_character(*ASRUtils::type_get_past_allocatable_pointer(sv->m_type))
                && !ASRUtils::is_array(sv->m_type)) {
                src = "(*" + var_name + ")";
            } else if (is_unlimited_polymorphic_dummy_slot_type(sv)) {
                src = "(*((void**)" + var_name + "))";
            } else if (is_aggregate_dummy_slot_type(sv)) {
                src = "(*" + var_name + ")";
            } else if ((sv->m_intent == ASRUtils::intent_inout
                || sv->m_intent == ASRUtils::intent_out)
                && !ASRUtils::is_aggregate_type(sv->m_type)) {
                src = "(*" + var_name + ")";
            } else if (ASRUtils::is_pointer(sv->m_type)
                && !ASRUtils::is_array(sv->m_type)) {
                ASR::ttype_t *ptr_target = ASR::down_cast<ASR::Pointer_t>(sv->m_type)->m_type;
                if (!ASRUtils::is_aggregate_type(ptr_target)
                        && !ASRUtils::is_character(*ASRUtils::type_get_past_pointer(sv->m_type))
                        && !ASR::is_a<ASR::FunctionType_t>(*ASRUtils::type_get_past_pointer(sv->m_type))) {
                    src = "(*" + var_name + ")";
                } else if (ASR::is_a<ASR::FunctionType_t>(*ASRUtils::type_get_past_pointer(sv->m_type))) {
                    src = var_name;
                } else {
                    src = var_name;
                }
            } else if (is_scalar_allocatable_storage_type(sv->m_type)) {
                src = force_storage_expr_in_call_args
                    ? var_name
                    : "(*" + var_name + ")";
            } else {
                src = var_name;
            }
        } else {
            src = var_name;
        }
        last_expr_precedence = 2;
    }

    void visit_StructInstanceMember(const ASR::StructInstanceMember_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        src = get_struct_instance_member_expr(x, true);
    }

    void visit_UnionInstanceMember(const ASR::UnionInstanceMember_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        std::string der_expr, member;
        this->visit_expr(*x.m_v);
        der_expr = std::move(src);
        member = CUtils::get_c_member_name(x.m_m);
        src = der_expr + "." + member;
    }

    void visit_Cast(const ASR::Cast_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        switch (x.m_kind) {
            case (ASR::cast_kindType::IntegerToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: src = "(float)(" + src + ")"; break;
                    case 8: src = "(double)(" + src + ")"; break;
                    default: throw CodeGenError("Cast IntegerToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                src = "(int" + std::to_string(dest_kind * 8) + "_t)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToReal) : {
                // In C++, we do not need to cast float to float explicitly:
                // src = src;
                break;
            }
            case (ASR::cast_kindType::IntegerToInteger) :
            case (ASR::cast_kindType::UnsignedIntegerToUnsignedInteger) : {
                // In C++, we do not need to cast int <-> long long explicitly:
                // we also do not need to cast uint8_t <-> uint32_t explicitly:
                // src = src;
                break;
            }
            case (ASR::cast_kindType::IntegerToUnsignedInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                src = "(uint" + std::to_string(dest_kind * 8) + "_t)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToUnsignedInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                src = "(uint" + std::to_string(dest_kind * 8) + "_t)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::UnsignedIntegerToInteger) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                src = "(int" + std::to_string(dest_kind * 8) + "_t)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::UnsignedIntegerToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: src = "(float)(" + src + ")"; break;
                    case 8: src = "(double)(" + src + ")"; break;
                    default: throw CodeGenError("Cast IntegerToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::ComplexToComplex) : {
                break;
            }
            case (ASR::cast_kindType::IntegerToComplex) : {
                if (is_c) {
                    headers.insert("complex.h");
                    src = "CMPLX(" + src + ", 0)";
                } else {
                    src = "std::complex<double>(" + src + ")";
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::ComplexToReal) : {
                if (is_c) {
                    headers.insert("complex.h");
                    src = "creal(" + src + ")";
                } else {
                    src = "std::real(" + src + ")";
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToComplex) : {
                if (is_c) {
                    headers.insert("complex.h");
                    src = "CMPLX(" + src + ", 0.0)";
                } else {
                    src = "std::complex<double>(" + src + ")";
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::LogicalToInteger) : {
                src = "(int)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::LogicalToLogical) : {
                // No conversion needed for logical-to-logical in C
                break;
            }
            case (ASR::cast_kindType::LogicalToString) : {
                src = "(" + src + " ? \"True\" : \"False\")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::IntegerToLogical) :
            case (ASR::cast_kindType::UnsignedIntegerToLogical) : {
                src = "(bool)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::LogicalToReal) : {
                int dest_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
                switch (dest_kind) {
                    case 4: src = "(float)(" + src + ")"; break;
                    case 8: src = "(double)(" + src + ")"; break;
                    default: throw CodeGenError("Cast LogicalToReal: Unsupported Kind " + std::to_string(dest_kind));
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::RealToLogical) : {
                src = "(bool)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::StringToLogical) : {
                src = "(bool)(strlen(" + src + ") > 0)";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::ComplexToLogical) : {
                src = "(bool)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::IntegerToString) : {
                if (is_c) {
                    ASR::ttype_t *arg_type = ASRUtils::expr_type(x.m_arg);
                    int arg_kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
                    switch (arg_kind) {
                        case 1: src = "_lfortran_int_to_str1_alloc(_lfortran_get_default_allocator(), " + src + ")"; break;
                        case 2: src = "_lfortran_int_to_str2_alloc(_lfortran_get_default_allocator(), " + src + ")"; break;
                        case 4: src = "_lfortran_int_to_str4_alloc(_lfortran_get_default_allocator(), " + src + ")"; break;
                        case 8: src = "_lfortran_int_to_str8_alloc(_lfortran_get_default_allocator(), " + src + ")"; break;
                        default: throw CodeGenError("Cast IntegerToString: Unsupported Kind " + \
                                        std::to_string(arg_kind));
                    }

                } else {
                    src = "std::to_string(" + src + ")";
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::StringToInteger) : {
                if (is_c) {
                    src = "atoi(" + src + ")";
                } else {
                    src = "std::stoi(" + src + ")";
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::StringToArray) : {
                // The C backend already represents scalar strings as `char*`.
                // Treating this cast as a no-op keeps string-backed dummy
                // arguments moving through the existing runtime helpers.
                break;
            }
            case (ASR::cast_kindType::RealToString) : {
                if (is_c) {
                    ASR::ttype_t *arg_type = ASRUtils::expr_type(x.m_arg);
                    int arg_kind = ASRUtils::extract_kind_from_ttype_t(arg_type);
                    switch (arg_kind) {
                        case 4: src = "_lfortran_float_to_str4_alloc(_lfortran_get_default_allocator(), " + src + ")"; break;
                        case 8: src = "_lfortran_float_to_str8_alloc(_lfortran_get_default_allocator(), " + src + ")"; break;
                        default: throw CodeGenError("Cast RealToString: Unsupported Kind " + \
                                        std::to_string(arg_kind));
                    }
                } else {
                    src = "std::to_string(" + src + ")";
                }
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::CPtrToUnsignedInteger) : {
                src = "(uint64_t)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::UnsignedIntegerToCPtr) : {
                src = "(void*)(" + src + ")";
                last_expr_precedence = 2;
                break;
            }
            case (ASR::cast_kindType::ClassToStruct):
            case (ASR::cast_kindType::ClassToClass): {
                ASR::expr_t *dest_expr = x.m_dest
                    ? x.m_dest
                    : const_cast<ASR::expr_t*>(reinterpret_cast<const ASR::expr_t*>(&x.base));
                ASR::symbol_t *struct_sym =
                    ASRUtils::get_struct_sym_from_struct_expr(dest_expr);
                if (struct_sym != nullptr) {
                    struct_sym = ASRUtils::symbol_get_past_external(struct_sym);
                }
                if (struct_sym == nullptr || !ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                    throw CodeGenError("Class cast target type could not be resolved in C backend",
                        x.base.base.loc);
                }
                std::string cast_arg = src;
                if (emits_plain_aggregate_dummy_pointee_value(x.m_arg)) {
                    cast_arg = canonicalize_raw_pointer_actual_src(cast_arg);
                }
                src = "((struct " + CUtils::get_c_symbol_name(struct_sym) + "*)(" + cast_arg + "))";
                last_expr_precedence = 2;
                break;
            }
            default : throw CodeGenError("Cast kind " + std::to_string(x.m_kind) + " not implemented",
                x.base.base.loc);
        }
    }

    void visit_IntegerBitLen(const ASR::IntegerBitLen_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_a);
        int arg_kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        switch (arg_kind) {
            case 1: src = "_lpython_bit_length1(" + src + ")"; break;
            case 2: src = "_lpython_bit_length2(" + src + ")"; break;
            case 4: src = "_lpython_bit_length4(" + src + ")"; break;
            case 8: src = "_lpython_bit_length8(" + src + ")"; break;
            default: throw CodeGenError("Unsupported Integer Kind: " + \
                            std::to_string(arg_kind));
        }
    }

    void visit_Ichar(const ASR::Ichar_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        if (x.m_value) {
            self().visit_expr(*x.m_value);
            return;
        }
        self().visit_expr(*x.m_arg);
        if (is_c && ASR::is_a<ASR::ArrayItem_t>(*x.m_arg)) {
            ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(x.m_arg);
            if (CUtils::is_len_one_character_array_type(ASRUtils::expr_type(item->m_v))) {
                src = "((int32_t)(unsigned char)(" + src + "))";
                return;
            }
        }
        src = "_lfortran_ichar(" + src + ")";
    }

    void visit_Iachar(const ASR::Iachar_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        if (x.m_value) {
            self().visit_expr(*x.m_value);
            return;
        }
        self().visit_expr(*x.m_arg);
        if (is_c && ASR::is_a<ASR::ArrayItem_t>(*x.m_arg)) {
            ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(x.m_arg);
            if (CUtils::is_len_one_character_array_type(ASRUtils::expr_type(item->m_v))) {
                src = "((int32_t)(unsigned char)(" + src + "))";
                return;
            }
        }
        src = "_lfortran_iachar(" + src + ")";
    }

    void visit_ArrayIsContiguous(const ASR::ArrayIsContiguous_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        if (x.m_value) {
            self().visit_expr(*x.m_value);
            return;
        }
        ASR::ttype_t *array_type = ASRUtils::expr_type(x.m_array);
        ASR::array_physical_typeType physical_type = ASRUtils::extract_physical_type(array_type);
        self().visit_expr(*x.m_array);
        std::string array_expr = src;
        switch (physical_type) {
            case ASR::array_physical_typeType::DescriptorArray: {
                ASR::dimension_t *m_dims = nullptr;
                int n_dims = ASRUtils::extract_dimensions_from_ttype(array_type, m_dims);
                std::string is_contiguous = "true";
                std::string expected_stride = "1";
                for (int i = 0; i < n_dims; i++) {
                    is_contiguous += " && (" + array_expr + "->dims[" + std::to_string(i)
                        + "].stride == " + expected_stride + ")";
                    expected_stride = "(" + expected_stride + " * " + array_expr
                        + "->dims[" + std::to_string(i) + "].length)";
                }
                src = "(" + array_expr + "->is_allocated && (" + is_contiguous + "))";
                return;
            }
            case ASR::array_physical_typeType::FixedSizeArray:
            case ASR::array_physical_typeType::SIMDArray:
            case ASR::array_physical_typeType::PointerArray:
            case ASR::array_physical_typeType::UnboundedPointerArray:
                src = "true";
                return;
            case ASR::array_physical_typeType::StringArraySinglePointer:
                src = "false";
                return;
            default:
                throw CodeGenError("ArrayIsContiguous is not implemented for physical type " +
                    std::to_string(static_cast<int>(physical_type)), x.base.base.loc);
        }
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t &x) {
        handle_Compare(x);
    }

    void visit_UnsignedIntegerCompare(const ASR::UnsignedIntegerCompare_t &x) {
        handle_Compare(x);
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
        handle_Compare(x);
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t &x) {
        handle_Compare(x);
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
        handle_Compare(x);
    }

    void visit_StringCompare(const ASR::StringCompare_t &x) {
        handle_Compare(x);
    }

    void visit_CPtrCompare(const ASR::CPtrCompare_t &x) {
        handle_Compare(x);
    }

    template<typename T>
    void handle_Compare(const T &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        if (is_c
                && T::class_type != ASR::exprType::StringCompare
                && ASRUtils::is_array(x.m_type)
                && ASRUtils::extract_n_dims_from_ttype(x.m_type) == 1) {
            self().visit_expr(*x.m_left);
            std::string left = std::move(src);
            std::string left_setup = drain_tmp_buffer();
            self().visit_expr(*x.m_right);
            std::string right = std::move(src);
            std::string right_setup = drain_tmp_buffer();

            ASR::ttype_t *left_type = ASRUtils::expr_type(x.m_left);
            ASR::ttype_t *right_type = ASRUtils::expr_type(x.m_right);
            bool left_is_array = ASRUtils::is_array(left_type);
            bool right_is_array = ASRUtils::is_array(right_type);
            std::string ref_array = left_is_array ? left : right;
            std::string result_type = get_c_declared_array_wrapper_type_name(x.m_type);
            std::string result_value_name =
                get_unique_local_name("__libasr_created__compare_result_value");
            std::string result_name = result_value_name + "__ptr";
            std::string index_name =
                get_unique_local_name("__libasr_created__compare_result_index");
            std::string left_index_name =
                get_unique_local_name("__libasr_created__compare_left_index");
            std::string right_index_name =
                get_unique_local_name("__libasr_created__compare_right_index");
            std::string op_str = ASRUtils::cmpop_to_str(x.m_op);
            std::string compare_cache_key;
            if (reuse_array_compare_temps_in_call_args) {
                compare_cache_key = left + "\n" + op_str + "\n" + right + "\n"
                    + std::to_string(static_cast<int>(T::class_type));
                auto it = array_compare_temp_cache.find(compare_cache_key);
                if (it != array_compare_temp_cache.end()) {
                    src = it->second;
                    last_expr_precedence = 2;
                    return;
                }
            }
            auto current_indent = [&]() {
                return std::string(indentation_level * indentation_spaces, ' ');
            };
            auto get_array_value = [&](const std::string &array_src,
                                       const std::string &idx_name) -> std::string {
                return array_src + "->data[((0 + (" + array_src + "->dims[0].stride * ("
                    + idx_name + " - " + array_src + "->dims[0].lower_bound))) + "
                    + array_src + "->offset)]";
            };

            std::string compare_setup = left_setup + right_setup;
            compare_setup += current_indent() + result_type + " "
                + result_value_name + ";\n";
            compare_setup += current_indent() + result_type + "* " + result_name + " = &"
                + result_value_name + ";\n";
            compare_setup += current_indent() + result_name + "->n_dims = 1;\n";
            compare_setup += current_indent() + result_name + "->offset = 0;\n";
            compare_setup += current_indent() + result_name + "->is_allocated = true;\n";
            compare_setup += current_indent() + result_name + "->dims[0].lower_bound = "
                + ref_array + "->dims[0].lower_bound;\n";
            compare_setup += current_indent() + result_name + "->dims[0].length = "
                + ref_array + "->dims[0].length;\n";
            compare_setup += current_indent() + result_name + "->dims[0].stride = 1;\n";
            compare_setup += current_indent() + result_name
                + "->data = (bool*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), "
                + result_name + "->dims[0].length*sizeof(bool));\n";
            if (left_is_array) {
                compare_setup += current_indent() + "int32_t " + left_index_name + " = "
                    + left + "->dims[0].lower_bound;\n";
            }
            if (right_is_array) {
                compare_setup += current_indent() + "int32_t " + right_index_name + " = "
                    + right + "->dims[0].lower_bound;\n";
            }
            compare_setup += current_indent() + "for (int32_t " + index_name + " = "
                + result_name + "->dims[0].lower_bound; " + index_name + " <= "
                + result_name + "->dims[0].length + " + result_name
                + "->dims[0].lower_bound - 1; " + index_name + "++) {\n";
            std::string left_value = left_is_array ? get_array_value(left, left_index_name) : left;
            std::string right_value = right_is_array ? get_array_value(right, right_index_name) : right;
            compare_setup += current_indent() + std::string(indentation_spaces, ' ')
                + result_name + "->data[((0 + (" + result_name
                + "->dims[0].stride * (" + index_name + " - " + result_name
                + "->dims[0].lower_bound))) + " + result_name + "->offset)] = "
                + left_value + " " + op_str + " " + right_value + ";\n";
            if (left_is_array) {
                compare_setup += current_indent() + std::string(indentation_spaces, ' ')
                    + left_index_name + " += 1;\n";
            }
            if (right_is_array) {
                compare_setup += current_indent() + std::string(indentation_spaces, ' ')
                    + right_index_name + " += 1;\n";
            }
            compare_setup += current_indent() + "}\n";
            tmp_buffer_src.push_back(compare_setup);
            src = result_name;
            if (reuse_array_compare_temps_in_call_args) {
                array_compare_temp_cache[compare_cache_key] = result_name;
            }
            last_expr_precedence = 2;
            return;
        }
        if (is_c && T::class_type == ASR::exprType::StringCompare) {
            std::string left, right, left_len, right_len, compare_setup;
            if (!try_get_unit_step_string_section_view(
                    x.m_left, left, left_len, compare_setup)) {
                self().visit_expr(*x.m_left);
                left = std::move(src);
                compare_setup += drain_tmp_buffer();
                compare_setup += extract_stmt_setup_from_expr(left);
                left_len = get_string_length_expr(x.m_left, left, compare_setup);
            }
            if (!try_get_unit_step_string_section_view(
                    x.m_right, right, right_len, compare_setup)) {
                self().visit_expr(*x.m_right);
                right = std::move(src);
                compare_setup += drain_tmp_buffer();
                compare_setup += extract_stmt_setup_from_expr(right);
                right_len = get_string_length_expr(x.m_right, right, compare_setup);
            }
            if (!compare_setup.empty()) {
                tmp_buffer_src.push_back(compare_setup);
            }
            std::string op_str = ASRUtils::cmpop_to_str(x.m_op);
            src = "str_compare(" + left + ", " + left_len + ", "
                + right + ", " + right_len + ") " + op_str + " 0";
            switch (x.m_op) {
                case (ASR::cmpopType::Eq) : { last_expr_precedence = 10; break; }
                case (ASR::cmpopType::Gt) : { last_expr_precedence = 9;  break; }
                case (ASR::cmpopType::GtE) : { last_expr_precedence = 9; break; }
                case (ASR::cmpopType::Lt) : { last_expr_precedence = 9;  break; }
                case (ASR::cmpopType::LtE) : { last_expr_precedence = 9; break; }
                case (ASR::cmpopType::NotEq): { last_expr_precedence = 10; break; }
                default : LCOMPILERS_ASSERT(false);
            }
            return;
        }
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        auto maybe_raw_pointer_for_null_compare = [&](ASR::expr_t *expr,
                                                      ASR::expr_t *other,
                                                      std::string &expr_src) {
            if (!is_c || other == nullptr || !ASR::is_a<ASR::PointerNullConstant_t>(*other)) {
                return;
            }
            ASR::expr_t *unwrapped_expr = unwrap_c_lvalue_expr(expr);
            if (unwrapped_expr == nullptr || !ASR::is_a<ASR::Var_t>(*unwrapped_expr)) {
                return;
            }
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(unwrapped_expr)->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                return;
            }
            ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
            if (ASRUtils::is_pointer(var->m_type)
                    && !ASRUtils::is_array(var->m_type)
                    && (var->m_intent == ASRUtils::intent_local
                        || var->m_intent == ASRUtils::intent_return_var)) {
                expr_src = CUtils::get_c_variable_name(*var);
            }
        };
        maybe_raw_pointer_for_null_compare(x.m_left, x.m_right, left);
        maybe_raw_pointer_for_null_compare(x.m_right, x.m_left, right);
        switch (x.m_op) {
            case (ASR::cmpopType::Eq) : { last_expr_precedence = 10; break; }
            case (ASR::cmpopType::Gt) : { last_expr_precedence = 9;  break; }
            case (ASR::cmpopType::GtE) : { last_expr_precedence = 9; break; }
            case (ASR::cmpopType::Lt) : { last_expr_precedence = 9;  break; }
            case (ASR::cmpopType::LtE) : { last_expr_precedence = 9; break; }
            case (ASR::cmpopType::NotEq): { last_expr_precedence = 10; break; }
            default : LCOMPILERS_ASSERT(false); // should never happen
        }
        if (left_precedence < last_expr_precedence) {
            src += left;
        } else {
            src += "(" + left + ")";
        }
        std::string op_str = ASRUtils::cmpop_to_str(x.m_op);
        if( T::class_type == ASR::exprType::StringCompare && is_c ) {
            auto get_string_length = [&](ASR::expr_t *expr, const std::string &expr_src) -> std::string {
                ASR::String_t *str_type = ASRUtils::get_string_type(expr);
                if (str_type && str_type->m_len) {
                    self().visit_expr(*str_type->m_len);
                    return src;
                }
                return "strlen(" + expr_src + ")";
            };
            std::string left_len = get_string_length(x.m_left, left);
            std::string right_len = get_string_length(x.m_right, right);
            src = "str_compare(" + left + ", " + left_len + ", "
                + right + ", " + right_len + ") " + op_str + " 0";
        } else {
            src += op_str;
            if (right_precedence < last_expr_precedence) {
                src += right;
            } else {
                src += "(" + right + ")";
            }
        }
    }

    template<typename T>
    void handle_SU_IntegerBitNot(const T& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        int expr_precedence = last_expr_precedence;
        last_expr_precedence = 3;
        if (expr_precedence <= last_expr_precedence) {
            src = "~" + src;
        } else {
            src = "~(" + src + ")";
        }
    }

    void visit_IntegerBitNot(const ASR::IntegerBitNot_t& x) {
        handle_SU_IntegerBitNot(x);
    }

    void visit_UnsignedIntegerBitNot(const ASR::UnsignedIntegerBitNot_t& x) {
        handle_SU_IntegerBitNot(x);
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        handle_UnaryMinus(x);
    }

    void visit_UnsignedIntegerUnaryMinus(const ASR::UnsignedIntegerUnaryMinus_t &x) {
        handle_UnaryMinus(x);
        int kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(x.m_arg));
        src = "(uint" + std::to_string(kind * 8) + "_t)" + src;
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        handle_UnaryMinus(x);
    }

    void visit_ComplexUnaryMinus(const ASR::ComplexUnaryMinus_t &x) {
        handle_UnaryMinus(x);
    }

    template <typename T>
    void handle_UnaryMinus(const T &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        int expr_precedence = last_expr_precedence;
        last_expr_precedence = 3;
        if (expr_precedence < last_expr_precedence) {
            src = "-" + src;
        } else {
            src = "-(" + src + ")";
        }
    }

    void visit_ComplexRe(const ASR::ComplexRe_t &x) {
        headers.insert("complex.h");
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        if (is_c) {
            src = "creal(" + src + ")";
        } else {
            src = src + ".real()";
        }
    }

    void visit_ComplexIm(const ASR::ComplexIm_t &x) {
        headers.insert("complex.h");
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        if (is_c) {
            src = "cimag(" + src + ")";
        } else {
            src = src + ".imag()";
        }
    }

    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        int expr_precedence = last_expr_precedence;
        last_expr_precedence = 3;
        if (expr_precedence <= last_expr_precedence) {
            src = "!" + src;
        } else {
            src = "!(" + src + ")";
        }
    }

    void visit_PointerNullConstant(const ASR::PointerNullConstant_t& /*x*/) {
        src = "NULL";
    }

    void visit_PointerAssociated(const ASR::PointerAssociated_t &x) {
        if (x.m_value) {
            self().visit_expr(*x.m_value);
            return;
        }
        auto get_pointer_compare_expr = [&](ASR::expr_t *expr) {
            ASR::expr_t *unwrapped_expr = unwrap_c_lvalue_expr(expr);
            if (unwrapped_expr && ASR::is_a<ASR::Var_t>(*unwrapped_expr)) {
                ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(unwrapped_expr)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*sym)) {
                    ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
                    if (ASRUtils::is_pointer(var->m_type)
                            && !ASRUtils::is_array(var->m_type)
                            && !ASR::is_a<ASR::FunctionType_t>(
                                *ASRUtils::type_get_past_pointer(var->m_type))) {
                        if (ASRUtils::is_arg_dummy(var->m_intent)) {
                            return "(*" + CUtils::get_c_variable_name(*var) + ")";
                        }
                        return CUtils::get_c_variable_name(*var);
                    }
                }
            }
            self().visit_expr(*expr);
            return src;
        };
        std::string ptr_src = get_pointer_compare_expr(x.m_ptr);
        if (x.m_tgt == nullptr || ASR::is_a<ASR::PointerNullConstant_t>(*x.m_tgt)) {
            src = "(" + ptr_src + " != NULL)";
            last_expr_precedence = 6;
            return;
        }
        self().visit_expr(*x.m_tgt);
        std::string tgt_src = std::move(src);
        std::string addr_prefix = "&";
        ASR::ttype_t *tgt_type = ASRUtils::expr_type(x.m_tgt);
        if (ASRUtils::is_array(tgt_type) ||
            ASR::is_a<ASR::StructType_t>(*ASRUtils::type_get_past_allocatable(
                ASRUtils::type_get_past_pointer(
                    ASRUtils::type_get_past_array(tgt_type))))) {
            addr_prefix.clear();
        }
        src = "(" + ptr_src + " == " + addr_prefix + tgt_src + ")";
        last_expr_precedence = 6;
    }

    void visit_GetPointer(const ASR::GetPointer_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        std::string arg_src = std::move(src);
        std::string addr_prefix = "&";
        if( ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) ||
            ASR::is_a<ASR::StructType_t>(*ASRUtils::expr_type(x.m_arg)) ) {
            addr_prefix.clear();
        }
        src = addr_prefix + arg_src;
    }

    void visit_PointerToCPtr(const ASR::PointerToCPtr_t& x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_arg);
        std::string arg_src = std::move(src);
        if( ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) ) {
            arg_src += "->data";
        }
        std::string type_src = CUtils::get_c_type_from_ttype_t(x.m_type);
        src = "(" + type_src + ") " + arg_src;
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t &x) {
        handle_BinOp(x);
    }

    void visit_UnsignedIntegerBinOp(const ASR::UnsignedIntegerBinOp_t &x) {
        handle_BinOp(x);
        int kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
        src = "(uint" + std::to_string(kind * 8) + "_t)(" + src + ")";
    }

    void visit_RealBinOp(const ASR::RealBinOp_t &x) {
        handle_BinOp(x);
    }

    void visit_RealCopySign(const ASR::RealCopySign_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        if (x.m_value) {
            self().visit_expr(*x.m_value);
            return;
        }
        self().visit_expr(*x.m_target);
        std::string target = std::move(src);
        self().visit_expr(*x.m_source);
        std::string source = std::move(src);
        headers.insert("math.h");
        src = "copysign(" + target + ", " + source + ")";
        if (!is_c) {
            src = "std::" + src;
        }
        last_expr_precedence = 2;
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t &x) {
        handle_BinOp(x);
    }

    void visit_ComplexConstructor(const ASR::ComplexConstructor_t &x) {
        self().visit_expr(*x.m_re);
        std::string re = std::move(src);
        self().visit_expr(*x.m_im);
        std::string im = std::move(src);
        src = "CMPLX(" + re + "," + im + ")";
    }

    void visit_StructConstructor(const ASR::StructConstructor_t &x) {
        std::string out = "{";
        ASR::symbol_t *struct_sym = ASRUtils::symbol_get_past_external(x.m_dt_sym);
        ASR::Struct_t *st = ASR::down_cast<ASR::Struct_t>(struct_sym);
        out += "." + get_runtime_type_tag_member_name() + " = "
            + std::to_string(get_struct_runtime_type_id(x.m_dt_sym));
        if (x.n_args > 0) {
            out += ", ";
        }
        for (size_t i = 0; i < x.n_args; i++) {
            if (x.m_args[i].m_value) {
                ASR::symbol_t *member_sym = st->m_symtab->get_symbol(st->m_members[i]);
                out += ".";
                out += CUtils::get_c_member_name(member_sym);
                out += " = ";
                std::string array_init = emit_c_array_constant_brace_init(
                    x.m_args[i].m_value, ASRUtils::symbol_type(member_sym));
                if (!array_init.empty()) {
                    out += array_init;
                } else {
                    self().visit_expr(*x.m_args[i].m_value);
                    ASR::symbol_t *member_type_decl = nullptr;
                    if (ASR::is_a<ASR::Variable_t>(*member_sym)) {
                        member_type_decl =
                            ASR::down_cast<ASR::Variable_t>(member_sym)->m_type_declaration;
                    }
                    out += coerce_c_struct_value_for_target(
                        ASRUtils::symbol_type(member_sym), member_type_decl,
                        x.m_args[i].m_value, src);
                }
                if (i < x.n_args-1) {
                    out += ", ";
                }
            }
        }
        out += "}";
        if (is_c) {
            std::string concrete_type = get_c_concrete_type_from_ttype_t(x.m_type, x.m_dt_sym);
            if (!concrete_type.empty()) {
                src = "(" + concrete_type + ")" + out;
                return;
            }
        }
        src = out;
    }

    void visit_StructConstant(const ASR::StructConstant_t &x) {
        std::string out = "{";
        ASR::symbol_t *struct_sym = ASRUtils::symbol_get_past_external(x.m_dt_sym);
        ASR::Struct_t *st = ASR::down_cast<ASR::Struct_t>(struct_sym);
        out += "." + get_runtime_type_tag_member_name() + " = "
            + std::to_string(get_struct_runtime_type_id(x.m_dt_sym));
        if (x.n_args > 0) {
            out += ", ";
        }
        for (size_t i = 0; i < x.n_args; i++) {
            if (x.m_args[i].m_value) {
                ASR::symbol_t *member_sym = st->m_symtab->get_symbol(st->m_members[i]);
                out += ".";
                out += CUtils::get_c_member_name(member_sym);
                out += " = ";
                std::string array_init = emit_c_array_constant_brace_init(
                    x.m_args[i].m_value, ASRUtils::symbol_type(member_sym));
                if (!array_init.empty()) {
                    out += array_init;
                } else {
                    self().visit_expr(*x.m_args[i].m_value);
                    ASR::symbol_t *member_type_decl = nullptr;
                    if (ASR::is_a<ASR::Variable_t>(*member_sym)) {
                        member_type_decl =
                            ASR::down_cast<ASR::Variable_t>(member_sym)->m_type_declaration;
                    }
                    out += coerce_c_struct_value_for_target(
                        ASRUtils::symbol_type(member_sym), member_type_decl,
                        x.m_args[i].m_value, src);
                }
                if (i < x.n_args-1) {
                    out += ", ";
                }
            }
        }
        out += "}";
        if (is_c) {
            std::string concrete_type = get_c_concrete_type_from_ttype_t(x.m_type, x.m_dt_sym);
            if (!concrete_type.empty()) {
                src = "(" + concrete_type + ")" + out;
                return;
            }
        }
        src = out;
    }

    int get_small_integer_power_exponent(ASR::expr_t *expr) {
        ASR::expr_t *value = ASRUtils::expr_value(expr);
        if (value == nullptr) {
            value = expr;
        }
        if (!ASR::is_a<ASR::IntegerConstant_t>(*value)) {
            return -1;
        }
        int64_t exponent = ASR::down_cast<ASR::IntegerConstant_t>(value)->m_n;
        if (exponent < 2 || exponent > 4) {
            return -1;
        }
        return static_cast<int>(exponent);
    }

    bool is_c_duplicate_safe_power_base(ASR::expr_t *expr) {
        if (expr == nullptr) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::IntegerConstant:
            case ASR::exprType::UnsignedIntegerConstant:
            case ASR::exprType::RealConstant: {
                return true;
            }
            case ASR::exprType::StructInstanceMember: {
                ASR::StructInstanceMember_t *member =
                    ASR::down_cast<ASR::StructInstanceMember_t>(expr);
                return is_c_duplicate_safe_power_base(member->m_v);
            }
            case ASR::exprType::ArrayItem: {
                if (ASRUtils::is_array(ASRUtils::expr_type(expr))) {
                    return false;
                }
                ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
                if (!is_c_duplicate_safe_power_base(item->m_v)) {
                    return false;
                }
                for (size_t i = 0; i < item->n_args; i++) {
                    ASR::expr_t *idx_expr = get_array_index_expr(item->m_args[i]);
                    if (idx_expr == nullptr || is_vector_subscript_expr(idx_expr)
                            || !is_c_duplicate_safe_power_base(idx_expr)) {
                        return false;
                    }
                }
                return true;
            }
            case ASR::exprType::Cast: {
                return is_c_duplicate_safe_power_base(
                    ASR::down_cast<ASR::Cast_t>(expr)->m_arg);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_duplicate_safe_power_base(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_duplicate_safe_power_base(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::UnsignedIntegerUnaryMinus: {
                return is_c_duplicate_safe_power_base(
                    ASR::down_cast<ASR::UnsignedIntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_duplicate_safe_power_base(binop->m_left)
                    && is_c_duplicate_safe_power_base(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_duplicate_safe_power_base(binop->m_left)
                    && is_c_duplicate_safe_power_base(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_duplicate_safe_power_base(binop->m_left)
                    && is_c_duplicate_safe_power_base(binop->m_right);
            }
            default: {
                return false;
            }
        }
    }

    bool try_emit_small_integer_power(ASR::expr_t *left_expr, ASR::expr_t *right_expr,
            const std::string &left) {
        int exponent = get_small_integer_power_exponent(right_expr);
        if (exponent == -1 || !is_c_duplicate_safe_power_base(left_expr)) {
            return false;
        }
        src = "(" + left + ")";
        for (int i = 1; i < exponent; i++) {
            src += "*(" + left + ")";
        }
        last_expr_precedence = 5;
        return true;
    }

    bool try_emit_c_real_integer_power(ASR::expr_t *left_expr, ASR::expr_t *right_expr,
            const std::string &left, const std::string &right) {
        if (!is_c) {
            return false;
        }
        ASR::ttype_t *left_type = ASRUtils::extract_type(ASRUtils::expr_type(left_expr));
        ASR::ttype_t *right_type = ASRUtils::extract_type(ASRUtils::expr_type(right_expr));
        if (!ASRUtils::is_real(*left_type) || !ASRUtils::is_integer(*right_type)) {
            return false;
        }
        int left_kind = ASRUtils::extract_kind_from_ttype_t(left_type);
        if (left_kind == 4) {
            src = "__lfortran_c_powi_f32(" + left + ", " + right + ")";
        } else if (left_kind == 8) {
            src = "__lfortran_c_powi_f64(" + left + ", " + right + ")";
        } else {
            return false;
        }
        last_expr_precedence = 2;
        return true;
    }

    bool try_emit_cached_c_real_power(ASR::expr_t *left_expr, ASR::expr_t *right_expr,
            ASR::ttype_t *result_type, const std::string &left,
            const std::string &right) {
        if (!is_c || current_function == nullptr || c_pow_cache_safe_expr_depth == 0
                || c_pow_cache_suppression_depth > 0) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::extract_type(result_type);
        if (type == nullptr || !ASRUtils::is_real(*type)) {
            return false;
        }
        std::set<ASR::symbol_t*> deps;
        if (!collect_c_pow_cache_dependencies(left_expr, deps)
                || !collect_c_pow_cache_dependencies(right_expr, deps)
                || deps.empty()) {
            return false;
        }
        std::string key = "pow(" + left + ", " + right + ")";
        auto it = current_function_pow_cache.find(key);
        if (it != current_function_pow_cache.end()) {
            src = it->second.name;
            last_expr_precedence = 2;
            return true;
        }
        std::string name = get_unique_local_name("__lfortran_pow_cache");
        std::string indent(indentation_level * indentation_spaces, ' ');
        headers.insert("math.h");
        tmp_buffer_src.push_back(indent + CUtils::get_c_type_from_ttype_t(type)
            + " " + name + " = pow(" + left + ", " + right + ");\n");
        current_function_pow_cache[key] = {name, deps};
        src = name;
        last_expr_precedence = 2;
        return true;
    }

    template <typename T>
    void handle_BinOp(const T &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (ASR::binopType::Add) : { last_expr_precedence = 6; break; }
            case (ASR::binopType::Sub) : { last_expr_precedence = 6; break; }
            case (ASR::binopType::Mul) : { last_expr_precedence = 5; break; }
            case (ASR::binopType::Div) : { last_expr_precedence = 5; break; }
            case (ASR::binopType::BitAnd) : { last_expr_precedence = 11; break; }
            case (ASR::binopType::BitOr) : { last_expr_precedence = 13; break; }
            case (ASR::binopType::BitXor) : { last_expr_precedence = 12; break; }
            case (ASR::binopType::BitLShift) : { last_expr_precedence = 7; break; }
            case (ASR::binopType::BitRShift) : { last_expr_precedence = 7; break; }
            case (ASR::binopType::LBitRShift) : { last_expr_precedence = 7; break; }
            case (ASR::binopType::Pow) : {
                if (try_emit_small_integer_power(x.m_left, x.m_right, left)) {
                    return;
                }
                if (try_emit_c_real_integer_power(x.m_left, x.m_right, left, right)) {
                    return;
                }
                if (try_emit_cached_c_real_power(
                        x.m_left, x.m_right, x.m_type, left, right)) {
                    return;
                }
                src = "pow(" + left + ", " + right + ")";
                if (is_c) {
                    headers.insert("math.h");
                } else {
                    src = "std::" + src;
                }
                return;
            }
            default: throw CodeGenError("BinOp: " + std::to_string(x.m_op) + " operator not implemented yet");
        }
        if (is_c && x.m_op == ASR::binopType::LBitRShift) {
            int kind = ASRUtils::extract_kind_from_ttype_t(x.m_type);
            std::string signed_type = CUtils::get_c_type_from_ttype_t(x.m_type);
            std::string unsigned_type = "uint" + std::to_string(kind * 8) + "_t";
            src = "((" + signed_type + ")(((" + unsigned_type + ")(" + left
                + ")) >> (" + right + ")))";
            last_expr_precedence = 2;
            return;
        }
        src = "";
        if (left_precedence == 3) {
            src += "(" + left + ")";
        } else {
            if (left_precedence <= last_expr_precedence) {
                src += left;
            } else {
                src += "(" + left + ")";
            }
        }
        src += ASRUtils::binop_to_str_python(x.m_op);
        if (right_precedence == 3) {
            src += "(" + right + ")";
        } else {
            if (right_precedence < last_expr_precedence) {
                src += right;
            } else {
                src += "(" + right + ")";
            }
        }
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t &x) {
        CHECK_FAST_C_CPP(compiler_options, x)
        self().visit_expr(*x.m_left);
        std::string left = std::move(src);
        int left_precedence = last_expr_precedence;
        self().visit_expr(*x.m_right);
        std::string right = std::move(src);
        int right_precedence = last_expr_precedence;
        switch (x.m_op) {
            case (ASR::logicalbinopType::And): {
                last_expr_precedence = 14;
                break;
            }
            case (ASR::logicalbinopType::Or): {
                last_expr_precedence = 15;
                break;
            }
            case (ASR::logicalbinopType::NEqv): {
                last_expr_precedence = 10;
                break;
            }
            case (ASR::logicalbinopType::Eqv): {
                last_expr_precedence = 10;
                break;
            }
            default : throw CodeGenError("Unhandled switch case");
        }

        if (left_precedence <= last_expr_precedence) {
            src += left;
        } else {
            src += "(" + left + ")";
        }
        src += ASRUtils::logicalbinop_to_str_python(x.m_op);
        if (right_precedence <= last_expr_precedence) {
            src += right;
        } else {
            src += "(" + right + ")";
        }
    }

    void allocate_array_members_of_struct(ASR::Struct_t*, std::string&,
            std::string, std::string) {
    }

    std::string get_allocate_stat_target(const ASR::Allocate_t &x) {
        if (!is_c || x.m_stat == nullptr) {
            return "";
        }
        self().visit_expr(*x.m_stat);
        return src;
    }

    std::string get_allocate_stat_target(const ASR::ReAlloc_t &) {
        return "";
    }

    void emit_allocate_stat_failure_check(std::string &out,
            const std::string &indent, const std::string &stat_tmp,
            const std::string &success_expr) {
        if (stat_tmp.empty()) {
            return;
        }
        out += indent + "if (!(" + success_expr + ")) {\n";
        out += indent + "    " + stat_tmp + " = 1;\n";
        out += indent + "}\n";
    }

    template <typename T>
    void handle_alloc_realloc(const T &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        bool is_realloc = T::class_type == ASR::stmtType::ReAlloc;
        if (is_c && current_function &&
                std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0) {
            src = indent + "/* move_alloc helper: allocation handled by ownership transfer */\n";
            return;
        }
        std::string out = "";
        std::string stat_target = get_allocate_stat_target(x);
        std::string stat_tmp;
        if (!stat_target.empty()) {
            stat_tmp = get_unique_local_name("__lfortran_alloc_stat");
            out += indent + "int32_t " + stat_tmp + " = 0;\n";
        }
        for (size_t i=0; i<x.n_args; i++) {
            ASR::ttype_t* type = nullptr;
            ASR::expr_t* tmp_expr = x.m_args[i].m_a;
            std::string sym;
            ASR::Variable_t *target_var = nullptr;
            if( ASR::is_a<ASR::Var_t>(*tmp_expr) ) {
                const ASR::Var_t* tmp_var = ASR::down_cast<ASR::Var_t>(tmp_expr);
                type = ASRUtils::expr_type(tmp_expr);
                ASR::symbol_t *var_sym = ASRUtils::symbol_get_past_external(tmp_var->m_v);
                if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                    ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(var_sym);
                    target_var = var;
                    if (is_c && is_aggregate_dummy_slot_type(var)) {
                        sym = "(*" + CUtils::get_c_variable_name(*var) + ")";
                    } else if (is_c && is_scalar_allocatable_storage_type(var->m_type)) {
                        sym = get_c_var_storage_name(var);
                    } else {
                        sym = CUtils::get_c_variable_name(*var);
                    }
                    if (is_c && !ASRUtils::is_array(type)) {
                        ASR::ttype_t *past_type = ASRUtils::type_get_past_allocatable_pointer(type);
                        if (past_type != nullptr && ASRUtils::is_character(*past_type)) {
                            sym = get_c_mutable_scalar_expr(tmp_expr);
                        }
                    }
                } else {
                    sym = CUtils::get_c_symbol_name(var_sym);
                }
            } else if (ASR::is_a<ASR::StructInstanceMember_t>(*tmp_expr) ||
                       ASR::is_a<ASR::UnionInstanceMember_t>(*tmp_expr) ||
                       ASR::is_a<ASR::ArrayItem_t>(*tmp_expr)) {
                type = ASRUtils::expr_type(tmp_expr);
                if (is_c && ASR::is_a<ASR::StructInstanceMember_t>(*tmp_expr)) {
                    ASR::StructInstanceMember_t *member_expr =
                        ASR::down_cast<ASR::StructInstanceMember_t>(tmp_expr);
                    ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_expr->m_m);
                    if (ASR::is_a<ASR::Variable_t>(*member_sym)
                            && is_scalar_allocatable_storage_type(
                                ASR::down_cast<ASR::Variable_t>(member_sym)->m_type)) {
                        sym = get_struct_instance_member_expr(*member_expr, false);
                    } else {
                        self().visit_expr(*tmp_expr);
                        sym = src;
                    }
                } else {
                    self().visit_expr(*tmp_expr);
                    sym = src;
                }
            } else {
                throw CodeGenError("Cannot deallocate variables in expression " +
                                    ASRUtils::type_to_str_python_expr(ASRUtils::expr_type(tmp_expr), tmp_expr),
                                    tmp_expr->base.loc);
            }
            if (ASRUtils::is_array(type)) {
                if (is_c && is_unlimited_polymorphic_storage_type(type)) {
                    continue;
                }
                if (is_c && is_vector_subscript_scalar_array_item(tmp_expr)) {
                    continue;
                }
                if (is_c && target_var != nullptr
                        && current_function != nullptr
                        && target_var->m_parent_symtab == current_function->m_symtab
                        && target_var->m_intent == ASRUtils::intent_local
                        && ASRUtils::is_allocatable(target_var->m_type)) {
                    register_current_function_local_allocatable_array_cleanup(sym);
                }
                std::string size_str = "1";
                std::string old_size_name;
                if (is_realloc) {
                    old_size_name = get_unique_local_name("__lfortran_realloc_old_size");
                    std::string dim_name =
                        get_unique_local_name("__lfortran_realloc_dim");
                    out += indent + "int64_t " + old_size_name + " = 0;\n";
                    out += indent + "if (" + sym + " != NULL && " + sym
                        + "->is_allocated && " + sym + "->data != NULL) {\n";
                    out += indent + "    " + old_size_name + " = 1;\n";
                    out += indent + "    for (int32_t " + dim_name + " = 0; "
                        + dim_name + " < " + sym + "->n_dims; " + dim_name
                        + "++) {\n";
                    out += indent + "        " + old_size_name + " *= " + sym
                        + "->dims[" + dim_name + "].length;\n";
                    out += indent + "    }\n";
                    out += indent + "}\n";
                }
                out += indent + sym + "->n_dims = " + std::to_string(x.m_args[i].n_dims) + ";\n";
                std::string stride = "1";
                for (size_t j = 0; j < x.m_args[i].n_dims; j++) {
                    std::string st, l;
                    if (x.m_args[i].m_dims[j].m_start) {
                        self().visit_expr(*x.m_args[i].m_dims[j].m_start);
                        st = src;
                    } else {
                        st = "0";
                    }
                    if (x.m_args[i].m_dims[j].m_length) {
                        self().visit_expr(*x.m_args[i].m_dims[j].m_length);
                        l = src;
                    } else {
                        l = "1";
                    }
                    size_str += "*" + sym + "->dims[" + std::to_string(j) + "].length";
                    out += indent + sym + "->dims[" + std::to_string(j) + "].lower_bound = ";
                    out += st + ";\n";
                    out += indent + sym + "->dims[" + std::to_string(j) + "].length = ";
                    out += l + ";\n";
                    out += indent + sym + "->dims[" + std::to_string(j) + "].stride = ";
                    out += stride + ";\n";
                    stride = "(" + stride + " * " + l + ")";
                }
                ASR::ttype_t *array_type = CUtils::get_c_array_type_for_wrapper(type);
                LCOMPILERS_ASSERT(array_type != nullptr);
                ASR::ttype_t *element_type = ASRUtils::type_get_past_array(array_type);
                std::string ty = CUtils::get_c_array_element_type_from_ttype_t(type);
                ASR::Struct_t *element_struct_t = nullptr;
                ASR::symbol_t *element_struct_sym = nullptr;
                if (ASR::is_a<ASR::StructType_t>(*element_type)) {
                    ASR::StructType_t *struct_type = ASR::down_cast<ASR::StructType_t>(element_type);
                    if (!struct_type->m_is_unlimited_polymorphic) {
                        ASR::symbol_t *struct_sym = ASRUtils::symbol_get_past_external(
                            ASRUtils::get_struct_sym_from_struct_expr(tmp_expr));
                        if (struct_sym && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                            ty = "struct " + CUtils::get_c_symbol_name(struct_sym);
                            element_struct_sym = struct_sym;
                            element_struct_t = ASR::down_cast<ASR::Struct_t>(struct_sym);
                        }
                    }
                }
                std::string elem_count = size_str;
                size_str += "*sizeof(" + ty + ")";
                out += indent + sym + "->offset = 0;\n";
                std::string did_allocate_name;
                if (is_realloc && element_struct_t != nullptr) {
                    did_allocate_name =
                        get_unique_local_name("__lfortran_realloc_did_allocate");
                    out += indent + "bool " + did_allocate_name + " = false;\n";
                }
                if (is_realloc) {
                    std::string new_size_name =
                        get_unique_local_name("__lfortran_realloc_new_size");
                    out += indent + "int64_t " + new_size_name + " = "
                        + elem_count + ";\n";
                    out += indent + "if (" + sym + "->is_allocated && " + sym
                        + "->data != NULL && " + old_size_name + " != "
                        + new_size_name + ") {\n";
                    out += indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                        + "(char*) " + sym + "->data);\n";
                    out += indent + "    " + sym + "->data = NULL;\n";
                    out += indent + "    " + sym + "->is_allocated = false;\n";
                    out += indent + "}\n";
                    out += indent + "if (!" + sym + "->is_allocated || "
                        + sym + "->data == NULL) {\n";
                    out += indent + "    " + sym + "->data = (" + ty
                        + "*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), "
                        + size_str + ");\n";
                    if (!did_allocate_name.empty()) {
                        out += indent + "    " + did_allocate_name + " = true;\n";
                    }
                    out += indent + "}\n";
                    emit_allocate_stat_failure_check(out, indent, stat_tmp,
                        elem_count + " == 0 || " + sym + "->data != NULL");
                } else {
                    out += indent + sym + "->data = (" + ty
                        + "*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), "
                        + size_str + ")";
                    out += ";\n";
                    emit_allocate_stat_failure_check(out, indent, stat_tmp,
                        elem_count + " == 0 || " + sym + "->data != NULL");
                }
                out += indent + sym + "->is_allocated = true;\n";
                if (is_c && element_struct_t != nullptr) {
                    std::string init_indent = indent;
                    if (!did_allocate_name.empty()) {
                        out += indent + "if (" + did_allocate_name + ") {\n";
                        init_indent += "    ";
                    }
                    out += init_indent + "memset(" + sym + "->data, 0, " + size_str + ");\n";
                    std::string init_idx = get_unique_local_name("__lfortran_struct_array_init_i");
                    out += init_indent + "for (int64_t " + init_idx + " = 0; " + init_idx
                        + " < " + elem_count + "; " + init_idx + "++) {\n";
                    std::string elem_ptr = "(&(" + sym + "->data[" + init_idx + "]))";
                    out += init_indent + "    " + elem_ptr + "->"
                        + get_runtime_type_tag_member_name() + " = "
                        + std::to_string(get_struct_runtime_type_id(element_struct_sym)) + ";\n";
                    self().initialize_struct_instance_members(
                        element_struct_t, out, init_indent + "    ", elem_ptr);
                    out += init_indent + "}\n";
                    if (!did_allocate_name.empty()) {
                        out += indent + "}\n";
                    }
                }
            } else {
                ASR::ttype_t *past_alloc_type = ASRUtils::type_get_past_allocatable_pointer(type);
                std::string ty = CUtils::get_c_type_from_ttype_t(type), size_str;
                std::string alloc_ty = ty;
                ASR::symbol_t *alloc_struct_sym = x.m_args[i].m_sym_subclass ?
                    ASRUtils::symbol_get_past_external(x.m_args[i].m_sym_subclass) : nullptr;
                if (ASRUtils::is_character(*past_alloc_type)) {
                    if (is_c && is_c_compiler_created_return_slot_expr(tmp_expr)) {
                        out += indent + "if (" + sym + " != NULL) {\n";
                        out += indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                            + sym + ");\n";
                        out += indent + "    " + sym + " = NULL;\n";
                        out += indent + "}\n";
                        continue;
                    }
                    std::string len_str = "1";
                    if (x.m_args[i].m_len_expr) {
                        self().visit_expr(*x.m_args[i].m_len_expr);
                        len_str = src;
                    } else {
                        ASR::String_t *str_type = ASR::down_cast<ASR::String_t>(past_alloc_type);
                        if (str_type->m_len) {
                            self().visit_expr(*str_type->m_len);
                            len_str = src;
                        } else if (x.m_args[i].n_dims > 0 && x.m_args[i].m_dims[0].m_length) {
                            self().visit_expr(*x.m_args[i].m_dims[0].m_length);
                            len_str = src;
                        }
                    }
                    out += indent + sym + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                        + len_str + ")";
                    out += ";\n";
                    emit_allocate_stat_failure_check(out, indent, stat_tmp,
                        len_str + " == 0 || " + sym + " != NULL");
                    continue;
                }
                if (ASR::is_a<ASR::StructType_t>(*past_alloc_type)) {
                    ASR::StructType_t *struct_type = ASR::down_cast<ASR::StructType_t>(past_alloc_type);
                    if (!struct_type->m_is_unlimited_polymorphic) {
                        ASR::symbol_t *struct_sym = ASRUtils::symbol_get_past_external(
                            ASRUtils::get_struct_sym_from_struct_expr(tmp_expr));
                        if (struct_sym && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                            ty = "struct " + CUtils::get_c_symbol_name(struct_sym);
                        }
                        if (!alloc_struct_sym) {
                            alloc_struct_sym = struct_sym;
                        }
                        if (alloc_struct_sym && ASR::is_a<ASR::Struct_t>(*alloc_struct_sym)) {
                            alloc_ty = "struct " + CUtils::get_c_symbol_name(alloc_struct_sym);
                        }
                    }
                }
                if (ASRUtils::is_class_type(type)) {
                    size_str = "sizeof(" + alloc_ty + ")";
                } else {
                    size_str = "sizeof(" + alloc_ty + ")";
                }
                out += indent + sym + " = (" + ty + "*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), " + size_str + ")";
                out += ";\n";
                emit_allocate_stat_failure_check(out, indent, stat_tmp,
                    sym + " != NULL");
                if (alloc_struct_sym && ASR::is_a<ASR::Struct_t>(*alloc_struct_sym)) {
                    std::string alloc_struct_ptr = "((" + alloc_ty + "*)(" + sym + "))";
                    out += indent + "memset(" + sym + ", 0, sizeof(" + alloc_ty + "));\n";
                    ASR::Struct_t *alloc_struct_t = ASR::down_cast<ASR::Struct_t>(alloc_struct_sym);
                    self().initialize_struct_instance_members(
                        alloc_struct_t, out, indent, alloc_struct_ptr);
                }
                if (alloc_struct_sym && ASR::is_a<ASR::Struct_t>(*alloc_struct_sym)) {
                    out += indent + sym + "->" + get_runtime_type_tag_member_name() + " = "
                        + std::to_string(get_struct_runtime_type_id(alloc_struct_sym)) + ";\n";
                }
            }
        }
        if (!stat_target.empty()) {
            out += indent + stat_target + " = " + stat_tmp + ";\n";
        }
        src = out;
    }

    void visit_Allocate(const ASR::Allocate_t &x) {
        handle_alloc_realloc(x);
    }

    void visit_ReAlloc(const ASR::ReAlloc_t &x) {
        handle_alloc_realloc(x);
    }

    std::string get_c_deallocation_target_expr(ASR::expr_t *expr) {
        ASR::expr_t *unwrapped_expr = unwrap_c_lvalue_expr(expr);
        if (unwrapped_expr && ASR::is_a<ASR::StructInstanceMember_t>(*unwrapped_expr)) {
            return get_struct_instance_member_expr(
                *ASR::down_cast<ASR::StructInstanceMember_t>(unwrapped_expr), false);
        }
        if (unwrapped_expr && ASR::is_a<ASR::Var_t>(*unwrapped_expr)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(unwrapped_expr)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
                std::string var_name = get_c_var_storage_name(var);
                if (current_function
                        && std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                        && var_name == "to"
                        && !ASRUtils::is_array(var->m_type)) {
                    return "(*((void**)" + var_name + "))";
                }
                if (current_function
                        && std::string(current_function->m_name).rfind("_lcompilers_move_alloc_", 0) == 0
                        && var_name == "from"
                        && !ASRUtils::is_array(var->m_type)) {
                    return var_name;
                }
                if (ASRUtils::is_arg_dummy(var->m_intent)
                        && ASRUtils::is_allocatable(var->m_type)
                        && !ASRUtils::is_array(var->m_type)) {
                    return "(*" + var_name + ")";
                }
                if (is_pointer_dummy_slot_type(var)
                        || is_scalar_allocatable_dummy_slot_type(var)
                        || is_unlimited_polymorphic_dummy_slot_type(var)) {
                    return "(*" + var_name + ")";
                }
                if (ASRUtils::is_pointer(var->m_type)
                        || ASRUtils::is_allocatable(var->m_type)) {
                    return var_name;
                }
            }
        }
        self().visit_expr(*expr);
        return src;
    }

    bool c_array_expr_uses_descriptor(ASR::expr_t *expr) {
        ASR::ttype_t *type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable_pointer(ASRUtils::expr_type(expr)));
        return !ASRUtils::is_unlimited_polymorphic_type(type);
    }

    bool is_c_compiler_created_return_slot_expr(ASR::expr_t *expr) {
        expr = unwrap_c_lvalue_expr(expr);
        if (expr == nullptr || !ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        if (!ASR::is_a<ASR::Variable_t>(*sym)) {
            return false;
        }
        std::string name = CUtils::get_c_variable_name(*ASR::down_cast<ASR::Variable_t>(sym));
        return name.find("__libasr__created__var__") != std::string::npos
            && name.find("return_slot") != std::string::npos;
    }

    bool is_c_compiler_created_return_slot_name(const std::string &name) const {
        return name.find("__libasr__created__var__") != std::string::npos
            && name.find("return_slot") != std::string::npos;
    }

    std::string emit_c_scope_compiler_return_slot_cleanup(SymbolTable *symtab,
            const std::string &indent) {
        if (!is_c || symtab == nullptr) {
            return "";
        }
        std::string cleanup;
        std::vector<std::string> var_order =
            ASRUtils::determine_variable_declaration_order(symtab);
        for (auto it = var_order.rbegin(); it != var_order.rend(); ++it) {
            ASR::symbol_t *var_sym = symtab->get_symbol(*it);
            if (!ASR::is_a<ASR::Variable_t>(*var_sym)) {
                continue;
            }
            ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(var_sym);
            std::string target = CUtils::get_c_variable_name(*var);
            ASR::ttype_t *value_type =
                ASRUtils::type_get_past_allocatable_pointer(var->m_type);
            if (!is_c_compiler_created_return_slot_name(target)
                    || !ASRUtils::is_allocatable(var->m_type)
                    || ASRUtils::is_array(var->m_type)
                    || value_type == nullptr
                    || !ASRUtils::is_character(*value_type)) {
                continue;
            }
            cleanup += indent + "if (" + target + " != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + target + ");\n"
                + indent + "    " + target + " = NULL;\n"
                + indent + "}\n";
        }
        return cleanup;
    }

    std::string emit_c_scope_allocatable_array_cleanup(SymbolTable *symtab,
            const std::string &indent) {
        if (!is_c || symtab == nullptr) {
            return "";
        }
        std::string cleanup;
        std::vector<std::string> var_order =
            ASRUtils::determine_variable_declaration_order(symtab);
        for (auto it = var_order.rbegin(); it != var_order.rend(); ++it) {
            ASR::symbol_t *var_sym = symtab->get_symbol(*it);
            if (!ASR::is_a<ASR::Variable_t>(*var_sym)) {
                continue;
            }
            ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(var_sym);
            if (var->m_intent != ASRUtils::intent_local
                    || !ASRUtils::is_allocatable(var->m_type)
                    || !ASRUtils::is_array(var->m_type)) {
                continue;
            }
            std::string target = CUtils::get_c_variable_name(*var);
            ASR::ttype_t *target_value_type =
                ASRUtils::type_get_past_allocatable_pointer(var->m_type);
            ASR::ttype_t *element_type =
                ASRUtils::extract_type(target_value_type);
            if (element_type != nullptr && ASRUtils::is_character(*element_type)) {
                cleanup += emit_c_character_array_element_cleanup(target, indent);
            } else if (element_type != nullptr
                    && ASR::is_a<ASR::StructType_t>(*element_type)
                    && var->m_type_declaration != nullptr) {
                ASR::symbol_t *struct_sym =
                    ASRUtils::symbol_get_past_external(var->m_type_declaration);
                if (struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*struct_sym)
                        && c_struct_has_member_cleanup(
                            ASR::down_cast<ASR::Struct_t>(struct_sym))) {
                    std::string idx = "__lfortran_cleanup_i_"
                        + CUtils::sanitize_c_identifier(target);
                    cleanup += indent + "if ((" + target + ") != NULL && ("
                        + target + ")->is_allocated && (" + target
                        + ")->data != NULL) {\n"
                        + indent + "    for (int64_t " + idx + " = 0; "
                        + idx + " < (" + target + ")->dims[0].length; "
                        + idx + "++) {\n";
                    cleanup += emit_c_struct_member_cleanup(
                        ASR::down_cast<ASR::Struct_t>(struct_sym),
                        indent + "        ", "(&((" + target + ")->data["
                        + idx + "]))", true, true, true, false);
                    cleanup += indent + "    }\n"
                        + indent + "}\n";
                }
            }
            cleanup += indent + "if ((" + target + ") != NULL && (" + target
                + ")->is_allocated && (" + target + ")->data != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + "(char*) (" + target + ")->data);\n"
                + indent + "    (" + target + ")->data = NULL;\n"
                + indent + "}\n"
                + indent + "if ((" + target + ") != NULL) {\n"
                + indent + "    (" + target + ")->offset = 0;\n"
                + indent + "    (" + target + ")->is_allocated = false;\n"
                + indent + "}\n";
        }
        return cleanup;
    }

    std::string emit_c_deallocate(ASR::expr_t *expr, const std::string &indent,
            const std::string &) {
        if (ASRUtils::is_array(ASRUtils::expr_type(expr))
                && c_array_expr_uses_descriptor(expr)) {
            std::string target = get_c_deallocation_target_expr(expr);
            std::string cleanup;
            ASR::ttype_t *target_value_type = ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(expr));
            ASR::ttype_t *element_type = ASRUtils::extract_type(target_value_type);
            ASR::symbol_t *struct_sym = nullptr;
            ASR::expr_t *unwrapped_expr = unwrap_c_lvalue_expr(expr);
            if (unwrapped_expr && ASR::is_a<ASR::Var_t>(*unwrapped_expr)) {
                ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(unwrapped_expr)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*sym)) {
                    struct_sym = ASR::down_cast<ASR::Variable_t>(sym)->m_type_declaration;
                }
            } else if (unwrapped_expr
                    && ASR::is_a<ASR::StructInstanceMember_t>(*unwrapped_expr)) {
                ASR::StructInstanceMember_t *member_expr =
                    ASR::down_cast<ASR::StructInstanceMember_t>(unwrapped_expr);
                ASR::symbol_t *member_sym =
                    ASRUtils::symbol_get_past_external(member_expr->m_m);
                if (ASR::is_a<ASR::Variable_t>(*member_sym)) {
                    struct_sym = ASR::down_cast<ASR::Variable_t>(
                        member_sym)->m_type_declaration;
                }
            }
            if (element_type != nullptr && ASRUtils::is_character(*element_type)) {
                cleanup += emit_c_character_array_element_cleanup(target, indent);
            } else if (element_type != nullptr && ASR::is_a<ASR::StructType_t>(*element_type)
                    && struct_sym != nullptr) {
                struct_sym = ASRUtils::symbol_get_past_external(struct_sym);
                if (struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*struct_sym)
                        && c_struct_has_member_cleanup(
                            ASR::down_cast<ASR::Struct_t>(struct_sym))) {
                    std::string idx = "__lfortran_cleanup_i_"
                        + CUtils::sanitize_c_identifier(target);
                    cleanup += indent + "if ((" + target + ") != NULL && (" + target
                        + ")->is_allocated && (" + target + ")->data != NULL) {\n"
                        + indent + "    for (int64_t " + idx + " = 0; " + idx
                        + " < (" + target + ")->dims[0].length; " + idx + "++) {\n";
                    cleanup += emit_c_struct_member_cleanup(
                        ASR::down_cast<ASR::Struct_t>(struct_sym),
                        indent + "        ", "(&((" + target + ")->data[" + idx + "]))");
                    cleanup += indent + "    }\n"
                        + indent + "}\n";
                }
            }
            return cleanup + indent + "if ((" + target + ") != NULL && (" + target
                + ")->is_allocated && (" + target + ")->data != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + "(char*) (" + target + ")->data);\n"
                + indent + "    (" + target + ")->data = NULL;\n"
                + indent + "}\n"
                + indent + "if ((" + target + ") != NULL) {\n"
                + indent + "    (" + target + ")->offset = 0;\n"
                + indent + "    (" + target + ")->is_allocated = false;\n"
                + indent + "}\n";
        }
        std::string target = get_c_deallocation_target_expr(expr);
        ASR::ttype_t *target_value_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (target_value_type != nullptr
                && ASRUtils::is_allocatable(ASRUtils::expr_type(expr))
                && !ASRUtils::is_array(ASRUtils::expr_type(expr))
                && ASRUtils::is_character(*target_value_type)) {
            if (current_function != nullptr
                    && std::string(current_function->m_name).rfind(
                        "_lcompilers_move_alloc_", 0) == 0
                    && target == "from") {
                return indent + "if (" + target + " != NULL) {\n"
                    + indent + "    " + target + " = NULL;\n"
                    + indent + "}\n";
            }
            return indent + "if (" + target + " != NULL) {\n"
                + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                + target + ");\n"
                + indent + "    " + target + " = NULL;\n"
                + indent + "}\n";
        }
        if (target_value_type != nullptr
                && ASRUtils::is_allocatable(ASRUtils::expr_type(expr))
                && !ASRUtils::is_array(ASRUtils::expr_type(expr))
                && ASR::is_a<ASR::StructType_t>(*target_value_type)) {
            if (current_function != nullptr
                    && std::string(current_function->m_name).rfind(
                        "_lcompilers_move_alloc_", 0) == 0) {
                return indent + "if ((" + target + ") != NULL) {\n"
                    + indent + "    " + target + " = NULL;\n"
                    + indent + "}\n";
            }
            ASR::symbol_t *struct_sym = nullptr;
            ASR::expr_t *unwrapped_expr = unwrap_c_lvalue_expr(expr);
            if (unwrapped_expr && ASR::is_a<ASR::Var_t>(*unwrapped_expr)) {
                ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(unwrapped_expr)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*sym)) {
                    struct_sym = ASR::down_cast<ASR::Variable_t>(sym)->m_type_declaration;
                }
            } else if (unwrapped_expr
                    && ASR::is_a<ASR::StructInstanceMember_t>(*unwrapped_expr)) {
                ASR::StructInstanceMember_t *member_expr =
                    ASR::down_cast<ASR::StructInstanceMember_t>(unwrapped_expr);
                ASR::symbol_t *member_sym =
                    ASRUtils::symbol_get_past_external(member_expr->m_m);
                if (ASR::is_a<ASR::Variable_t>(*member_sym)) {
                    struct_sym = ASR::down_cast<ASR::Variable_t>(
                        member_sym)->m_type_declaration;
                }
            }
            if (struct_sym == nullptr) {
                struct_sym = ASRUtils::get_struct_sym_from_struct_expr(expr);
            }
            if (struct_sym != nullptr) {
                struct_sym = ASRUtils::symbol_get_past_external(struct_sym);
            }
            if (struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                return emit_c_scalar_allocatable_struct_cleanup(
                    ASR::down_cast<ASR::Struct_t>(struct_sym), indent, target,
                    ASRUtils::is_class_type(target_value_type));
            }
        }
        return indent + "if ((" + target + ") != NULL) {\n"
            + indent + "    " + target + " = NULL;\n"
            + indent + "}\n";
    }


    void visit_Assert(const ASR::Assert_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent;
        if (x.m_msg) {
            out += "assert ((";
            self().visit_expr(*x.m_msg);
            out += src + ", ";
            self().visit_expr(*x.m_test);
            out += src + "));\n";
        } else {
            out += "assert (";
            self().visit_expr(*x.m_test);
            out += src + ");\n";
        }
        src = out;
    }

    void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out;
        for (size_t i=0; i<x.n_vars; i++) {
            out += emit_c_deallocate(x.m_vars[i], indent, "explicit");
        }
        src = out;
    }

    void visit_ImplicitDeallocate(const ASR::ImplicitDeallocate_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out;
        for (size_t i=0; i<x.n_vars; i++) {
            out += emit_c_deallocate(x.m_vars[i], indent, "implicit");
        }
        src = out;
    }

    void visit_Nullify(const ASR::Nullify_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out;
        for (size_t i = 0; i < x.n_vars; i++) {
            if (is_c && ASRUtils::is_array(ASRUtils::expr_type(x.m_vars[i]))
                    && c_array_expr_uses_descriptor(x.m_vars[i])) {
                ASR::expr_t *unwrapped_expr = unwrap_c_lvalue_expr(x.m_vars[i]);
                if (unwrapped_expr && ASR::is_a<ASR::Var_t>(*unwrapped_expr)) {
                    ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(unwrapped_expr)->m_v);
                    if (ASR::is_a<ASR::Variable_t>(*sym)) {
                        ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
                        ASR::asr_t *owner = var->m_parent_symtab
                            ? var->m_parent_symtab->asr_owner : nullptr;
                        bool has_local_backing_descriptor =
                            !ASRUtils::is_arg_dummy(var->m_intent)
                            && owner
                            && (CUtils::is_symbol_owner<ASR::Function_t>(owner)
                                || CUtils::is_symbol_owner<ASR::Program_t>(owner)
                                || CUtils::is_symbol_owner<ASR::Block_t>(owner));
                        if (has_local_backing_descriptor) {
                            std::string var_name = get_c_var_storage_name(var);
                            out += indent + var_name + " = &" + var_name + "_value;\n";
                            out += indent + "if ((" + var_name + ") != NULL) {\n"
                                + indent + "    (" + var_name + ")->data = NULL;\n"
                                + indent + "    (" + var_name + ")->offset = 0;\n"
                                + indent + "    (" + var_name + ")->is_allocated = false;\n"
                                + indent + "}\n";
                            continue;
                        }
                    }
                }
                std::string target = get_c_deallocation_target_expr(x.m_vars[i]);
                out += indent + "if ((" + target + ") != NULL) {\n"
                    + indent + "    (" + target + ")->data = NULL;\n"
                    + indent + "    (" + target + ")->offset = 0;\n"
                    + indent + "    (" + target + ")->is_allocated = false;\n"
                    + indent + "}\n";
                continue;
            }
            if (is_c && ASR::is_a<ASR::Var_t>(*x.m_vars[i])) {
                ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(x.m_vars[i])->m_v);
                ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
                ASR::ttype_t *var_value_type =
                    ASRUtils::type_get_past_allocatable_pointer(var->m_type);
                if (ASRUtils::is_allocatable(var->m_type)
                        && !ASRUtils::is_array(var->m_type)
                        && var_value_type != nullptr
                        && ASRUtils::is_character(*var_value_type)
                        && is_c_compiler_created_return_slot_expr(x.m_vars[i])) {
                    std::string target = get_c_deallocation_target_expr(x.m_vars[i]);
                    out += indent + "if (" + target + " != NULL) {\n"
                        + indent + "    _lfortran_free_alloc(_lfortran_get_default_allocator(), "
                        + target + ");\n"
                        + indent + "    " + target + " = NULL;\n"
                        + indent + "}\n";
                    continue;
                }
                bool use_raw_name = ASRUtils::is_pointer(var->m_type)
                    && !ASRUtils::is_array(var->m_type)
                    && current_function
                    && current_function->m_return_var
                    && ASR::is_a<ASR::Var_t>(*current_function->m_return_var)
                    && ASRUtils::symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(current_function->m_return_var)->m_v) == sym;
                if (use_raw_name) {
                    out += indent + CUtils::get_c_variable_name(*var)
                        + " = NULL;\n";
                    continue;
                }
            }
            self().visit_expr(*x.m_vars[i]);
            out += indent + src + " = NULL;\n";
        }
        src = out;
    }

    void visit_TypeStmtName(const ASR::TypeStmtName_t &x) {
        std::string out;
        for (size_t i = 0; i < x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            out += src;
        }
        src = out;
    }

    void visit_ClassStmt(const ASR::ClassStmt_t &x) {
        std::string out;
        for (size_t i = 0; i < x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            out += src;
        }
        src = out;
    }

    void visit_TypeStmtType(const ASR::TypeStmtType_t &x) {
        std::string out;
        for (size_t i = 0; i < x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            out += src;
        }
        src = out;
    }

    bool selector_matches_type_stmt(ASR::expr_t *selector, ASR::type_stmt_t *type_stmt) {
        ASR::symbol_t *selector_sym = ASRUtils::get_struct_sym_from_struct_expr(selector);
        if (selector_sym != nullptr) {
            selector_sym = ASRUtils::symbol_get_past_external(selector_sym);
        }
        ASR::ttype_t *selector_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(selector));

        switch (type_stmt->type) {
            case ASR::type_stmtType::TypeStmtName: {
                if (selector_sym == nullptr) return false;
                ASR::symbol_t *guard_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::TypeStmtName_t>(type_stmt)->m_sym);
                return selector_sym == guard_sym;
            }
            case ASR::type_stmtType::ClassStmt: {
                if (selector_sym == nullptr) return false;
                ASR::symbol_t *guard_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::ClassStmt_t>(type_stmt)->m_sym);
                ASR::symbol_t *current = selector_sym;
                while (current != nullptr) {
                    if (current == guard_sym) {
                        return true;
                    }
                    if (!ASR::is_a<ASR::Struct_t>(*current)) {
                        break;
                    }
                    ASR::Struct_t *current_struct = ASR::down_cast<ASR::Struct_t>(current);
                    if (current_struct->m_parent == nullptr) {
                        break;
                    }
                    current = ASRUtils::symbol_get_past_external(current_struct->m_parent);
                }
                return false;
            }
            case ASR::type_stmtType::TypeStmtType: {
                ASR::ttype_t *guard_type = ASRUtils::type_get_past_allocatable_pointer(
                    ASR::down_cast<ASR::TypeStmtType_t>(type_stmt)->m_type);
                return ASRUtils::check_equal_type(selector_type, guard_type, nullptr, nullptr);
            }
            default:
                return false;
        }
    }

    void fill_type_stmt(const ASR::SelectType_t &x,
            std::vector<ASR::type_stmt_t*> &type_stmt_order,
            ASR::type_stmtType type_stmt_type) {
        for (size_t i = 0; i < x.n_body; i++) {
            if (x.m_body[i]->type == type_stmt_type) {
                type_stmt_order.push_back(x.m_body[i]);
            }
        }
    }

    std::string emit_select_type_stmt_body(ASR::type_stmt_t *type_stmt) {
        switch (type_stmt->type) {
            case ASR::type_stmtType::TypeStmtName:
                self().visit_TypeStmtName(
                    *ASR::down_cast<ASR::TypeStmtName_t>(type_stmt));
                break;
            case ASR::type_stmtType::ClassStmt:
                self().visit_ClassStmt(
                    *ASR::down_cast<ASR::ClassStmt_t>(type_stmt));
                break;
            case ASR::type_stmtType::TypeStmtType:
                self().visit_TypeStmtType(
                    *ASR::down_cast<ASR::TypeStmtType_t>(type_stmt));
                break;
            default:
                throw CodeGenError("Unsupported type statement in SelectType");
        }
        return src;
    }

    std::string get_select_type_selector_storage_expr(ASR::expr_t *selector) {
        selector = unwrap_c_lvalue_expr(selector);
        if (selector == nullptr) {
            return "";
        }
        if (ASR::is_a<ASR::Var_t>(*selector)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(selector)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
                std::string var_name = get_c_var_storage_name(var);
                if (emits_plain_aggregate_dummy_pointee_value(selector)) {
                    return var_name;
                }
                if (is_aggregate_dummy_slot_type(var)) {
                    return "(*" + var_name + ")";
                }
                return var_name;
            }
        } else if (ASR::is_a<ASR::StructInstanceMember_t>(*selector)) {
            return get_struct_instance_member_expr(
                *ASR::down_cast<ASR::StructInstanceMember_t>(selector), false);
        }
        self().visit_expr(*selector);
        return src;
    }

    std::string get_select_type_runtime_condition(ASR::expr_t *selector,
            ASR::type_stmt_t *type_stmt) {
        selector = unwrap_c_lvalue_expr(selector);
        if (selector == nullptr) {
            return "";
        }
        ASR::ttype_t *selector_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(selector));
        if (selector_type == nullptr
                || !(ASR::is_a<ASR::StructType_t>(*selector_type)
                    || ASRUtils::is_class_type(selector_type))) {
            return "";
        }

        std::string selector_expr = get_select_type_selector_storage_expr(selector);
        if (selector_expr.empty()) {
            return "";
        }

        bool selector_is_pointer_backed = is_pointer_backed_struct_expr(selector);
        std::string tag_expr = get_runtime_type_tag_expr(selector_expr,
            selector_is_pointer_backed);
        std::string cond_prefix = selector_is_pointer_backed
            ? "((" + selector_expr + ") != NULL) && "
            : "";

        switch (type_stmt->type) {
            case ASR::type_stmtType::TypeStmtName: {
                ASR::symbol_t *guard_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::TypeStmtName_t>(type_stmt)->m_sym);
                if (!ASR::is_a<ASR::Struct_t>(*guard_sym)) {
                    return "";
                }
                return cond_prefix + "(" + tag_expr + " == "
                    + std::to_string(get_struct_runtime_type_id(guard_sym)) + ")";
            }
            case ASR::type_stmtType::ClassStmt: {
                ASR::symbol_t *guard_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::ClassStmt_t>(type_stmt)->m_sym);
                if (!ASR::is_a<ASR::Struct_t>(*guard_sym)) {
                    return "";
                }
                std::vector<std::string> type_checks;
                type_checks.push_back("(" + tag_expr + " == "
                    + std::to_string(get_struct_runtime_type_id(guard_sym)) + ")");
                std::vector<ASR::Struct_t*> derived_structs;
                std::set<uint64_t> seen;
                collect_descendant_structs(global_scope, guard_sym, derived_structs, seen);
                for (ASR::Struct_t *derived_struct: derived_structs) {
                    type_checks.push_back("(" + tag_expr + " == "
                        + std::to_string(get_struct_runtime_type_id(
                            reinterpret_cast<ASR::symbol_t*>(derived_struct))) + ")");
                }
                std::string type_cond;
                for (size_t i = 0; i < type_checks.size(); i++) {
                    if (i > 0) {
                        type_cond += " || ";
                    }
                    type_cond += type_checks[i];
                }
                return cond_prefix + "(" + type_cond + ")";
            }
            case ASR::type_stmtType::TypeStmtType:
                return "";
            default:
                return "";
        }
    }

    void visit_SelectType(const ASR::SelectType_t &x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string out;
        bool emitted_branch = false;
        std::vector<ASR::type_stmt_t*> ordered_type_stmts;
        fill_type_stmt(x, ordered_type_stmts, ASR::type_stmtType::TypeStmtName);
        fill_type_stmt(x, ordered_type_stmts, ASR::type_stmtType::TypeStmtType);
        fill_type_stmt(x, ordered_type_stmts, ASR::type_stmtType::ClassStmt);

        for (size_t i = 0; i < ordered_type_stmts.size(); i++) {
            ASR::type_stmt_t *type_stmt = ordered_type_stmts[i];
            std::string cond = get_select_type_runtime_condition(x.m_selector, type_stmt);
            if (cond.empty()) {
                if (!selector_matches_type_stmt(x.m_selector, type_stmt)) {
                    continue;
                }
                cond = "true";
            }
            out += indent + (emitted_branch ? "else if (" : "if (") + cond + ") {\n";
            indentation_level += 1;
            out += emit_select_type_stmt_body(type_stmt);
            indentation_level -= 1;
            out += indent + "}\n";
            emitted_branch = true;
            if (cond == "true") {
                break;
            }
        }

        if (x.n_default > 0) {
            if (emitted_branch) {
                out += indent + "else {\n";
                indentation_level += 1;
                for (size_t i = 0; i < x.n_default; i++) {
                    self().visit_stmt(*x.m_default[i]);
                    out += src;
                }
                indentation_level -= 1;
                out += indent + "}\n";
            } else {
                for (size_t i = 0; i < x.n_default; i++) {
                    self().visit_stmt(*x.m_default[i]);
                    out += src;
                }
            }
        } else if (!emitted_branch) {
            out += indent
                + "/* FIXME: C backend SelectType dispatch could not resolve selector type. */\n";
        }

        src = out;
    }

    void visit_Select(const ASR::Select_t& x)
    {
        std::map<std::string, CScalarExprCacheEntry> pow_cache_copy =
            current_function_pow_cache;
        current_function_pow_cache.clear();
        auto is_character_expr = [&](ASR::expr_t *expr) -> bool {
            return expr != nullptr && ASRUtils::is_character(*ASRUtils::expr_type(expr));
        };
        auto get_string_length = [&](ASR::expr_t *expr, std::string expr_src) -> std::string {
            ASR::String_t *str_type = is_character_expr(expr)
                ? ASRUtils::get_string_type(expr)
                : nullptr;
            if (str_type && str_type->m_len) {
                self().visit_expr(*str_type->m_len);
                return src;
            }
            return "strlen(" + expr_src + ")";
        };
        auto emit_select_compare = [&](ASR::expr_t *left_expr, std::string left_src,
                                       ASR::expr_t *right_expr, std::string right_src,
                                       ASR::cmpopType op,
                                       const std::string &left_len_override = "",
                                       const std::string &right_len_override = "") -> std::string {
            std::string op_str = ASRUtils::cmpop_to_str(op);
            if (is_c && is_character_expr(left_expr) && is_character_expr(right_expr)) {
                std::string left_len = left_len_override.empty()
                    ? get_string_length(left_expr, left_src) : left_len_override;
                std::string right_len = right_len_override.empty()
                    ? get_string_length(right_expr, right_src) : right_len_override;
                return "str_compare(" + left_src + ", " + left_len + ", "
                    + right_src + ", " + right_len + ") " + op_str + " 0";
            }
            return "(" + left_src + ") " + op_str + " (" + right_src + ")";
        };
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string var, var_len, select_setup;
        bool use_string_view = is_c && is_character_expr(x.m_test)
            && try_get_unit_step_string_section_view(
                x.m_test, var, var_len, select_setup);
        if (!use_string_view) {
            this->visit_expr(*x.m_test);
            var = std::move(src);
            select_setup += drain_tmp_buffer();
            select_setup += extract_stmt_setup_from_expr(var);
            if (is_c && is_character_expr(x.m_test)) {
                var_len = get_string_length(x.m_test, var);
            }
        }
        std::string out = select_setup + indent + "if (";

        for (size_t i = 0; i < x.n_body; i++) {
            if (i > 0)
                out += indent + "else if (";
            bracket_open++;
            ASR::case_stmt_t* stmt = x.m_body[i];
            if (stmt->type == ASR::case_stmtType::CaseStmt) {
                ASR::CaseStmt_t* case_stmt = ASR::down_cast<ASR::CaseStmt_t>(stmt);
                for (size_t j = 0; j < case_stmt->n_test; j++) {
                    if (j > 0)
                        out += " || ";
                    this->visit_expr(*case_stmt->m_test[j]);
                    out += emit_select_compare(x.m_test, var, case_stmt->m_test[j], src,
                                               ASR::cmpopType::Eq, var_len);
                }
                out += ") {\n";
                bracket_open--;
                indentation_level += 1;
                for (size_t j = 0; j < case_stmt->n_body; j++) {
                    this->visit_stmt(*case_stmt->m_body[j]);
                    out += check_tmp_buffer() + src;
                }
                out += indent + "}\n";
                indentation_level -= 1;
            } else {
                ASR::CaseStmt_Range_t* case_stmt_range
                    = ASR::down_cast<ASR::CaseStmt_Range_t>(stmt);
                std::string left, right;
                if (case_stmt_range->m_start) {
                    this->visit_expr(*case_stmt_range->m_start);
                    left = std::move(src);
                }
                if (case_stmt_range->m_end) {
                    this->visit_expr(*case_stmt_range->m_end);
                    right = std::move(src);
                }
                if (left.empty() && right.empty()) {
                    diag.codegen_error_label(
                        "Empty range in select statement", { x.base.base.loc }, "");
                    throw Abort();
                }
                if (left.empty()) {
                    out += emit_select_compare(x.m_test, var, case_stmt_range->m_end, right,
                                               ASR::cmpopType::LtE, var_len);
                } else if (right.empty()) {
                    out += emit_select_compare(x.m_test, var, case_stmt_range->m_start, left,
                                               ASR::cmpopType::GtE, var_len);
                } else {
                    out += "(" + emit_select_compare(case_stmt_range->m_start, left, x.m_test, var,
                                                     ASR::cmpopType::LtE, "", var_len) + ") && ("
                        + emit_select_compare(x.m_test, var, case_stmt_range->m_end, right,
                                              ASR::cmpopType::LtE, var_len) + ")";
                }
                out += ") {\n";
                bracket_open--;
                indentation_level += 1;
                for (size_t j = 0; j < case_stmt_range->n_body; j++) {
                    this->visit_stmt(*case_stmt_range->m_body[j]);
                    out += check_tmp_buffer() + src;
                }
                out += indent + "}\n";
                indentation_level -= 1;
            }
        }
        if (x.n_default) {
            out += indent + "else {\n";
            indentation_level += 1;
            for (size_t i = 0; i < x.n_default; i++) {
                this->visit_stmt(*x.m_default[i]);
                out += check_tmp_buffer() + src;
            }
            out += indent + "}\n";
            indentation_level -= 1;
        }
        src = check_tmp_buffer() + out;
        current_function_pow_cache = pow_cache_copy;
    }

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        std::map<std::string, CScalarExprCacheEntry> pow_cache_copy =
            current_function_pow_cache;
        current_function_pow_cache.clear();
        std::string indent(indentation_level*indentation_spaces, ' ');
        bracket_open++;
        std::string out = indent + "while (";
        self().visit_expr(*x.m_test);
        out += src + ") {\n";
        bracket_open--;
        out = check_tmp_buffer() + out;
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            out += check_tmp_buffer() + src;
        }
        out += indent + "}\n";
        indentation_level -= 1;
        src = out;
        current_function_pow_cache = pow_cache_copy;
    }

    void visit_Exit(const ASR::Exit_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "break;\n";
    }

    void visit_Cycle(const ASR::Cycle_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "continue;\n";
    }

    std::string get_current_return_var_name() {
        if (!current_return_var_name.empty()) {
            return current_return_var_name;
        }
        if (!current_function || !current_function->m_return_var) {
            return "";
        }
        std::string raw_name;
        if (ASR::is_a<ASR::Var_t>(*current_function->m_return_var)) {
            ASR::symbol_t *ret_sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(current_function->m_return_var)->m_v);
            raw_name = CUtils::sanitize_c_identifier(ASRUtils::symbol_name(ret_sym));
            if (ASR::is_a<ASR::Variable_t>(*ret_sym)) {
                ASR::Variable_t *ret_var = ASR::down_cast<ASR::Variable_t>(ret_sym);
                if (ret_var->m_intent == ASRUtils::intent_return_var) {
                    return CUtils::get_c_variable_name(*ret_var);
                }
            }
            if (current_function->m_symtab) {
                std::string fallback_name;
                for (auto &item: current_function->m_symtab->get_scope()) {
                    if (!ASR::is_a<ASR::Variable_t>(*item.second)) {
                        continue;
                    }
                    ASR::Variable_t *candidate = ASR::down_cast<ASR::Variable_t>(item.second);
                    std::string emitted_name = CUtils::get_c_variable_name(*candidate);
                    if (candidate->m_intent == ASRUtils::intent_return_var) {
                        return emitted_name;
                    }
                    if (!raw_name.empty()) {
                        if (emitted_name == raw_name) {
                            fallback_name = emitted_name;
                        } else if (emitted_name.size() > raw_name.size()
                                && emitted_name.compare(emitted_name.size() - raw_name.size(),
                                    raw_name.size(), raw_name) == 0
                                && emitted_name[emitted_name.size() - raw_name.size() - 1] == '_') {
                            fallback_name = emitted_name;
                        }
                    }
                }
                if (!fallback_name.empty()) {
                    return fallback_name;
                }
            }
        }
        self().visit_expr(*current_function->m_return_var);
        if (!src.empty()) {
            return src;
        }
        if (!raw_name.empty()) {
            return CUtils::sanitize_c_identifier(current_function->m_name)
                + "__" + raw_name;
        }
        return raw_name;
    }

    void visit_Return(const ASR::Return_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        if (is_c && current_function) {
            current_function_has_explicit_return = true;
            src = indent + "/* __lfortran_return_cleanup_marker__ */\n";
            if (!current_return_var_name.empty() || current_function->m_return_var) {
                src += indent + "return " + get_current_return_var_name() + ";\n";
            } else {
                src += indent + "return;\n";
            }
        } else if (current_function && (!current_return_var_name.empty() || current_function->m_return_var)) {
            src = emit_current_function_heap_array_cleanup(indent);
            src += indent + "return " + get_current_return_var_name() + ";\n";
        } else {
            src = emit_current_function_heap_array_cleanup(indent);
            src += indent + "return;\n";
        }
    }

    void visit_GoTo(const ASR::GoTo_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string goto_c_name = "__c__goto__" + CUtils::sanitize_c_identifier(x.m_name);
        src =  indent + "goto " + goto_c_name + ";\n";
        gotoid2name[x.m_target_id] = goto_c_name;
    }

    void visit_GoToTarget(const ASR::GoToTarget_t &x) {
        std::string goto_c_name = "__c__goto__" + CUtils::sanitize_c_identifier(x.m_name);
        src = goto_c_name + ":\n";
    }

    void visit_Stop(const ASR::Stop_t &x) {
        if (x.m_code) {
            self().visit_expr(*x.m_code);
        } else {
            src = "0";
        }
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "exit(" + src + ");\n";
    }

    void visit_ErrorStop(const ASR::ErrorStop_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        if (is_c) {
            src = indent + "fprintf(stderr, \"ERROR STOP\");\n";
        } else {
            src = indent + "std::cerr << \"ERROR STOP\" << std::endl;\n";
        }
        src += indent + "exit(1);\n";
    }

    void visit_SyncAll(const ASR::SyncAll_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "// SYNC ALL\n";
    }

    void visit_SyncMemory(const ASR::SyncMemory_t & /* x */) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        src = indent + "// SYNC MEMORY\n";
    }

    void visit_ImpliedDoLoop(const ASR::ImpliedDoLoop_t &/*x*/) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + " /* FIXME: implied do loop */ ";
        src = out;
        last_expr_precedence = 2;
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        std::string current_body_copy = current_body;
        std::map<std::string, CScalarExprCacheEntry> pow_cache_copy =
            current_function_pow_cache;
        current_function_pow_cache.clear();
        current_body = "";
        std::string loop_end_decl = "";
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "for (";
        ASR::symbol_t *loop_sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(x.m_head.m_v)->m_v);
        ASR::Variable_t *loop_var = ASR::down_cast<ASR::Variable_t>(loop_sym);
        std::string lvname = CUtils::get_c_variable_name(*loop_var);
        ASR::expr_t *a=x.m_head.m_start;
        ASR::expr_t *b=x.m_head.m_end;
        ASR::expr_t *c=x.m_head.m_increment;
        LCOMPILERS_ASSERT(a);
        LCOMPILERS_ASSERT(b);
        int increment;
        bool is_c_constant = false;
        if (!c) {
            increment = 1;
            is_c_constant = true;
        } else {
            ASR::expr_t* c_value = ASRUtils::expr_value(c);
            is_c_constant = ASRUtils::extract_value(c_value, increment);
        }

        if( is_c_constant ) {
            std::string cmp_op;
            if (increment > 0) {
                cmp_op = "<=";
            } else {
                cmp_op = ">=";
            }

            out += lvname + "=";
            self().visit_expr(*a);
            out += src + "; " + lvname + cmp_op;
            self().visit_expr(*b);
            out += src + "; " + lvname;
            if (increment == 1) {
                out += "++";
            } else if (increment == -1) {
                out += "--";
            } else {
                out += "+=" + std::to_string(increment);
            }
        } else {
            this->visit_expr(*c);
            std::string increment_ = std::move(src);
            self().visit_expr(*b);
            std::string do_loop_end = std::move(src);
            std::string do_loop_end_name = get_unique_local_name(
                "loop_end___" + std::to_string(loop_end_count));
            loop_end_count += 1;
            loop_end_decl = indent + CUtils::get_c_type_from_ttype_t(ASRUtils::expr_type(b), is_c) +
                            " " + do_loop_end_name + " = " + do_loop_end + ";\n";
            out += lvname + " = ";
            self().visit_expr(*a);
            out += src + "; ";
            out += "((" + increment_ + " >= 0) && (" +
                    lvname + " <= " + do_loop_end_name + ")) || (("
                    + increment_ + " < 0) && (" + lvname + " >= "
                    + do_loop_end_name + ")); " + lvname;
            out += " += " + increment_;
        }

        out += ") {\n";
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            current_body += src;
        }
        out += current_body;
        out += indent + "}\n";
        indentation_level -= 1;
        src = loop_end_decl + out;
        current_body = current_body_copy;
        current_function_pow_cache = pow_cache_copy;
    }

    void visit_If(const ASR::If_t &x) {
        std::string current_body_copy = current_body;
        std::map<std::string, CScalarExprCacheEntry> pow_cache_copy =
            current_function_pow_cache;
        current_function_pow_cache.clear();
        current_body = "";
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent + "if (";
        bracket_open++;
        self().visit_expr(*x.m_test);
        out += src + ") {\n";
        bracket_open--;
        out = check_tmp_buffer() + out;
        indentation_level += 1;
        for (size_t i=0; i<x.n_body; i++) {
            self().visit_stmt(*x.m_body[i]);
            current_body += check_tmp_buffer() + src;
        }
        out += current_body;
        out += indent + "}";
        if (x.n_orelse == 0) {
            out += "\n";
        } else {
            current_body = "";
            out += " else {\n";
            for (size_t i=0; i<x.n_orelse; i++) {
                self().visit_stmt(*x.m_orelse[i]);
                current_body += check_tmp_buffer() + src;
            }
            out += current_body;
            out += indent + "}\n";
        }
        indentation_level -= 1;
        src = out;
        current_body = current_body_copy;
        current_function_pow_cache = pow_cache_copy;
    }

    void visit_IfExp(const ASR::IfExp_t &x) {
        // IfExp is like a ternary operator in c++
        // test ? body : orelse;
        CHECK_FAST_C_CPP(compiler_options, x)
        std::string out = "(";
        self().visit_expr(*x.m_test);
        out += src + ") ? (";
        self().visit_expr(*x.m_body);
        out += src + ") : (";
        self().visit_expr(*x.m_orelse);
        out += src + ")";
        src = out;
        last_expr_precedence = 16;
    }

    std::string emit_c_subroutine_call_struct_temp_root_cleanup(
            const ASR::SubroutineCall_t &x, const std::string &indent) {
        std::string cleanup;
        std::set<std::string> cleaned;
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::expr_t *arg = x.m_args[i].m_value;
            if (arg == nullptr) {
                continue;
            }
            arg = unwrap_c_lvalue_expr(arg);
            if (arg == nullptr || !ASR::is_a<ASR::Var_t>(*arg)) {
                continue;
            }
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(arg)->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                continue;
            }
            ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
            std::string var_name(var->m_name);
            if (var_name.rfind("__libasr_created__subroutine_call_", 0) != 0) {
                continue;
            }
            ASR::ttype_t *var_type = var->m_type;
            ASR::ttype_t *var_type_unwrapped =
                ASRUtils::type_get_past_allocatable_pointer(var_type);
            if (!ASRUtils::is_allocatable(var_type)
                    || ASRUtils::is_array(var_type)
                    || var_type_unwrapped == nullptr
                    || !ASR::is_a<ASR::StructType_t>(*var_type_unwrapped)) {
                continue;
            }
            std::string target = CUtils::get_c_variable_name(*var);
            if (!cleaned.insert(target).second) {
                continue;
            }
            ASR::Struct_t *struct_t = nullptr;
            if (var->m_type_declaration != nullptr) {
                ASR::symbol_t *struct_sym = ASRUtils::symbol_get_past_external(
                    var->m_type_declaration);
                if (struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                    struct_t = ASR::down_cast<ASR::Struct_t>(struct_sym);
                }
            }
            cleanup += emit_c_shallow_copied_struct_root_cleanup(
                struct_t, indent, target, ASRUtils::is_class_type(var_type_unwrapped));
        }
        return cleanup;
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        clear_c_pow_cache();
        std::string indent(indentation_level*indentation_spaces, ' ');
        ASR::symbol_t *callee_sym = ASRUtils::symbol_get_past_external(x.m_name);
        if (is_c) {
            std::string deferred_dispatch;
            if (build_deferred_struct_method_dispatch(x.m_name, x.n_args, x.m_args,
                    deferred_dispatch, true)) {
                src = indent + deferred_dispatch;
                return;
            }
        }
        ASR::Function_t *s = get_procedure_interface_function(x.m_name);
        if (!s) {
            throw CodeGenError("Unsupported subroutine call target", x.base.base.loc);
        }
        std::string sym_name;
        if (ASR::is_a<ASR::Variable_t>(*callee_sym)) {
            if (is_c && x.m_dt != nullptr) {
                sym_name = get_procedure_component_callee_expr(x.m_dt, callee_sym);
            } else {
                sym_name = CUtils::get_c_variable_name(
                    *ASR::down_cast<ASR::Variable_t>(callee_sym));
            }
        } else {
            record_forward_decl_for_function(*s);
            sym_name = get_c_function_target_name(*s);
        }
        std::string function_result_slot_setup;
        if (is_c && x.n_args > 0) {
            ASR::expr_t *result_slot = x.m_args[x.n_args - 1].m_value;
            ASR::Variable_t *return_var = nullptr;
            if (s->m_return_var != nullptr) {
                return_var = ASRUtils::EXPR2VAR(s->m_return_var);
            } else if (x.n_args <= s->n_args && s->m_args[x.n_args - 1] != nullptr
                    && ASR::is_a<ASR::Var_t>(*s->m_args[x.n_args - 1])) {
                ASR::symbol_t *formal_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(s->m_args[x.n_args - 1])->m_v);
                if (ASR::is_a<ASR::Variable_t>(*formal_sym)) {
                    ASR::Variable_t *candidate =
                        ASR::down_cast<ASR::Variable_t>(formal_sym);
                    ASR::ttype_t *candidate_value_type =
                        ASRUtils::type_get_past_allocatable_pointer(candidate->m_type);
                    if ((candidate->m_intent == ASRUtils::intent_return_var
                            || candidate->m_intent == ASRUtils::intent_out
                            || candidate->m_intent == ASRUtils::intent_inout)
                            && candidate_value_type != nullptr
                            && ASR::is_a<ASR::StructType_t>(*candidate_value_type)
                            && !ASRUtils::is_allocatable(candidate->m_type)
                            && !ASRUtils::is_array(candidate->m_type)) {
                        return_var = candidate;
                    }
                }
            }
            ASR::ttype_t *result_slot_type = result_slot != nullptr
                ? ASRUtils::expr_type(result_slot) : nullptr;
            ASR::ttype_t *result_slot_value_type =
                ASRUtils::type_get_past_allocatable_pointer(result_slot_type);
            ASR::ttype_t *return_value_type =
                return_var != nullptr
                    ? ASRUtils::type_get_past_allocatable_pointer(return_var->m_type)
                    : nullptr;
            ASR::symbol_t *return_struct_sym = return_var != nullptr
                ? ASRUtils::symbol_get_past_external(return_var->m_type_declaration)
                : nullptr;
            if (return_struct_sym == nullptr && s->m_return_var != nullptr) {
                return_struct_sym = ASRUtils::symbol_get_past_external(
                    ASRUtils::get_struct_sym_from_struct_expr(s->m_return_var));
            }
            if (return_var != nullptr
                    && result_slot != nullptr
                    && result_slot_type != nullptr
                    && ASRUtils::is_allocatable(result_slot_type)
                    && !ASRUtils::is_array(result_slot_type)
                    && result_slot_value_type != nullptr
                    && return_value_type != nullptr
                    && ASR::is_a<ASR::StructType_t>(*result_slot_value_type)
                    && ASR::is_a<ASR::StructType_t>(*return_value_type)
                    && !ASR::down_cast<ASR::StructType_t>(
                        result_slot_value_type)->m_is_unlimited_polymorphic
                    && return_struct_sym != nullptr
                    && ASR::is_a<ASR::Struct_t>(*return_struct_sym)) {
                ASR::symbol_t *target_type_decl =
                    get_expr_type_declaration_symbol(result_slot);
                if (target_type_decl == nullptr) {
                    target_type_decl = return_struct_sym;
                }
                std::string target_concrete_type = get_c_concrete_type_from_ttype_t(
                    result_slot_value_type, target_type_decl);
                std::string allocation_type = "struct "
                    + CUtils::get_c_symbol_name(return_struct_sym);
                if (!target_concrete_type.empty() && target_concrete_type != "void*") {
                    headers.insert("string.h");
                    std::string target = get_c_deallocation_target_expr(result_slot);
                    function_result_slot_setup += drain_tmp_buffer();
                    std::string result_type_id =
                        std::to_string(get_struct_runtime_type_id(return_struct_sym));
                    std::string old_type_id_name =
                        get_unique_local_name("__lfortran_result_slot_type_id");
                    function_result_slot_setup += indent + "if (" + target + " != NULL) {\n";
                    function_result_slot_setup += indent + std::string(indentation_spaces, ' ')
                        + "int64_t " + old_type_id_name + " = "
                        + get_runtime_type_tag_expr(target, true) + ";\n";
                    function_result_slot_setup += indent + std::string(indentation_spaces, ' ')
                        + "_lfortran_cleanup_c_struct(" + old_type_id_name
                        + ", (void*) " + target + ");\n";
                    function_result_slot_setup += indent + std::string(indentation_spaces, ' ')
                        + "if (" + old_type_id_name + " != " + result_type_id + ") {\n";
                    function_result_slot_setup += indent + std::string(2 * indentation_spaces, ' ')
                        + "_lfortran_free_alloc(_lfortran_get_default_allocator(), (char*) "
                        + target + ");\n";
                    function_result_slot_setup += indent + std::string(2 * indentation_spaces, ' ')
                        + target + " = NULL;\n";
                    function_result_slot_setup += indent + std::string(indentation_spaces, ' ')
                        + "}\n";
                    function_result_slot_setup += indent + "}\n";
                    function_result_slot_setup += indent + "if (" + target + " == NULL) {\n";
                    function_result_slot_setup += indent + std::string(indentation_spaces, ' ')
                        + target + " = (" + target_concrete_type
                        + "*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof("
                        + allocation_type + "));\n";
                    function_result_slot_setup += indent + std::string(indentation_spaces, ' ')
                        + "memset(" + target + ", 0, sizeof(" + allocation_type + "));\n";
                    function_result_slot_setup += indent + std::string(indentation_spaces, ' ')
                        + get_runtime_type_tag_expr(target, true) + " = "
                        + result_type_id + ";\n";
                    function_result_slot_setup += indent + "}\n";
                }
            }
        }
        bool callee_is_procedure_variable = ASR::is_a<ASR::Variable_t>(*callee_sym);
        std::string call_args;
        if (is_c && !callee_is_procedure_variable) {
            std::vector<size_t> pass_array_by_data_indices;
            std::string direct_target_name;
            if (get_pass_array_by_data_direct_call_target(
                    *s, direct_target_name, pass_array_by_data_indices)
                    && try_construct_pass_array_by_data_direct_call_args(
                        s, x.n_args, x.m_args, pass_array_by_data_indices, call_args)) {
                record_pass_array_by_data_direct_call_decl(
                    *s, direct_target_name, pass_array_by_data_indices);
                sym_name = direct_target_name;
            } else if (try_construct_specialized_pass_array_no_copy_call_args(
                    s, x.n_args, x.m_args, call_args)) {
            } else {
                call_args = construct_call_args(
                    s, x.n_args, x.m_args, false, true, true);
            }
        } else {
            call_args = construct_call_args(
                s, x.n_args, x.m_args, callee_is_procedure_variable, true, true);
        }
        src = drain_tmp_buffer() + function_result_slot_setup
            + indent + sym_name + "(" + call_args + ");\n";
        if (is_c && x.n_args > 0
                && std::string(s->m_name).rfind("_lcompilers_move_alloc_", 0) == 0) {
            src += emit_move_alloc_source_reset(x.m_args[0].m_value, indent);
        }
        if (is_c) {
            src += emit_c_subroutine_call_struct_temp_root_cleanup(x, indent);
        }
    }

    void visit_ForEach(const ASR::ForEach_t &x) {
        std::string current_body_copy = current_body;
        current_body = "";
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = check_tmp_buffer();
        ASR::ttype_t *container_type = ASRUtils::expr_type(x.m_container);
        ASR::ttype_t *container_type_past_alloc = ASRUtils::type_get_past_allocatable(container_type);

        self().visit_expr(*x.m_var);
        std::string iter_var = src;
        self().visit_expr(*x.m_container);
        std::string container = src;

        auto emit_body = [&](const std::string &assign_stmt) {
            std::string body_out = indent + std::string(indentation_spaces, ' ') + assign_stmt + "\n";
            indentation_level += 1;
            for (size_t i=0; i<x.n_body; i++) {
                self().visit_stmt(*x.m_body[i]);
                current_body += check_tmp_buffer() + src;
            }
            body_out += current_body;
            indentation_level -= 1;
            current_body.clear();
            return body_out;
        };

        if (ASR::is_a<ASR::List_t>(*container_type_past_alloc)) {
            std::string idx_name = get_unique_local_name("foreach_i");
            out += indent + "for (int32_t " + idx_name + " = 0; " + idx_name + " < "
                + container + ".current_end_point; " + idx_name + "++) {\n";
            out += emit_body(iter_var + " = " + container + ".data[" + idx_name + "];");
            out += indent + "}\n";
        } else if (ASR::is_a<ASR::Tuple_t>(*container_type_past_alloc)) {
            ASR::Tuple_t *tuple_type = ASR::down_cast<ASR::Tuple_t>(container_type_past_alloc);
            for (size_t i = 0; i < tuple_type->n_type; i++) {
                out += indent + "{\n";
                out += emit_body(iter_var + " = " + container + ".element_" + std::to_string(i) + ";");
                out += indent + "}\n";
            }
        } else if (ASRUtils::is_array(container_type)) {
            ASR::dimension_t* m_dims = nullptr;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(container_type, m_dims);
            std::string idx_name = get_unique_local_name("foreach_i");
            std::string total_name = get_unique_local_name("foreach_n");
            out += indent + "int32_t " + total_name + " = 1;\n";
            std::string container_desc = container;
            if (!ASR::is_a<ASR::Array_t>(*container_type_past_alloc)) {
                container_desc += "->";
            } else {
                container_desc += ".";
            }
            for (int i = 0; i < n_dims; i++) {
                out += indent + total_name + " *= " + container + "->dims[" + std::to_string(i) + "].length;\n";
            }
            out += indent + "for (int32_t " + idx_name + " = 0; " + idx_name + " < "
                + total_name + "; " + idx_name + "++) {\n";
            out += emit_body(iter_var + " = " + container + "->data[" + idx_name + "];");
            out += indent + "}\n";
        } else {
            throw CodeGenError("ForEach container type not supported in C backend: " +
                ASRUtils::type_to_str_python_expr(container_type, x.m_container), x.base.base.loc);
        }

        src = out;
        current_body = current_body_copy;
    }

    #define SET_INTRINSIC_NAME(X, func_name)                                    \
        case (static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::X)) : {  \
            out += func_name; break;                                            \
        }

    #define SET_INTRINSIC_SUBROUTINE_NAME(X, func_name)                                    \
        case (static_cast<int64_t>(ASRUtils::IntrinsicImpureSubroutines::X)) : {  \
            out += func_name; break;                                            \
        }

    void visit_IntrinsicElementalFunction(const ASR::IntrinsicElementalFunction_t &x) {
        if (is_c && x.m_intrinsic_id == static_cast<int64_t>(
                ASRUtils::IntrinsicElementalFunctions::Char)) {
            visit_expr_without_c_pow_cache(*x.m_args[0]);
            src = "_lfortran_str_chr_alloc(_lfortran_get_default_allocator(), " + src + ")";
            return;
        }
        if (is_c && x.m_intrinsic_id == static_cast<int64_t>(
                ASRUtils::IntrinsicElementalFunctions::Repeat)) {
            visit_expr_without_c_pow_cache(*x.m_args[0]);
            std::string s = src;
            std::string setup;
            std::string s_len = get_string_length_expr(x.m_args[0], s, setup);
            visit_expr_without_c_pow_cache(*x.m_args[1]);
            std::string n = src;
            src = setup + "_lfortran_strrepeat_c_len_alloc(_lfortran_get_default_allocator(), "
                + s + ", " + s_len + ", " + n + ")";
            return;
        }
        if (compiler_options.po.fast && x.m_value != nullptr) {
            visit_expr_without_c_pow_cache(*x.m_value);
            return;
        }
        if (x.m_intrinsic_id == static_cast<int64_t>(
                ASRUtils::IntrinsicElementalFunctions::CompilerVersion)) {
            LCOMPILERS_ASSERT(x.m_value);
            self().visit_expr(*x.m_value);
            return;
        }
        std::string out;
        std::string indent(4, ' ');
        switch (x.m_intrinsic_id) {
            SET_INTRINSIC_NAME(Sin, "sin");
            SET_INTRINSIC_NAME(Cos, "cos");
            SET_INTRINSIC_NAME(Tan, "tan");
            SET_INTRINSIC_NAME(Asin, "asin");
            SET_INTRINSIC_NAME(Acos, "acos");
            SET_INTRINSIC_NAME(Atan, "atan");
            SET_INTRINSIC_NAME(Sinh, "sinh");
            SET_INTRINSIC_NAME(Cosh, "cosh");
            SET_INTRINSIC_NAME(Tanh, "tanh");
            case (static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::Abs)) : {
                ASR::ttype_t *t = ASRUtils::expr_type(x.m_args[0]);
                visit_expr_without_c_pow_cache(*x.m_args[0]);
                headers.insert("math.h");
                if (ASRUtils::is_real(*t)) {
                    src = "fabs(" + src + ")";
                } else {
                    src = "abs(" + src + ")";
                }
                return;
            }
            SET_INTRINSIC_NAME(Exp, "exp");
            SET_INTRINSIC_NAME(Exp2, "exp2");
            SET_INTRINSIC_NAME(Expm1, "expm1");
            SET_INTRINSIC_NAME(Trunc, "trunc");
            SET_INTRINSIC_NAME(Fix, "fix");
            SET_INTRINSIC_NAME(FloorDiv, "floordiv");
            SET_INTRINSIC_NAME(Char, "char");
            SET_INTRINSIC_NAME(StringContainsSet, "verify");
            SET_INTRINSIC_NAME(StringFindSet, "scan");
            SET_INTRINSIC_NAME(SubstrIndex, "index");
            SET_INTRINSIC_NAME(StringLenTrim, "len_trim");
            SET_INTRINSIC_NAME(StringTrim, "trim");
            case (static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::FMA)) : {
                visit_expr_without_c_pow_cache(*x.m_args[0]);
                std::string a = src;
                visit_expr_without_c_pow_cache(*x.m_args[1]);
                std::string b = src;
                visit_expr_without_c_pow_cache(*x.m_args[2]);
                std::string c = src;
                src = a +" + "+ b +"*"+ c;
                return;
            }
            case (static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::Max)) : {
                ASR::ttype_t *t = ASRUtils::expr_type(x.m_args[0]);
                visit_expr_without_c_pow_cache(*x.m_args[0]);
                std::string result = src;
                for (size_t i = 1; i < x.n_args; i++) {
                    visit_expr_without_c_pow_cache(*x.m_args[i]);
                    if (ASRUtils::is_real(*t)) {
                        headers.insert("math.h");
                        result = "fmax(" + result + ", " + src + ")";
                    } else {
                        // Integer: use ternary
                        result = "((" + result + ") > (" + src + ") ? (" + result + ") : (" + src + "))";
                    }
                }
                src = result;
                return;
            }
            case (static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::Min)) : {
                ASR::ttype_t *t = ASRUtils::expr_type(x.m_args[0]);
                visit_expr_without_c_pow_cache(*x.m_args[0]);
                std::string result = src;
                for (size_t i = 1; i < x.n_args; i++) {
                    visit_expr_without_c_pow_cache(*x.m_args[i]);
                    if (ASRUtils::is_real(*t)) {
                        headers.insert("math.h");
                        result = "fmin(" + result + ", " + src + ")";
                    } else {
                        // Integer: use ternary
                        result = "((" + result + ") < (" + src + ") ? (" + result + ") : (" + src + "))";
                    }
                }
                src = result;
                return;
            }
            case (static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::Present)) : {
                ASR::expr_t *arg_expr = x.m_args[0];
                LCOMPILERS_ASSERT(ASR::is_a<ASR::Var_t>(*arg_expr));
                ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(arg_expr)->m_v);
                LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*arg_sym));
                ASR::Variable_t *arg_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                src = "(" + get_c_var_storage_name(arg_var) + " != NULL)";
                return;
            }
            default : {
                throw LCompilersException("IntrinsicElementalFunction: `"
                    + ASRUtils::get_intrinsic_name(x.m_intrinsic_id)
                    + "` is not implemented");
            }
        }
        headers.insert("math.h");
        visit_expr_without_c_pow_cache(*x.m_args[0]);
        out += "(" + src + ")";
        src = out;
    }

    void visit_TypeInquiry(const ASR::TypeInquiry_t &x) {
        this->visit_expr(*x.m_value);
    }

    void visit_IntrinsicImpureFunction(const ASR::IntrinsicImpureFunction_t &x) {
        switch (static_cast<ASRUtils::IntrinsicImpureFunctions>(x.m_impure_intrinsic_id)) {
            case ASRUtils::IntrinsicImpureFunctions::IsIostatEnd: {
                self().visit_expr(*x.m_args[0]);
                src = "(" + src + " == -1)";
                return;
            }
            case ASRUtils::IntrinsicImpureFunctions::IsIostatEor: {
                self().visit_expr(*x.m_args[0]);
                src = "(" + src + " == -2)";
                return;
            }
            case ASRUtils::IntrinsicImpureFunctions::Allocated: {
                ASR::expr_t *arg = x.m_args[0];
                ASR::ttype_t *arg_type = ASRUtils::expr_type(arg);
                ASR::ttype_t *arg_base_type = ASRUtils::type_get_past_allocatable_pointer(arg_type);
                auto is_unlimited_polymorphic_storage_type = [&](ASR::ttype_t *type) -> bool {
                    type = ASRUtils::type_get_past_allocatable_pointer(type);
                    if (type == nullptr) {
                        return false;
                    }
                    if (ASRUtils::is_array(type)) {
                        type = ASRUtils::type_get_past_array(type);
                    }
                    return type != nullptr
                        && ASR::is_a<ASR::StructType_t>(*type)
                        && ASR::down_cast<ASR::StructType_t>(type)->m_is_unlimited_polymorphic;
                };
                auto is_unlimited_polymorphic_dummy_slot_type = [&](ASR::Variable_t *var) -> bool {
                    return var != nullptr
                        && is_aggregate_dummy_slot_type(var)
                        && is_unlimited_polymorphic_storage_type(var->m_type);
                };
                bool is_scalar_allocatable = ASR::is_a<ASR::Allocatable_t>(
                    *ASRUtils::type_get_past_array(arg_type))
                    && !ASRUtils::is_array(arg_type);
                std::string arg_src;
                if (is_c && ASR::is_a<ASR::Var_t>(*arg)) {
                    ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(arg)->m_v);
                    if (ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                        ASR::Variable_t *arg_var = ASR::down_cast<ASR::Variable_t>(arg_sym);
                        if (is_scalar_allocatable_dummy_slot_type(arg_var)) {
                            arg_src = "(*" + get_c_var_storage_name(arg_var) + ")";
                        } else if (is_scalar_allocatable_storage_type(arg_var->m_type)) {
                            arg_src = get_c_var_storage_name(arg_var);
                        } else if (is_unlimited_polymorphic_dummy_slot_type(arg_var)) {
                            arg_src = "(*((void**)" + CUtils::get_c_variable_name(*arg_var) + "))";
                        } else if (is_aggregate_dummy_slot_type(arg_var)) {
                            arg_src = "(*" + CUtils::get_c_variable_name(*arg_var) + ")";
                        }
                    }
                } else if (is_c && ASR::is_a<ASR::StructInstanceMember_t>(*arg)) {
                    ASR::StructInstanceMember_t *member_arg =
                        ASR::down_cast<ASR::StructInstanceMember_t>(arg);
                    ASR::symbol_t *member_sym = ASRUtils::symbol_get_past_external(member_arg->m_m);
                    if (ASR::is_a<ASR::Variable_t>(*member_sym)
                            && is_scalar_allocatable_storage_type(
                                ASR::down_cast<ASR::Variable_t>(member_sym)->m_type)) {
                        arg_src = get_struct_instance_member_expr(*member_arg, false);
                    }
                }
                if (arg_src.empty()) {
                    self().visit_expr(*arg);
                    arg_src = std::move(src);
                }
                if (is_unlimited_polymorphic_storage_type(arg_type)) {
                    src = "((" + arg_src + ") != NULL)";
                } else if (ASRUtils::is_array(arg_type)) {
                    src = "((" + arg_src + ") != NULL && (" + arg_src + ")->data != NULL)";
                } else if (ASRUtils::is_character(*arg_base_type)) {
                    src = "((" + arg_src + ") != NULL)";
                } else if (is_scalar_allocatable) {
                    src = "((" + arg_src + ") != NULL)";
                } else {
                    src = "((" + arg_src + ") != NULL)";
                }
                return;
            }
            default: {
                throw CodeGenError("IntrinsicImpureFunction not implemented for " +
                    ASRUtils::get_impure_intrinsic_name(x.m_impure_intrinsic_id));
            }
        }
    }

    void visit_StringPhysicalCast(const ASR::StringPhysicalCast_t &x) {
        self().visit_expr(*x.m_arg);
    }

    void visit_BitCast(const ASR::BitCast_t &x) {
        ASR::expr_t *src_expr = x.m_value ? x.m_value : x.m_source;
        ASR::ttype_t *target_type = x.m_type;
        ASR::ttype_t *source_type = src_expr ? ASRUtils::expr_type(src_expr) : nullptr;
        if (src_expr && source_type && ASR::is_a<ASR::ArrayItem_t>(*src_expr)) {
            ASR::ArrayItem_t *array_item = ASR::down_cast<ASR::ArrayItem_t>(src_expr);
            source_type = ASRUtils::type_get_past_array(ASRUtils::expr_type(array_item->m_v));
        }
        if (is_c && src_expr && target_type
                && ASRUtils::is_character(*target_type)
                && source_type
                && (ASRUtils::is_integer(*source_type)
                    || ASRUtils::is_unsigned_integer(*source_type))) {
            self().visit_expr(*src_expr);
            std::string source = src;
            std::string source_type_name = CUtils::get_c_type_from_ttype_t(source_type);
            std::string target_len = "sizeof(" + source_type_name + ")";
            int64_t fixed_len = -1;
            ASR::String_t *str_type = ASRUtils::get_string_type(target_type);
            if (str_type && str_type->m_len
                    && ASRUtils::extract_value(str_type->m_len, fixed_len)) {
                target_len = std::to_string(fixed_len);
            }
            src = "_lfortran_transfer_scalar_to_string_alloc(_lfortran_get_default_allocator(), "
                "&((" + source_type_name + "){" + source + "}), sizeof("
                + source_type_name + "), " + target_len + ")";
            return;
        }
        if (is_c && src_expr && target_type && source_type
                && !ASRUtils::is_array(target_type)
                && ASRUtils::is_character(*source_type)
                && (ASRUtils::is_integer(*target_type)
                    || ASRUtils::is_unsigned_integer(*target_type)
                    || ASRUtils::is_real(*target_type)
                    || ASRUtils::is_logical(*target_type))) {
            self().visit_expr(*src_expr);
            std::string source = src;
            std::string target_c_type = CUtils::get_c_type_from_ttype_t(target_type);
            std::string target_code = CUtils::get_c_type_code(target_type, true, false);
            src = c_utils_functions->get_bitcast_string_to_scalar(target_c_type, target_code)
                + "(" + source + ")";
            return;
        }
        if (is_c && src_expr && target_type && source_type
                && ASRUtils::is_array(target_type)
                && (!ASRUtils::is_array(source_type)
                    || ASR::is_a<ASR::ArrayItem_t>(*src_expr))) {
            if (is_len_one_character_array_type(target_type)
                    && (ASRUtils::is_integer(*source_type)
                        || ASRUtils::is_unsigned_integer(*source_type)
                        || ASRUtils::is_real(*source_type)
                        || ASRUtils::is_logical(*source_type))) {
                size_t nbytes = ASRUtils::get_fixed_size_of_array(target_type);
                std::string target_code = CUtils::get_c_type_code(target_type, true, false);
                std::string target_type_name = get_c_array_wrapper_type_name(target_type);
                std::string source_type_name = CUtils::get_c_type_from_ttype_t(source_type);
                std::string source_code = CUtils::get_c_type_code(source_type, false, false);
                self().visit_expr(*src_expr);
                src = c_utils_functions->get_bitcast_scalar_to_char_array(
                    target_type_name, source_type_name, source_code, target_code, nbytes) + "(" + src + ")";
                return;
            }
        }
        if (x.m_value) {
            self().visit_expr(*x.m_value);
        } else if (x.m_source) {
            self().visit_expr(*x.m_source);
        }
    }

    void visit_RealSqrt(const ASR::RealSqrt_t &x) {
        std::string out = "sqrt";
        headers.insert("math.h");
        this->visit_expr(*x.m_arg);
        out += "(" + src + ")";
        src = out;
    }

    void visit_DebugCheckArrayBounds(const ASR::DebugCheckArrayBounds_t& /*x*/) {
        src = "";
    }

};

} // namespace LCompilers

#endif // LFORTRAN_ASR_TO_C_CPP_H
