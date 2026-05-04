#include <fstream>
#include <filesystem>
#include <memory>
#include <sstream>

#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/codegen/asr_to_c.h>
#include <libasr/codegen/asr_to_c_cpp.h>
#include <libasr/codegen/c_utils.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/pass/unused_functions.h>
#include <libasr/pass/replace_class_constructor.h>
#include <libasr/pass/replace_array_op.h>
#include <libasr/pass/create_subroutine_from_function.h>
#include <libasr/pass/intrinsic_array_function_registry.h>

#include <map>
#include <set>
#include <unordered_map>
#include <utility>

#define CHECK_FAST_C(compiler_options, x)                         \
        if (compiler_options.po.fast && x.m_value != nullptr) {    \
            visit_expr(*x.m_value);                             \
            return;                                             \
        }                                                       \

namespace LCompilers {

class ASRToCVisitor : public BaseCCPPVisitor<ASRToCVisitor>
{
public:

    int counter;
    std::set<std::string> emitted_aggregate_names;
    std::unordered_map<const ASR::symbol_t*, std::string> enum_name_map;
    std::string string_concat_helper_name;

    bool target_offload_enabled;
    std::vector<std::string> kernel_func_names;
    int kernel_counter=0; // To generate unique kernel names
    std::string current_kernel_name; // Track current kernel for wrapper
    std::vector<std::pair<std::string, ASR::OMPMap_t*>> map_vars; // Track map vars for target offload
    std::string indent() {
        return std::string(indentation_level * indentation_spaces, ' ');
    }

    bool prepare_string_readback_target(ASR::expr_t *value_expr, const std::string &value_len,
            std::string &tmp_name, std::string &setup, std::string &post) {
        ASR::expr_t *target_expr = unwrap_c_lvalue_expr(value_expr);
        ASR::expr_t *string_arg = nullptr;
        std::string left, right, step, left_present, right_present;
        if (target_expr != nullptr && ASR::is_a<ASR::StringSection_t>(*target_expr)) {
            ASR::StringSection_t *ss = ASR::down_cast<ASR::StringSection_t>(target_expr);
            string_arg = ss->m_arg;
            if (ss->m_start) {
                this->visit_expr(*ss->m_start);
                left = src;
                left_present = "true";
            } else {
                left = "0";
                left_present = "false";
            }
            if (ss->m_end) {
                this->visit_expr(*ss->m_end);
                right = src;
                right_present = "true";
            } else {
                right = "0";
                right_present = "false";
            }
            if (ss->m_step) {
                this->visit_expr(*ss->m_step);
                step = src;
            } else {
                step = "1";
            }
        } else if (target_expr != nullptr && ASR::is_a<ASR::StringItem_t>(*target_expr)) {
            ASR::StringItem_t *si = ASR::down_cast<ASR::StringItem_t>(target_expr);
            string_arg = si->m_arg;
            this->visit_expr(*si->m_idx);
            left = src;
            right = left;
            step = "1";
            left_present = "true";
            right_present = "true";
        } else {
            return false;
        }

        tmp_name = get_unique_local_name("__lfortran_read_str_tmp");
        setup += indent() + "char* " + tmp_name
            + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
            + value_len + ");\n";

        this->visit_expr(*string_arg);
        std::string string_value = src;
        std::string string_len = "((" + string_value + ") != NULL ? strlen(" + string_value + ") : 0)";
        std::string updated_value =
            "_lfortran_str_slice_assign_alloc(_lfortran_get_default_allocator(), "
            + string_value + ", " + string_len + ", "
            + tmp_name + ", " + value_len + ", "
            + left + ", " + right + ", " + step + ", "
            + left_present + ", " + right_present + ")";
        post += indent() + c_ds_api->get_deepcopy(ASRUtils::expr_type(string_arg),
            updated_value, string_value) + "\n";
        return true;
    }
    std ::string kernel_func_code;

    std::string serialize_fortran_format_type(ASR::expr_t *expr, ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_allocatable(ASRUtils::type_get_past_pointer(type));
        if (ASR::is_a<ASR::Integer_t>(*type)) {
            return "I" + std::to_string(ASRUtils::extract_kind_from_ttype_t(type));
        }
        if (ASR::is_a<ASR::UnsignedInteger_t>(*type)) {
            return "U" + std::to_string(ASRUtils::extract_kind_from_ttype_t(type));
        }
        if (ASR::is_a<ASR::Real_t>(*type)) {
            return "R" + std::to_string(ASRUtils::extract_kind_from_ttype_t(type));
        }
        if (ASR::is_a<ASR::Logical_t>(*type)) {
            int kind = ASRUtils::extract_kind_from_ttype_t(type);
            return "L" + std::to_string(kind * 8);
        }
        if (ASR::is_a<ASR::String_t>(*type)) {
            std::string res = "S-";
            ASR::String_t *str_type = ASR::down_cast<ASR::String_t>(type);
            if (str_type->m_physical_type == ASR::DescriptorString) {
                res += "DESC";
            } else if (str_type->m_physical_type == ASR::CChar) {
                res += "CCHAR";
            } else {
                throw CodeGenError("C backend FileWrite does not support this string representation in Fortran formatting",
                    expr->base.loc);
            }
            int64_t len = 0;
            if (str_type->m_len && ASRUtils::extract_value(str_type->m_len, len)) {
                res += "-" + std::to_string(len);
            }
            return res;
        }
        if (ASR::is_a<ASR::Complex_t>(*type)) {
            std::string kind = std::to_string(ASRUtils::extract_kind_from_ttype_t(type));
            return "{R" + kind + ",R" + kind + "}";
        }
        if (ASR::is_a<ASR::CPtr_t>(*type)) {
            return "CPtr";
        }
        throw CodeGenError("C backend FileWrite does not support Fortran formatting for `" +
            ASRUtils::type_to_str_fortran_expr(type, expr) + "` values",
            expr->base.loc);
    }

    std::string serialize_fortran_format_args(ASR::expr_t **args, size_t n_args) {
        std::string serialization;
        for (size_t i = 0; i < n_args; i++) {
            if (i != 0) {
                serialization += ",";
            }
            serialization += serialize_fortran_format_type(args[i], ASRUtils::expr_type(args[i]));
        }
        return serialization;
    }

    ASRToCVisitor(diag::Diagnostics &diag, CompilerOptions &co,
                  int64_t default_lower_bound)
         : BaseCCPPVisitor(diag, co.platform, co, false, false, true, default_lower_bound),
           counter{0} {
           target_offload_enabled = co.target_offload_enabled;
           }

    std::string get_default_head() const {
        std::string head =
R"(
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <lfortran_intrinsics.h>

)";
        if(compiler_options.target_offload_enabled) {
            head += R"(
#ifdef USE_GPU
#include<cuda_runtime.h>
#else
#include"cuda_cpu_runtime.h"
#endif
)";
        }
        return head;
    }

    std::string get_include_block() const {
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
        return to_include;
    }

    void finalize_common_sections(std::string &helper_defs) {
        if( c_ds_api->get_func_decls().size() > 0 ) {
            array_types_decls += "\n" + c_ds_api->get_func_decls() + "\n";
        }
        if( c_utils_functions->get_util_func_decls().size() > 0 ) {
            array_types_decls += "\n" + c_utils_functions->get_util_func_decls() + "\n";
        }
        if( bind_py_utils_functions->get_util_func_decls().size() > 0 ) {
            array_types_decls += "\n" + bind_py_utils_functions->get_util_func_decls() + "\n";
        }

        if( c_ds_api->get_generated_code().size() > 0 ) {
            helper_defs += "\n" + c_ds_api->get_generated_code() + "\n";
        }
        if( c_utils_functions->get_generated_code().size() > 0 ) {
            helper_defs += "\n" + c_utils_functions->get_generated_code() + "\n";
        }
        if( bind_py_utils_functions->get_generated_code().size() > 0 ) {
            helper_defs += "\n" + bind_py_utils_functions->get_generated_code() + "\n";
        }

        if (array_types_decls.size() != 0) {
            array_types_decls = "\nstruct dimension_descriptor\n"
                "{\n    int32_t lower_bound, length, stride;\n};\n" + array_types_decls;
        }
    }

    std::string make_unit_filename(const std::string &kind,
            const std::string &name) const {
        return kind + "_" + CUtils::sanitize_c_identifier(name) + ".c";
    }

    std::string make_scoped_unit_filename(const std::string &kind,
            const std::string &scope_name, const std::string &name) const {
        std::string file_stem = kind + "_" + CUtils::sanitize_c_identifier(scope_name);
        if (!name.empty()) {
            file_stem += "__" + CUtils::sanitize_c_identifier(name);
        }
        return file_stem + ".c";
    }

    std::string make_function_unit_filename(const std::string &kind,
            const std::string &scope_name, const ASR::Function_t &fn) const {
        uint64_t fn_hash = get_hash(
            reinterpret_cast<ASR::asr_t*>(const_cast<ASR::Function_t*>(&fn)));
        return make_scoped_unit_filename(
            kind, scope_name,
            std::string(fn.m_name) + "__" + std::to_string(fn_hash));
    }

    std::string get_unit_file_prelude(const std::string &header_name) const {
        return "#include \"" + header_name + "\"\n\n";
    }

    std::string get_global_extern_decl(const ASR::Variable_t &v) {
        std::string emitted_name = CUtils::get_c_variable_name(v);
        std::string full_def = convert_variable_decl(v);
        std::string primary_decl;
        for (const auto &line : split_lines_keep_newlines(full_def)) {
            std::string trimmed = trim_copy(line);
            if (trimmed.empty()) {
                continue;
            }
            if (!line_contains_word(trimmed, emitted_name)) {
                continue;
            }
            primary_decl = strip_initializer_from_decl(trimmed);
        }
        if (primary_decl.empty()) {
            throw CodeGenError("Split C emission failed to derive global declaration for `"
                + std::string(v.m_name) + "`");
        }
        if (primary_decl.rfind("static ", 0) == 0) {
            primary_decl = primary_decl.substr(7);
        }
        return "extern " + primary_decl + ";\n";
    }

    std::string get_split_visible_definition(const ASR::Variable_t &v) {
        std::string module_array_def = get_static_module_array_definition(v);
        if (!module_array_def.empty()) {
            return module_array_def;
        }
        std::string emitted_name = CUtils::get_c_variable_name(v);
        std::string full_def = convert_variable_decl(v);
        std::string normalized_def;
        for (const auto &line : split_lines_keep_newlines(full_def)) {
            std::string updated = line;
            std::string trimmed = trim_copy(updated);
            if (!trimmed.empty()
                    && line_contains_word(trimmed, emitted_name)
                    && trimmed.rfind("static ", 0) == 0) {
                size_t static_pos = updated.find("static ");
                if (static_pos != std::string::npos) {
                    updated.erase(static_pos, 7);
                }
            }
            normalized_def += updated;
        }
        return normalized_def;
    }

    std::string get_static_dim_expr(ASR::expr_t *expr,
            const std::string &fallback) {
        if (expr == nullptr) {
            return fallback;
        }
        ASR::expr_t *value = ASRUtils::expr_value(expr);
        int64_t int_value = 0;
        if (value != nullptr && ASRUtils::extract_value(value, int_value)) {
            return std::to_string(int_value);
        }
        visit_expr(*expr);
        return src;
    }

    bool get_static_module_array_element_type(const ASR::Variable_t &v,
            ASR::ttype_t *element_type, std::string &type_name,
            std::string &encoded_type_name) {
        if (ASRUtils::is_integer(*element_type)) {
            ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(element_type);
            headers.insert("inttypes.h");
            type_name = "int" + std::to_string(t->m_kind * 8) + "_t";
            encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
            return true;
        }
        if (ASRUtils::is_unsigned_integer(*element_type)) {
            ASR::UnsignedInteger_t *t = ASR::down_cast<ASR::UnsignedInteger_t>(element_type);
            headers.insert("inttypes.h");
            type_name = "uint" + std::to_string(t->m_kind * 8) + "_t";
            encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
            return true;
        }
        if (ASRUtils::is_real(*element_type)) {
            ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(element_type);
            if (t->m_kind == 4) {
                type_name = "float";
            } else if (t->m_kind == 8) {
                type_name = "double";
            } else {
                return false;
            }
            encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
            return true;
        }
        if (ASRUtils::is_complex(*element_type)) {
            ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(element_type);
            headers.insert("complex.h");
            if (t->m_kind == 4) {
                type_name = "float complex";
            } else if (t->m_kind == 8) {
                type_name = "double complex";
            } else {
                return false;
            }
            encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
            return true;
        }
        if (ASRUtils::is_logical(*element_type)) {
            headers.insert("stdbool.h");
            type_name = "bool";
            encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
            return true;
        }
        if (ASRUtils::is_character(*element_type)) {
            type_name = "char *";
            encoded_type_name = CUtils::get_c_type_code(v.m_type);
            return true;
        }
        if (ASR::is_a<ASR::StructType_t>(*element_type) && v.m_type_declaration) {
            type_name = "struct " + CUtils::get_c_symbol_name(
                ASRUtils::symbol_get_past_external(v.m_type_declaration));
            encoded_type_name = CUtils::sanitize_c_identifier(
                "x" + CUtils::get_c_symbol_name(v.m_type_declaration));
            return true;
        }
        return false;
    }

    std::string get_static_module_array_definition(const ASR::Variable_t &v) {
        if (!ASRUtils::is_array(v.m_type)
                || ASRUtils::is_pointer(v.m_type)
                || ASRUtils::is_allocatable(v.m_type)
                || v.m_parent_symtab == nullptr
                || v.m_parent_symtab->asr_owner == nullptr
                || !ASR::is_a<ASR::Module_t>(
                    *ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner))) {
            return "";
        }
        ASR::dimension_t *m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(v.m_type, m_dims);
        int64_t total_size = ASRUtils::get_fixed_size_of_array(m_dims, n_dims);
        if (n_dims == 0 || total_size < 0) {
            return "";
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable_pointer(v.m_type));
        if (element_type == nullptr) {
            return "";
        }
        std::string type_name, encoded_type_name;
        if (!get_static_module_array_element_type(v, element_type, type_name,
                encoded_type_name)) {
            return "";
        }

        std::string emitted_name = CUtils::get_c_variable_name(v);
        std::string wrapper_type = c_ds_api->get_array_type(
            type_name, encoded_type_name, array_types_decls, false);
        std::string pointer_type = c_ds_api->get_array_type(
            type_name, encoded_type_name, array_types_decls, true);
        std::string data_name = emitted_name + "_data";
        std::string value_name = emitted_name + "_value";

        std::string sub;
        sub += "static " + format_type_c("[" + std::to_string(total_size) + "]",
            type_name, data_name, false, false);
        ASR::expr_t *init_expr = get_variable_init_value_expr(v);
        std::string init_brace = init_expr != nullptr
            ? emit_c_array_constant_brace_init(init_expr, v.m_type) : "";
        if (!init_brace.empty()) {
            sub += " = " + init_brace;
        } else if (type_name == "char *" || type_name == "char*") {
            sub += " = {0}";
        }
        sub += ";\n";

        std::vector<std::string> lower_bounds(n_dims);
        std::vector<std::string> lengths(n_dims);
        std::vector<std::string> strides(n_dims);
        std::string stride = "1";
        for (size_t i = 0; i < n_dims; i++) {
            lower_bounds[i] = get_static_dim_expr(m_dims[i].m_start, "1");
            lengths[i] = get_static_dim_expr(m_dims[i].m_length, "0");
            strides[i] = stride;
            stride = "(" + stride + "*" + lengths[i] + ")";
        }

        sub += "static " + wrapper_type + " " + value_name + " = { ";
        sub += ".data = " + data_name + ", .dims = {";
        for (size_t i = 0; i < n_dims; i++) {
            sub += "{" + lower_bounds[i] + ", " + lengths[i] + ", "
                + strides[i] + "}";
            if (i + 1 < n_dims) {
                sub += ", ";
            }
        }
        sub += "}, .n_dims = " + std::to_string(n_dims)
            + ", .offset = 0, .is_allocated = true };\n";
        sub += format_type_c("", pointer_type, emitted_name, false, false)
            + " = &" + value_name;
        return sub;
    }

    std::string get_global_definition(const ASR::Variable_t &v) {
        return get_split_visible_definition(v) + ";\n";
    }

    void hoist_split_header_enum_name_defs(std::string &type_decls,
            std::string &shared_defs) {
        std::string kept_decls;
        for (const auto &line : split_lines_keep_newlines(type_decls)) {
            std::string trimmed = trim_copy(line);
            if (trimmed.empty()) {
                kept_decls += line;
                continue;
            }
            if (trimmed.rfind("char enum_names_", 0) == 0
                    && trimmed.find('=') != std::string::npos) {
                kept_decls += "extern " + strip_initializer_from_decl(trimmed) + ";\n";
                shared_defs += line;
                if (shared_defs.empty() || shared_defs.back() != '\n') {
                    shared_defs += "\n";
                }
                continue;
            }
            kept_decls += line;
        }
        type_decls = kept_decls;
    }

    std::string collect_module_extern_decls(const ASR::TranslationUnit_t &x) {
        std::string decls;
        std::vector<std::string> build_order =
            ASRUtils::determine_module_dependencies(x);
        for (const auto &item : build_order) {
            ASR::symbol_t *mod_sym = x.m_symtab->get_symbol(item);
            if (mod_sym == nullptr || !ASR::is_a<ASR::Module_t>(*mod_sym)) {
                continue;
            }
            ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(mod_sym);
            if (to_lower(mod->m_name) == "omp_lib"
                    || to_lower(mod->m_name) == "iso_c_binding"
                    || to_lower(mod->m_name) == "lfortran_intrinsic_iso_c_binding") {
                continue;
            }
            for (auto &scope_item : mod->m_symtab->get_scope()) {
                if (ASR::is_a<ASR::Variable_t>(*scope_item.second)) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(scope_item.second);
                    try {
                        decls += get_global_extern_decl(*v);
                    } catch (const CodeGenError &e) {
                        throw CodeGenError(
                            "Split C emission failed for module variable extern declaration `"
                            + std::string(mod->m_name) + "::" + std::string(v->m_name)
                            + "`: " + e.d.message
                        );
                    }
                }
            }
        }
        return decls;
    }

    std::string collect_split_function_decls(const ASR::TranslationUnit_t &x) {
        std::string decls;
        std::set<std::string> seen;
        auto add_function_decl = [&](ASR::Function_t *fn) {
            if (is_split_global_helper_function(fn)) {
                return;
            }
            ASR::FunctionType_t *f_type = ASRUtils::get_FunctionType(*fn);
            if (f_type->m_abi == ASR::abiType::Intrinsic) {
                return;
            }
            bool has_typevar = false;
            try {
                std::string decl = get_function_declaration(*fn, has_typevar, false, false) + ";\n";
                if (!has_typevar && decl != ";\n" && seen.insert(decl).second) {
                    decls += decl;
                }
            } catch (const CodeGenError &e) {
                throw CodeGenError(
                    "Split C emission failed for function declaration `"
                    + std::string(fn->m_name) + "`: " + e.d.message
                );
            }
        };

        std::vector<ASR::Function_t*> global_functions =
            get_complete_function_definitions(x.m_symtab);
        for (ASR::Function_t *fn : global_functions) {
            add_function_decl(fn);
        }

        std::vector<std::string> build_order =
            ASRUtils::determine_module_dependencies(x);
        for (const auto &item : build_order) {
            ASR::symbol_t *mod_sym = x.m_symtab->get_symbol(item);
            if (mod_sym == nullptr || !ASR::is_a<ASR::Module_t>(*mod_sym)) {
                continue;
            }
            ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(mod_sym);
            std::vector<ASR::Function_t*> functions =
                get_complete_function_definitions(mod->m_symtab);
            for (ASR::Function_t *fn : functions) {
                add_function_decl(fn);
            }
        }

        for (const auto &item : x.m_symtab->get_scope()) {
            if (!ASR::is_a<ASR::Program_t>(*item.second)) {
                continue;
            }
            ASR::Program_t *program = ASR::down_cast<ASR::Program_t>(item.second);
            std::vector<ASR::Function_t*> functions =
                get_complete_function_definitions(program->m_symtab);
            for (ASR::Function_t *fn : functions) {
                add_function_decl(fn);
            }
        }
        return decls;
    }

    void write_text_file(const std::filesystem::path &path,
            const std::string &contents) {
        std::ofstream out_file(path);
        out_file << contents;
        out_file.close();
    }

    std::string emit_module_variable_defs(const ASR::Module_t &x) {
        std::string unit_src;
        for (auto &item : x.m_symtab->get_scope()) {
            if (!ASR::is_a<ASR::Variable_t>(*item.second)) {
                continue;
            }
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
            std::string decl = get_split_visible_definition(*v);
            unit_src += decl;
            if (!decl.empty()) {
                unit_src += ";\n";
            }
        }
        return unit_src;
    }

    std::vector<ASR::Function_t*> get_complete_function_definitions(
            SymbolTable *symtab) {
        std::vector<std::string> func_order =
            ASRUtils::determine_function_definition_order(symtab);
        std::vector<ASR::Function_t*> complete_functions;
        std::set<uint64_t> seen;

        std::function<void(ASR::symbol_t*)> add_symbol;
        std::function<void(ASR::Function_t*)> add_function;

        auto owned_by_scope = [&](const ASR::symbol_t *sym) -> bool {
            if (sym == nullptr) {
                return false;
            }
            SymbolTable *owner = ASRUtils::symbol_parent_symtab(
                const_cast<ASR::symbol_t*>(sym));
            while (owner != nullptr) {
                if (owner == symtab) {
                    return true;
                }
                owner = owner->parent;
            }
            return false;
        };

        auto resolve_dependency = [&](ASR::Function_t *fn, const char *dep_name) -> ASR::symbol_t* {
            if (fn == nullptr || fn->m_symtab == nullptr || dep_name == nullptr) {
                return nullptr;
            }
            return fn->m_symtab->resolve_symbol(std::string(dep_name));
        };

        add_function = [&](ASR::Function_t *fn) {
            if (fn == nullptr) {
                return;
            }
            uint64_t key = get_hash(reinterpret_cast<ASR::asr_t*>(fn));
            if (!seen.insert(key).second) {
                return;
            }

            for (size_t i = 0; i < fn->n_dependencies; i++) {
                add_symbol(resolve_dependency(fn, fn->m_dependencies[i]));
            }

            complete_functions.push_back(fn);
        };

        add_symbol = [&](ASR::symbol_t *sym) {
            sym = ASRUtils::symbol_get_past_external(sym);
            if (sym == nullptr) {
                return;
            }

            if (ASR::is_a<ASR::Function_t>(*sym)) {
                if (!owned_by_scope(sym)) {
                    return;
                }
                add_function(ASR::down_cast<ASR::Function_t>(sym));
                return;
            }

            auto add_procs = [&](ASR::symbol_t **procs, size_t n_procs) {
                for (size_t i = 0; i < n_procs; i++) {
                    add_symbol(procs[i]);
                }
            };

            if (ASR::is_a<ASR::GenericProcedure_t>(*sym)) {
                ASR::GenericProcedure_t *gp = ASR::down_cast<ASR::GenericProcedure_t>(sym);
                add_procs(gp->m_procs, gp->n_procs);
            } else if (ASR::is_a<ASR::CustomOperator_t>(*sym)) {
                ASR::CustomOperator_t *op = ASR::down_cast<ASR::CustomOperator_t>(sym);
                add_procs(op->m_procs, op->n_procs);
            } else if (ASR::is_a<ASR::StructMethodDeclaration_t>(*sym)) {
                ASR::StructMethodDeclaration_t *md =
                    ASR::down_cast<ASR::StructMethodDeclaration_t>(sym);
                add_symbol(md->m_proc);
            }
        };

        // Preserve the dependency order where we have it first.
        for (const auto &name : func_order) {
            add_symbol(symtab->get_symbol(name));
        }

        // Then scan the local scope directly so private/local procedures that
        // never enter the dependency order still get emitted in split mode.
        for (const auto &item : symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                add_function(ASR::down_cast<ASR::Function_t>(item.second));
                continue;
            }
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(item.second);
            if (sym == nullptr) {
                continue;
            }
            add_symbol(sym);
        }
        return complete_functions;
    }

    bool is_split_global_helper_function(const ASR::Function_t *fn) const {
        return fn != nullptr && startswith(fn->m_name, "_lcompilers_");
    }

    static bool scope_contains(SymbolTable *scope, SymbolTable *candidate) {
        while (candidate != nullptr) {
            if (candidate == scope) {
                return true;
            }
            candidate = candidate->parent;
        }
        return false;
    }

    std::vector<ASR::Function_t*> consume_pending_function_definitions(
            SymbolTable *scope, std::set<uint64_t> &emitted,
            bool helpers_only=false) {
        std::vector<ASR::Function_t*> ready;
        std::vector<ASR::Function_t*> remaining;
        std::set<uint64_t> remaining_hashes;
        for (ASR::Function_t *fn : pending_function_definitions) {
            if (fn == nullptr) {
                continue;
            }
            uint64_t key = get_hash(reinterpret_cast<ASR::asr_t*>(fn));
            if (emitted.find(key) != emitted.end()) {
                continue;
            }
            SymbolTable *parent_scope = fn->m_symtab ? fn->m_symtab->parent : nullptr;
            bool ready_for_emission = false;
            if (helpers_only) {
                ready_for_emission = is_split_global_helper_function(fn);
            } else {
                ready_for_emission = parent_scope != nullptr
                    && scope_contains(scope, parent_scope);
            }
            if (ready_for_emission) {
                emitted.insert(key);
                ready.push_back(fn);
                continue;
            }
            if (remaining_hashes.insert(key).second) {
                remaining.push_back(fn);
            }
        }
        pending_function_definitions = std::move(remaining);
        pending_function_definition_hashes = std::move(remaining_hashes);
        return ready;
    }

    std::vector<std::pair<std::string, std::string>> emit_module_split_units(
            const ASR::Module_t &x) {
        std::vector<std::pair<std::string, std::string>> units;
        std::set<uint64_t> emitted_functions;
        bool intrinsic_module_copy = intrinsic_module;
        intrinsic_module = x.m_intrinsic;

        std::string module_name = x.m_name;
        std::string module_name_lower = to_lower(module_name);
        if (module_name_lower == "omp_lib") {
            headers.insert("omp.h");
            intrinsic_module = intrinsic_module_copy;
            return units;
        } else if (module_name_lower == "iso_c_binding"
                || module_name_lower == "lfortran_intrinsic_iso_c_binding") {
            intrinsic_module = intrinsic_module_copy;
            return units;
        }

        std::string variable_defs = emit_module_variable_defs(x);
        if (!variable_defs.empty()) {
            units.push_back({
                make_scoped_unit_filename("module", module_name, "data"),
                variable_defs
            });
        }

        std::vector<ASR::Function_t*> functions =
            get_complete_function_definitions(x.m_symtab);
        auto emit_function_unit = [&](ASR::Function_t *fn) {
            if (fn == nullptr) {
                return;
            }
            uint64_t key = get_hash(reinterpret_cast<ASR::asr_t*>(fn));
            if (!emitted_functions.insert(key).second) {
                return;
            }
            bool has_typevar = false;
            std::string expected_decl =
                get_function_declaration(*fn, has_typevar, false, false);
            visit_Function(*fn);
            if (!src.empty()) {
                if (!expected_decl.empty()
                        && src.find(expected_decl) == std::string::npos) {
                    throw CodeGenError(
                        "Split C emission produced an invalid function body for `"
                        + std::string(module_name) + "::" + std::string(fn->m_name)
                        + "`; expected declaration `" + expected_decl
                        + "` was not present in emitted source."
                    );
                }
                std::string unit_filename =
                    make_function_unit_filename("module", module_name, *fn);
                auto fn_units = maybe_split_large_subroutine_unit(
                    unit_filename, *fn, src);
                for (auto &unit : fn_units) {
                    if (!unit.second.empty()) {
                        units.push_back(std::move(unit));
                    }
                }
            }
        };
        for (ASR::Function_t *fn : functions) {
            emit_function_unit(fn);
        }

        intrinsic_module = intrinsic_module_copy;
        return units;
    }

    std::vector<std::pair<std::string, std::string>> emit_program_split_units(
            const ASR::Program_t &x) {
        std::vector<std::pair<std::string, std::string>> units;

        if (compiler_options.enable_cpython
                || compiler_options.link_numpy
                || compiler_options.target_offload_enabled) {
            visit_Program(x);
            if (!src.empty()) {
                units.push_back({make_unit_filename("program", x.m_name), src});
            }
            return units;
        }

        std::vector<ASR::Function_t*> functions =
            get_complete_function_definitions(x.m_symtab);
        SymbolTable *current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        std::map<std::string, std::vector<std::string>> struct_dep_graph;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Struct_t>(*item.second)
                    || ASR::is_a<ASR::Enum_t>(*item.second)
                    || ASR::is_a<ASR::Union_t>(*item.second)) {
                std::vector<std::string> struct_deps_vec;
                std::pair<char**, size_t> struct_deps_ptr =
                    ASRUtils::symbol_dependencies(item.second);
                for (size_t i = 0; i < struct_deps_ptr.second; i++) {
                    struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                }
                struct_dep_graph[item.first] = struct_deps_vec;
            }
        }
        std::vector<std::string> struct_deps = ASRUtils::order_deps(struct_dep_graph);
        for (const auto &item : struct_deps) {
            ASR::symbol_t *struct_sym = x.m_symtab->get_symbol(item);
            if (struct_sym == nullptr) {
                continue;
            }
            if (!(ASR::is_a<ASR::Struct_t>(*struct_sym)
                    || ASR::is_a<ASR::Enum_t>(*struct_sym)
                    || ASR::is_a<ASR::Union_t>(*struct_sym))) {
                continue;
            }
            visit_symbol(*struct_sym);
        }
        auto emit_function_unit = [&](ASR::Function_t *fn) {
            visit_Function(*fn);
            if (!src.empty()) {
                std::string unit_filename =
                    make_function_unit_filename("program", x.m_name, *fn);
                auto fn_units = maybe_split_large_subroutine_unit(
                    unit_filename, *fn, src);
                for (auto &unit : fn_units) {
                    if (!unit.second.empty()) {
                        units.push_back(std::move(unit));
                    }
                }
            }
        };
        for (ASR::Function_t *fn : functions) {
            emit_function_unit(fn);
        }

        current_scope = x.m_symtab;
        current_function = nullptr;
        indentation_level = 1;
        std::string indent1(indentation_level * indentation_spaces, ' ');
        std::string decl;
        std::vector<std::string> var_order =
            ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (const auto &item : var_order) {
            ASR::symbol_t *var_sym = x.m_symtab->get_symbol(item);
            if (var_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*var_sym)) {
                continue;
            }
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
            std::string decl_tmp = convert_variable_decl(*v);
            if (!decl_tmp.empty()) {
                decl += indent1 + decl_tmp + ";\n";
            }
        }

        std::vector<std::string> body_stmts;
        for (size_t i = 0; i < x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            if (!src.empty()) {
                body_stmts.push_back(src);
            }
        }
        indentation_level = 0;

        auto main_units = maybe_split_large_program_unit(
            make_unit_filename("program", x.m_name), x, decl, body_stmts);
        for (auto &unit : main_units) {
            if (!unit.second.empty()) {
                units.push_back(std::move(unit));
            }
        }
        current_scope = current_scope_copy;
        return units;
    }

    size_t count_lines(const std::string &text) const {
        return static_cast<size_t>(std::count(text.begin(), text.end(), '\n')) + 1;
    }

    std::vector<std::string> split_lines_keep_newlines(const std::string &text) const {
        std::vector<std::string> lines;
        size_t start = 0;
        while (start < text.size()) {
            size_t end = text.find('\n', start);
            if (end == std::string::npos) {
                lines.push_back(text.substr(start));
                break;
            }
            lines.push_back(text.substr(start, end - start + 1));
            start = end + 1;
        }
        if (text.empty()) {
            lines.push_back("");
        }
        return lines;
    }

    std::string trim_copy(const std::string &line) const {
        size_t start = 0;
        while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
            start++;
        }
        size_t end = line.size();
        while (end > start && std::isspace(static_cast<unsigned char>(line[end - 1]))) {
            end--;
        }
        return line.substr(start, end - start);
    }

    bool line_contains_word(const std::string &line, const std::string &word) const {
        size_t pos = line.find(word);
        while (pos != std::string::npos) {
            bool left_ok = pos == 0
                || !(std::isalnum(static_cast<unsigned char>(line[pos - 1])) || line[pos - 1] == '_');
            size_t end_pos = pos + word.size();
            bool right_ok = end_pos >= line.size()
                || !(std::isalnum(static_cast<unsigned char>(line[end_pos])) || line[end_pos] == '_');
            if (left_ok && right_ok) {
                return true;
            }
            pos = line.find(word, pos + 1);
        }
        return false;
    }

    std::string strip_initializer_from_decl(std::string decl) const {
        size_t eq_pos = decl.find(" = ");
        if (eq_pos != std::string::npos) {
            decl = decl.substr(0, eq_pos);
        }
        while (!decl.empty() && std::isspace(static_cast<unsigned char>(decl.back()))) {
            decl.pop_back();
        }
        if (!decl.empty() && decl.back() == ';') {
            decl.pop_back();
        }
        while (!decl.empty() && std::isspace(static_cast<unsigned char>(decl.back()))) {
            decl.pop_back();
        }
        return decl;
    }

    std::vector<ASR::Variable_t*> collect_function_local_vars(const ASR::Function_t &x) {
        std::vector<ASR::Variable_t*> locals;
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (const auto &item : var_order) {
            ASR::symbol_t *var_sym = x.m_symtab->get_symbol(item);
            if (var_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*var_sym)) {
                continue;
            }
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
            if (v->m_intent == ASRUtils::intent_local
                    || v->m_intent == ASRUtils::intent_return_var) {
                locals.push_back(v);
            }
        }
        return locals;
    }

    std::vector<ASR::Variable_t*> collect_program_local_vars(const ASR::Program_t &x) {
        std::vector<ASR::Variable_t*> locals;
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        for (const auto &item : var_order) {
            ASR::symbol_t *var_sym = x.m_symtab->get_symbol(item);
            if (var_sym == nullptr || !ASR::is_a<ASR::Variable_t>(*var_sym)) {
                continue;
            }
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
            if (v->m_intent == ASRUtils::intent_local
                    || v->m_intent == ASRUtils::intent_return_var) {
                locals.push_back(v);
            }
        }
        return locals;
    }

    bool contains_early_return_or_labels(const std::string &body_text) const {
        for (const auto &line : split_lines_keep_newlines(body_text)) {
            std::string trimmed = trim_copy(line);
            if (trimmed.empty()) {
                continue;
            }
            if (trimmed == "return;" || trimmed.rfind("return ", 0) == 0) {
                return true;
            }
            if (!trimmed.empty() && trimmed.back() == ':') {
                return true;
            }
            if (trimmed.find("goto ") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool looks_like_c_decl_stmt(const std::string &stmt_text) const {
        std::string trimmed = trim_copy(stmt_text);
        if (trimmed.empty() || trimmed.back() != ';') {
            return false;
        }
        if (trimmed.find('{') != std::string::npos || trimmed.find('}') != std::string::npos) {
            return false;
        }
        static const std::vector<std::string> decl_prefixes = {
            "struct ", "union ", "enum ", "int ", "int32_t ", "int64_t ",
            "float ", "double ", "bool ", "char ", "const ", "static ",
            "unsigned ", "signed ", "complex ", "_Complex "
        };
        for (const auto &prefix : decl_prefixes) {
            if (trimmed.rfind(prefix, 0) == 0) {
                return true;
            }
        }
        return false;
    }

    std::string make_pointer_field_decl(const ASR::Variable_t &v) {
        std::string full_def = convert_variable_decl(v);
        std::string emitted_name = CUtils::get_c_variable_name(v);
        std::string decl;
        for (const auto &line : split_lines_keep_newlines(full_def)) {
            std::string trimmed = trim_copy(line);
            if (trimmed.empty()) {
                continue;
            }
            if (line_contains_word(trimmed, emitted_name)) {
                decl = strip_initializer_from_decl(trimmed);
            }
        }
        if (decl.empty()) {
            throw CodeGenError("Failed to build split helper context for local variable `"
                + std::string(v.m_name) + "`");
        }
        size_t pos = decl.rfind(emitted_name);
        if (pos == std::string::npos) {
            throw CodeGenError("Failed to build split helper context for local variable `"
                + std::string(v.m_name) + "`");
        }
        decl.replace(pos, emitted_name.size(), "(*" + emitted_name + ")");
        return decl;
    }

    std::string build_large_function_ctx_struct(const std::string &ctx_name,
            const std::vector<ASR::Variable_t*> &locals) {
        if (locals.empty()) {
            return "";
        }
        std::string out = "struct " + ctx_name + " {\n";
        for (ASR::Variable_t *v : locals) {
            out += "    " + make_pointer_field_decl(*v) + ";\n";
        }
        out += "};\n\n";
        return out;
    }

    std::string build_large_function_ctx_init(const std::string &ctx_name,
            const std::vector<ASR::Variable_t*> &locals,
            const std::string &indent) {
        if (locals.empty()) {
            return "";
        }
        std::string out = indent + "struct " + ctx_name + " __ctx = {\n";
        for (ASR::Variable_t *v : locals) {
            std::string emitted_name = CUtils::get_c_variable_name(*v);
            out += indent + "    ." + emitted_name + " = &" + emitted_name + ",\n";
        }
        out += indent + "};\n";
        return out;
    }

    std::string indent_c_block(const std::string &text, const std::string &indent) const {
        std::string out;
        for (const auto &line : split_lines_keep_newlines(text)) {
            if (trim_copy(line).empty()) {
                out += line;
            } else {
                out += indent + line;
            }
        }
        return out;
    }

    std::set<std::string> build_local_decl_statement_set(
            const std::vector<ASR::Variable_t*> &locals,
            std::string &local_decl_block,
            const std::string &indent) {
        std::set<std::string> local_decl_stmts;
        for (ASR::Variable_t *v : locals) {
            std::string decl_text = convert_variable_decl(*v) + ";\n";
            local_decl_block += indent_c_block(decl_text, indent);
            for (const auto &stmt : split_top_level_statements(decl_text)) {
                std::string trimmed = trim_copy(stmt);
                if (!trimmed.empty()) {
                    local_decl_stmts.insert(trimmed);
                }
            }
        }
        return local_decl_stmts;
    }

    std::vector<std::string> collect_function_param_decls(const ASR::Function_t &x) {
        std::vector<std::string> param_decls;
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::expr_t *arg = x.m_args[i];
            if (arg == nullptr || !ASR::is_a<ASR::Var_t>(*arg)) {
                continue;
            }
            ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(arg)->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                continue;
            }
            param_decls.push_back(convert_variable_decl(*ASR::down_cast<ASR::Variable_t>(arg_sym)));
        }
        return param_decls;
    }

    std::vector<std::string> collect_function_param_names(const ASR::Function_t &x) {
        std::vector<std::string> param_names;
        for (size_t i = 0; i < x.n_args; i++) {
            ASR::expr_t *arg = x.m_args[i];
            if (arg == nullptr || !ASR::is_a<ASR::Var_t>(*arg)) {
                continue;
            }
            ASR::symbol_t *arg_sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(arg)->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*arg_sym)) {
                continue;
            }
            param_names.push_back(CUtils::get_c_variable_name(
                *ASR::down_cast<ASR::Variable_t>(arg_sym)));
        }
        return param_names;
    }

    std::vector<std::string> split_top_level_statements(const std::string &body_text) const {
        std::vector<std::string> stmts;
        std::string current_stmt;
        int brace_depth = 0;
        for (const auto &line : split_lines_keep_newlines(body_text)) {
            current_stmt += line;
            for (char c : line) {
                if (c == '{') {
                    brace_depth++;
                } else if (c == '}') {
                    brace_depth--;
                }
            }
            std::string trimmed = trim_copy(line);
            if (brace_depth == 0 && !trimmed.empty()
                    && (trimmed.back() == ';' || trimmed == "}")) {
                stmts.push_back(current_stmt);
                current_stmt.clear();
            }
        }
        if (!current_stmt.empty()) {
            stmts.push_back(current_stmt);
        }
        return stmts;
    }

    std::vector<std::string> group_statements_into_chunks(
            const std::vector<std::string> &stmts, size_t max_chunk_lines) const {
        std::vector<std::string> chunks;
        std::string current_chunk;
        size_t current_lines = 0;
        for (const auto &stmt : stmts) {
            size_t stmt_lines = count_lines(stmt);
            if (!current_chunk.empty() && current_lines + stmt_lines > max_chunk_lines) {
                chunks.push_back(current_chunk);
                current_chunk.clear();
                current_lines = 0;
            }
            current_chunk += stmt;
            current_lines += stmt_lines;
        }
        if (!current_chunk.empty()) {
            chunks.push_back(current_chunk);
        }
        return chunks;
    }

    bool is_safe_for_split_pack(const std::pair<std::string, std::string> &unit,
            size_t max_single_unit_lines) const {
        size_t unit_lines = count_lines(unit.second);
        if (unit_lines == 0 || unit_lines > max_single_unit_lines) {
            return false;
        }
        for (const auto &line : split_lines_keep_newlines(unit.second)) {
            std::string trimmed = trim_copy(line);
            if (trimmed.empty()) {
                continue;
            }
            if (startswith(trimmed, "#") || startswith(line, "static ")) {
                return false;
            }
        }
        return true;
    }

    std::string get_split_pack_group(const std::string &filename) const {
        std::string stem = filename;
        if (endswith(stem, ".c")) {
            stem = stem.substr(0, stem.size() - 2);
        }
        size_t sep = stem.find("__");
        if (sep != std::string::npos) {
            stem = stem.substr(0, sep);
        }
        if (stem.empty()) {
            stem = "unit";
        }
        return CUtils::sanitize_c_identifier(stem);
    }

    std::string add_inline_hint_to_small_split_unit(const std::string &body,
            size_t max_inline_unit_lines) const {
        if (count_lines(body) > max_inline_unit_lines) {
            return body;
        }
        if (body.find("_lfortran_malloc_alloc") != std::string::npos
                || body.find("_lfortran_free_alloc") != std::string::npos) {
            return body;
        }
        size_t line_start = 0;
        while (line_start < body.size()) {
            size_t line_end = body.find('\n', line_start);
            if (line_end == std::string::npos) {
                line_end = body.size();
            }
            std::string line = body.substr(line_start, line_end - line_start);
            std::string trimmed = trim_copy(line);
            if (trimmed.empty()) {
                line_start = line_end + (line_end < body.size() ? 1 : 0);
                continue;
            }
            if (startswith(trimmed, "int main(")
                    || startswith(trimmed, "__attribute__((always_inline))")
                    || startswith(trimmed, "inline __attribute__((always_inline))")
                    || trimmed.find('(') == std::string::npos
                    || trimmed.find(';') != std::string::npos) {
                return body;
            }
            size_t open_paren = trimmed.find('(');
            std::string before_args = trim_copy(trimmed.substr(0, open_paren));
            size_t name_start = before_args.find_last_of(" *\t");
            std::string function_name = before_args.substr(
                name_start == std::string::npos ? 0 : name_start + 1);
            if (!function_name.empty()
                    && body.find(function_name + "(", line_end) != std::string::npos) {
                return body;
            }
            return body.substr(0, line_start)
                + "inline __attribute__((always_inline)) "
                + body.substr(line_start);
        }
        return body;
    }

    std::vector<std::pair<std::string, std::string>> pack_split_units_by_budget(
            const std::vector<std::pair<std::string, std::string>> &units,
            const std::string &project_name) const {
        static const size_t pack_line_budget = 12000;
        static const size_t max_packable_unit_lines = 4000;
        static const size_t max_inline_hint_unit_lines = 250;

        struct PackState {
            size_t index = 0;
            size_t lines = 0;
            std::string body;
        };

        std::vector<std::pair<std::string, std::string>> packed_units;
        std::map<std::string, PackState> active_packs;
        std::map<std::string, size_t> next_pack_index;

        auto flush_pack = [&](const std::string &group) {
            auto it = active_packs.find(group);
            if (it == active_packs.end() || it->second.body.empty()) {
                return;
            }
            std::string filename = project_name + "_pack_" + group + "_"
                + std::to_string(it->second.index) + ".c";
            packed_units.push_back({filename, std::move(it->second.body)});
            active_packs.erase(it);
        };

        for (const auto &unit : units) {
            size_t unit_lines = count_lines(unit.second);
            if (!is_safe_for_split_pack(unit, max_packable_unit_lines)) {
                packed_units.push_back(unit);
                continue;
            }

            std::string group = get_split_pack_group(unit.first);
            PackState &pack = active_packs[group];
            if (pack.body.empty()) {
                pack.index = next_pack_index[group]++;
                pack.lines = 0;
            } else if (pack.lines + unit_lines > pack_line_budget) {
                flush_pack(group);
                PackState &new_pack = active_packs[group];
                new_pack.index = next_pack_index[group]++;
                new_pack.lines = 0;
            }

            PackState &current_pack = active_packs[group];
            current_pack.body += "\n/* split-C packed from " + unit.first + " */\n";
            current_pack.body += add_inline_hint_to_small_split_unit(
                unit.second, max_inline_hint_unit_lines);
            if (!endswith(current_pack.body, "\n")) {
                current_pack.body += "\n";
            }
            current_pack.lines += unit_lines + 1;
        }

        std::vector<std::string> groups;
        groups.reserve(active_packs.size());
        for (const auto &item : active_packs) {
            groups.push_back(item.first);
        }
        for (const auto &group : groups) {
            flush_pack(group);
        }
        return packed_units;
    }

    std::string dedupe_decl_lines(const std::string &text) const {
        auto extract_decl_function_name = [](const std::string &line) -> std::string {
            if (line.find('=') != std::string::npos) {
                return "";
            }
            size_t lparen = line.find('(');
            if (lparen == std::string::npos || lparen == 0) {
                return "";
            }
            if (lparen >= 2 && line[lparen - 1] == '*' && line[lparen - 2] == '(') {
                return "";
            }
            size_t end = lparen;
            while (end > 0 && std::isspace(static_cast<unsigned char>(line[end - 1]))) {
                end--;
            }
            size_t begin = end;
            while (begin > 0) {
                unsigned char ch = static_cast<unsigned char>(line[begin - 1]);
                if (!(std::isalnum(ch) || ch == '_')) {
                    break;
                }
                begin--;
            }
            if (begin == end) {
                return "";
            }
            return line.substr(begin, end - begin);
        };

        std::vector<std::string> lines;
        std::set<std::string> seen_lines;
        std::map<std::string, std::set<std::string>> decls_by_name;
        for (const auto &line : split_lines_keep_newlines(text)) {
            std::string trimmed = trim_copy(line);
            if (trimmed.empty() || !seen_lines.insert(trimmed).second) {
                continue;
            }
            lines.push_back(trimmed);
            std::string fn_name = extract_decl_function_name(trimmed);
            if (!fn_name.empty()) {
                decls_by_name[fn_name].insert(trimmed);
            }
        }

        std::string out;
        for (const auto &line : lines) {
            std::string fn_name = extract_decl_function_name(line);
            if (!fn_name.empty() && decls_by_name[fn_name].size() > 1) {
                continue;
            }
            out += line + "\n";
        }
        if (!out.empty()) {
            out += "\n";
        }
        return out;
    }

    std::string collect_module_aggregate_decls(const ASR::TranslationUnit_t &x) {
        std::string decls;
        std::vector<std::string> build_order =
            ASRUtils::determine_module_dependencies(x);
        for (const auto &item : build_order) {
            ASR::symbol_t *mod_sym = x.m_symtab->get_symbol(item);
            if (mod_sym == nullptr || !ASR::is_a<ASR::Module_t>(*mod_sym)) {
                continue;
            }
            ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(mod_sym);
            std::map<std::string, std::vector<std::string>> struct_dep_graph;
            for (auto &scope_item : mod->m_symtab->get_scope()) {
                if (ASR::is_a<ASR::Struct_t>(*scope_item.second) ||
                    ASR::is_a<ASR::Enum_t>(*scope_item.second) ||
                    ASR::is_a<ASR::Union_t>(*scope_item.second)) {
                    std::vector<std::string> struct_deps_vec;
                    std::pair<char**, size_t> struct_deps_ptr =
                        ASRUtils::symbol_dependencies(scope_item.second);
                    for (size_t i = 0; i < struct_deps_ptr.second; i++) {
                        struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                    }
                    struct_dep_graph[scope_item.first] = struct_deps_vec;
                }
            }
            for (const auto &dep_name : ASRUtils::order_deps(struct_dep_graph)) {
                ASR::symbol_t *struct_sym = mod->m_symtab->get_symbol(dep_name);
                if (struct_sym == nullptr) {
                    continue;
                }
                visit_symbol(*struct_sym);
                decls += src;
            }
        }
        return decls;
    }

    std::vector<std::pair<std::string, std::string>> maybe_split_large_subroutine_unit(
            const std::string &unit_filename, const ASR::Function_t &fn,
            const std::string &function_src) {
        // Keep per-procedure split emission enabled, but avoid the secondary
        // intra-procedure chunking path for now. Giant procedures such as the
        // Lebedev grid generators still compile fine as standalone `.c` units,
        // while the current chunker can corrupt emitted output for them.
        static const size_t large_function_line_threshold = 20000;
        static const size_t large_function_chunk_lines = 8000;
        std::vector<std::pair<std::string, std::string>> units;
        if (fn.m_return_var != nullptr || count_lines(function_src) <= large_function_line_threshold) {
            units.push_back({unit_filename, function_src});
            return units;
        }

        bool has_typevar = false;
        std::string decl_header = get_function_declaration(fn, has_typevar);
        if (has_typevar) {
            units.push_back({unit_filename, function_src});
            return units;
        }
        size_t decl_pos = function_src.rfind(decl_header);
        if (decl_pos == std::string::npos) {
            units.push_back({unit_filename, function_src});
            return units;
        }
        size_t brace_open = function_src.find("{\n", decl_pos + decl_header.size());
        size_t brace_close = function_src.rfind("}\n");
        if (brace_open == std::string::npos || brace_close == std::string::npos
                || brace_close <= brace_open) {
            units.push_back({unit_filename, function_src});
            return units;
        }

        std::string prefix = function_src.substr(0, decl_pos);
        std::string body_text = function_src.substr(brace_open + 2, brace_close - (brace_open + 2));
        if (contains_early_return_or_labels(body_text)) {
            units.push_back({unit_filename, function_src});
            return units;
        }

        std::vector<ASR::Variable_t*> locals = collect_function_local_vars(fn);
        std::vector<std::string> local_names;
        for (ASR::Variable_t *v : locals) {
            local_names.push_back(CUtils::get_c_variable_name(*v));
        }
        std::string indent = "    ";
        std::string local_decl_block;
        std::set<std::string> local_decl_stmts =
            build_local_decl_statement_set(locals, local_decl_block, indent);

        std::string decl_block;
        std::vector<std::string> stmts = split_top_level_statements(body_text);
        size_t stmt_start = 0;
        for (; stmt_start < stmts.size(); stmt_start++) {
            std::string trimmed_stmt = trim_copy(stmts[stmt_start]);
            if (local_decl_stmts.find(trimmed_stmt) != local_decl_stmts.end()) {
                continue;
            }
            if (trimmed_stmt.empty()) {
                decl_block += stmts[stmt_start];
                continue;
            }
            if (!looks_like_c_decl_stmt(trimmed_stmt)) {
                break;
            }
            decl_block += stmts[stmt_start];
        }
        stmts.erase(stmts.begin(), stmts.begin() + stmt_start);
        std::vector<std::string> filtered_stmts;
        filtered_stmts.reserve(stmts.size());
        for (const auto &stmt : stmts) {
            if (local_decl_stmts.find(trim_copy(stmt)) == local_decl_stmts.end()) {
                filtered_stmts.push_back(stmt);
            }
        }
        stmts = std::move(filtered_stmts);
        if (stmts.size() < 2) {
            units.push_back({unit_filename, function_src});
            return units;
        }
        std::vector<std::string> chunks = group_statements_into_chunks(stmts, large_function_chunk_lines);
        if (chunks.size() < 2) {
            units.push_back({unit_filename, function_src});
            return units;
        }

        std::string unit_stem = unit_filename;
        if (endswith(unit_stem, ".c")) {
            unit_stem = unit_stem.substr(0, unit_stem.size() - 2);
        }
        std::string ctx_name = CUtils::sanitize_c_identifier(unit_stem + "__locals");
        std::string ctx_struct = build_large_function_ctx_struct(ctx_name, locals);
        std::vector<std::string> param_decls = collect_function_param_decls(fn);
        std::vector<std::string> param_names = collect_function_param_names(fn);

        std::vector<std::string> helper_prototypes;
        for (size_t i = 0; i < chunks.size(); i++) {
            std::string helper_name = CUtils::sanitize_c_identifier(
                unit_stem + "__chunk_" + std::to_string(i));
            std::string signature = "void " + helper_name + "(";
            for (size_t j = 0; j < param_decls.size(); j++) {
                if (j > 0) signature += ", ";
                signature += param_decls[j];
            }
            if (!param_decls.empty()) {
                signature += ", ";
            }
            signature += "struct " + ctx_name + "* __ctx)";
            helper_prototypes.push_back(signature + ";");

            std::string helper_src = ctx_struct + signature + "\n{\n";
            for (const auto &local_name : local_names) {
                helper_src += "#define " + local_name + " (*__ctx->" + local_name + ")\n";
            }
            helper_src += chunks[i];
            for (const auto &local_name : local_names) {
                helper_src += "#undef " + local_name + "\n";
            }
            helper_src += "}\n";
            units.push_back({unit_stem + "__chunk_" + std::to_string(i) + ".c", helper_src});
        }

        std::string main_src = prefix + ctx_struct;
        for (const auto &proto : helper_prototypes) {
            main_src += proto + "\n";
        }
        if (!helper_prototypes.empty()) {
            main_src += "\n";
        }
        main_src += decl_header + "\n{\n";
        main_src += decl_block;
        main_src += local_decl_block;
        main_src += build_large_function_ctx_init(ctx_name, locals, indent);
        for (size_t i = 0; i < chunks.size(); i++) {
            std::string helper_name = CUtils::sanitize_c_identifier(
                unit_stem + "__chunk_" + std::to_string(i));
            main_src += indent + helper_name + "(";
            for (size_t j = 0; j < param_names.size(); j++) {
                if (j > 0) main_src += ", ";
                main_src += param_names[j];
            }
            if (!param_names.empty()) {
                main_src += ", ";
            }
            main_src += "&__ctx);\n";
        }
        main_src += "}\n";
        units.push_back({unit_filename, main_src});
        return units;
    }

    std::vector<std::pair<std::string, std::string>> maybe_split_large_program_unit(
            const std::string &unit_filename, const ASR::Program_t &program,
            const std::string &decl_block, const std::vector<std::string> &body_stmts) {
        static const size_t large_program_line_threshold = 8000;
        static const size_t large_program_chunk_lines = 4000;
        std::vector<std::pair<std::string, std::string>> units;

        std::string body_text;
        for (const auto &stmt : body_stmts) {
            body_text += stmt;
        }

        auto emit_single_main_unit = [&]() {
            std::string main_src = "int main(int argc, char* argv[])\n{\n"
                "    _lpython_set_argv(argc, argv);\n"
                + decl_block + body_text + "    return 0;\n}\n";
            units.push_back({unit_filename, main_src});
        };

        if (count_lines(body_text) <= large_program_line_threshold
                || contains_early_return_or_labels(body_text)) {
            emit_single_main_unit();
            return units;
        }

        std::vector<std::string> chunks =
            group_statements_into_chunks(body_stmts, large_program_chunk_lines);
        if (chunks.size() < 2) {
            emit_single_main_unit();
            return units;
        }

        std::vector<ASR::Variable_t*> locals = collect_program_local_vars(program);
        std::string unit_stem = unit_filename;
        if (endswith(unit_stem, ".c")) {
            unit_stem = unit_stem.substr(0, unit_stem.size() - 2);
        }
        std::string ctx_name = CUtils::sanitize_c_identifier(unit_stem + "__locals");
        std::string ctx_struct = build_large_function_ctx_struct(ctx_name, locals);
        std::vector<std::string> helper_prototypes;

        for (size_t i = 0; i < chunks.size(); i++) {
            std::string helper_name = CUtils::sanitize_c_identifier(
                unit_stem + "__chunk_" + std::to_string(i));
            std::string signature = "void " + helper_name + "(struct " + ctx_name + "* __ctx)";
            helper_prototypes.push_back(signature + ";");

            std::string helper_src = ctx_struct + signature + "\n{\n";
            for (ASR::Variable_t *v : locals) {
                std::string local_name = CUtils::get_c_variable_name(*v);
                helper_src += "#define " + local_name + " (*__ctx->" + local_name + ")\n";
            }
            helper_src += chunks[i];
            for (ASR::Variable_t *v : locals) {
                helper_src += "#undef " + CUtils::get_c_variable_name(*v) + "\n";
            }
            helper_src += "}\n";
            units.push_back({unit_stem + "__chunk_" + std::to_string(i) + ".c", helper_src});
        }

        std::string main_src = ctx_struct;
        for (const auto &proto : helper_prototypes) {
            main_src += proto + "\n";
        }
        if (!helper_prototypes.empty()) {
            main_src += "\n";
        }
        main_src += "int main(int argc, char* argv[])\n{\n";
        main_src += "    _lpython_set_argv(argc, argv);\n";
        main_src += decl_block;
        main_src += build_large_function_ctx_init(ctx_name, locals, "    ");
        for (size_t i = 0; i < chunks.size(); i++) {
            std::string helper_name = CUtils::sanitize_c_identifier(
                unit_stem + "__chunk_" + std::to_string(i));
            main_src += "    " + helper_name + "(&__ctx);\n";
        }
        main_src += "    return 0;\n}\n";
        units.push_back({unit_filename, main_src});
        return units;
    }

    template <typename T>
    std::string get_aggregate_c_name(const T &x) {
        const ASR::symbol_t *sym =
            ASR::down_cast<ASR::symbol_t>(x.m_symtab->asr_owner);
        return CUtils::get_c_symbol_name(sym);
    }

    std::string get_enum_c_name(const ASR::Enum_t &x) {
        const ASR::symbol_t *sym =
            ASR::down_cast<ASR::symbol_t>(x.m_symtab->asr_owner);
        auto it = enum_name_map.find(sym);
        if (it != enum_name_map.end()) {
            return it->second;
        }
        std::string name = get_aggregate_c_name(x);
        enum_name_map[sym] = name;
        return name;
    }

    std::string get_enum_member_c_name(const ASR::Variable_t &member_var) {
        return CUtils::get_c_enum_member_name(member_var);
    }

    bool should_emit_struct_definition(const ASR::Struct_t &x) {
        std::string aggregate_name = get_aggregate_c_name(x);
        return emitted_aggregate_names.insert("struct " + aggregate_name).second;
    }

    bool should_emit_union_definition(const ASR::Union_t &x) {
        std::string aggregate_name = get_aggregate_c_name(x);
        return emitted_aggregate_names.insert("union " + aggregate_name).second;
    }

    std::string get_variable_c_name(const ASR::Variable_t &v) {
        return CUtils::get_c_variable_name(v);
    }

    std::string convert_dims_c(size_t n_dims, ASR::dimension_t *m_dims,
                               ASR::ttype_t* element_type, bool& is_fixed_size,
                               bool convert_to_1d=false,
                               std::string *dynamic_stack_size_expr=nullptr)
    {
        std::string dims = "";
        size_t size = 1;
        std::string array_size = "";
        if (dynamic_stack_size_expr != nullptr) {
            dynamic_stack_size_expr->clear();
        }
        for (size_t i=0; i<n_dims; i++) {
            ASR::expr_t *length = m_dims[i].m_length;
            if (!length) {
                is_fixed_size = false;
                return dims;
            } else {
                visit_expr(*length);
                array_size += "*" + src;
                ASR::expr_t* length_value = ASRUtils::expr_value(length);
                if( length_value ) {
                    int64_t length_int = -1;
                    ASRUtils::extract_value(length_value, length_int);
                    size *= length_int;
                    dims += "[" + std::to_string(length_int) + "]";
                } else {
                    size = 0;
                    dims += "[ /* FIXME symbolic dimensions */ ]";
                }
            }
        }
        if( size == 0 ) {
            if (convert_to_1d && dynamic_stack_size_expr != nullptr) {
                *dynamic_stack_size_expr = array_size.empty()
                    ? "1" : array_size.substr(1);
                is_fixed_size = false;
                return dims;
            }
            std::string element_type_str = CUtils::get_c_type_from_ttype_t(element_type);
            dims = "(" + element_type_str + "*)" + " malloc(sizeof(" + element_type_str + ")" + array_size + ")";
            is_fixed_size = false;
            return dims;
        }
        if( convert_to_1d && size != 0 ) {
            dims = "[" + std::to_string(size) + "]";
        }
        return dims;
    }

    void generate_array_decl(std::string& sub, std::string v_m_name,
                             std::string& type_name, std::string& dims,
                             std::string& encoded_type_name,
                             ASR::dimension_t* m_dims, int n_dims,
                             bool use_ref, bool dummy,
                             bool declare_value, bool is_fixed_size,
                             bool is_pointer,
                             ASR::abiType m_abi,
                             bool is_simd_array,
                             const ASR::Variable_t *var = nullptr,
                             const std::string &dynamic_stack_size_expr = "") {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string type_name_copy = type_name;
        std::string original_type_name = type_name;
        bool element_needs_null_init = original_type_name == "char *"
            || original_type_name == "char*";
        type_name = c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls);
        std::string type_name_without_ptr = c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, false);
        if (is_simd_array) {
            int64_t size = ASRUtils::get_fixed_size_of_array(m_dims, n_dims);
            sub = original_type_name + " " + v_m_name + " __attribute__ (( vector_size(sizeof(" + original_type_name + ") * " + std::to_string(size) + ") ))";
            return;
        }
        if (dummy && m_abi != ASR::abiType::BindC && !is_pointer
                && var != nullptr && !ASRUtils::is_allocatable(var->m_type)
                && !ASRUtils::is_pointer(var->m_type)) {
            type_name += " restrict";
        }
        if( declare_value ) {
            std::string variable_name = std::string(v_m_name) + "_value";
            sub = format_type_c("", type_name_without_ptr, variable_name, use_ref, dummy);
            if (is_pointer) {
                bool compiler_created_pointer_array =
                    var != nullptr
                    && std::string(var->m_name).rfind("__libasr_created_", 0) == 0;
                if (compiler_created_pointer_array) {
                    sub += ";\n";
                    sub += indent + variable_name + ".data = NULL;\n";
                    sub += indent + variable_name + ".is_allocated = false";
                } else {
                    sub += " = {0}";
                }
            }
            if (!is_pointer
                    && var != nullptr
                    && is_c_compiler_generated_temporary_name(std::string(var->m_name))
                    && !is_fixed_size
                    && dims.empty()
                    && dynamic_stack_size_expr.empty()
                    && get_variable_init_expr_raw(*var) == nullptr) {
                sub += ";\n";
                sub += indent + format_type_c("", type_name, v_m_name, use_ref, dummy);
                sub += " = &" + variable_name + ";\n";
                sub += indent + std::string(v_m_name) + "->data = NULL;\n";
                sub += indent + std::string(v_m_name) + "->n_dims = "
                    + std::to_string(n_dims) + ";\n";
                sub += indent + std::string(v_m_name) + "->offset = 0;\n";
                sub += indent + std::string(v_m_name) + "->is_allocated = false";
                return;
            }
            sub += ";\n";
            sub += indent + format_type_c("", type_name, v_m_name, use_ref, dummy);
            sub += " = &" + variable_name;
            if( !is_pointer ) {
                std::string init_brace;
                ASR::expr_t *init_expr = nullptr;
                bool needs_runtime_init = false;
                if (var != nullptr
                        && var->m_storage == ASR::storage_typeType::Parameter) {
                    init_expr = get_variable_init_value_expr(*var);
                    if (init_expr != nullptr && is_fixed_size) {
                        init_brace = emit_c_array_constant_brace_init(init_expr, var->m_type);
                        needs_runtime_init = init_brace.empty();
                    }
                }
                bool static_parameter_data = var != nullptr
                    && var->m_storage == ASR::storage_typeType::Parameter
                    && !init_brace.empty()
                    && !element_needs_null_init;
                if (static_parameter_data && is_fixed_size && !is_pointer && !dummy) {
                    std::string data_name = std::string(v_m_name) + "_data";
                    std::string variable_name = std::string(v_m_name) + "_value";
                    sub = "static "
                        + format_type_c(dims, type_name_copy, data_name, use_ref, dummy)
                        + " = " + init_brace + ";\n";
                    sub += indent + "static "
                        + format_type_c("", type_name_without_ptr, variable_name, use_ref, dummy)
                        + " = { .data = " + data_name
                        + ", .n_dims = " + std::to_string(n_dims)
                        + ", .offset = 0, .is_allocated = true, .dims = {";
                    std::string stride = "1";
                    for (int i = 0; i < n_dims; i++) {
                        std::string start = "1", length = "0";
                        if (m_dims[i].m_start) {
                            this->visit_expr(*m_dims[i].m_start);
                            start = src;
                        }
                        if (m_dims[i].m_length) {
                            this->visit_expr(*m_dims[i].m_length);
                            length = src;
                        }
                        if (i > 0) {
                            sub += ", ";
                        }
                        sub += "{" + start + ", " + length + ", " + stride + "}";
                        stride = "(" + stride + "*" + length + ")";
                    }
                    sub += "} };\n";
                    sub += indent + format_type_c("", type_name, v_m_name, use_ref, dummy)
                        + " = &" + variable_name;
                    return;
                }
                sub += ";\n";
                if( !is_fixed_size ) {
                    std::string data_name = std::string(v_m_name) + "_data";
                    if (!dynamic_stack_size_expr.empty()) {
                        std::string size_name = get_unique_local_name(
                            std::string(v_m_name) + "_data_size");
                        std::string stack_data_name = data_name + "_stack";
                        sub += indent + "int64_t " + size_name + " = "
                            + dynamic_stack_size_expr + ";\n";
                        sub += indent + format_type_c(
                            "[(" + size_name + " > 0 && " + size_name + " <= 4096 ? "
                                + size_name + " : 1)]",
                            type_name_copy, stack_data_name, use_ref, dummy) + ";\n";
                        sub += indent + format_type_c("*", type_name_copy, data_name,
                                                    use_ref, dummy)
                            + " = " + stack_data_name + ";\n";
                        sub += indent + "if (" + size_name + " > 4096) {\n";
                        sub += indent + "    " + data_name + " = (" + type_name_copy
                            + "*) malloc(sizeof(" + type_name_copy + ")*" + size_name
                            + ");\n";
                        sub += indent + "}\n";
                        current_function_heap_array_data.push_back("(" + size_name
                            + " > 4096 ? " + data_name + " : NULL)");
                    } else {
                        sub += indent + format_type_c("*", type_name_copy, data_name,
                                                    use_ref, dummy);
                        if( dims.size() > 0 ) {
                            sub += " = " + dims + ";\n";
                            if (var != nullptr && var->m_intent == ASRUtils::intent_local) {
                                current_function_heap_array_data.push_back(data_name);
                            }
                        } else {
                            sub += " = NULL;\n";
                        }
                    }
                } else {
                    sub += indent;
                    if (static_parameter_data) {
                        sub += "static ";
                    }
                    sub += format_type_c(dims, type_name_copy, std::string(v_m_name) + "_data",
                                         use_ref, dummy);
                    if (!init_brace.empty()) {
                        sub += " = " + init_brace;
                    } else if (element_needs_null_init) {
                        sub += " = {0}";
                    }
                    sub += ";\n";
                }
                sub += indent + std::string(v_m_name) + "->data = " + std::string(v_m_name) + "_data;\n";
                sub += indent + std::string(v_m_name) + "->n_dims = " + std::to_string(n_dims) + ";\n";
                sub += indent + std::string(v_m_name) + "->offset = " + std::to_string(0) + ";\n";
                sub += indent + std::string(v_m_name) + "->is_allocated = "
                    + std::string((is_fixed_size || !dims.empty()) ? "true" : "false") + ";\n";
                std::string stride = "1";
                for (int i = 0; i < n_dims; i++) {
                    std::string start = "1", length = "0";
                    if( m_dims[i].m_start ) {
                        this->visit_expr(*m_dims[i].m_start);
                        start = src;
                    }
                    if( m_dims[i].m_length ) {
                        this->visit_expr(*m_dims[i].m_length);
                        length = src;
                    }
                    sub += indent + std::string(v_m_name) +
                        "->dims[" + std::to_string(i) + "].lower_bound = " + start + ";\n";
                    sub += indent + std::string(v_m_name) +
                        "->dims[" + std::to_string(i) + "].length = " + length + ";\n";
                    sub += indent + std::string(v_m_name) +
                        "->dims[" + std::to_string(i) + "].stride = " + stride + ";\n";
                    stride = "(" + stride + "*" + length + ")";
                }
                if (needs_runtime_init && init_expr != nullptr) {
                    this->visit_expr(*init_expr);
                    std::string init_src = src;
                    std::string init_name = get_unique_local_name(v_m_name + "__init");
                    sub += indent + format_type_c("", type_name, init_name, false, false)
                        + " = " + init_src + ";\n";
                    size_t total_size = ASRUtils::get_fixed_size_of_array(var->m_type);
                    std::string init_data = get_c_array_data_expr(init_expr, init_name);
                    std::string init_offset = get_c_array_offset_expr(init_expr, init_name);
                    std::string init_stride = get_c_array_stride_expr(init_expr, init_name);
                    std::string loop_var = get_unique_local_name("array_init_i");
                    sub += indent + "for (int32_t " + loop_var + " = 0; " + loop_var + " < "
                        + std::to_string(total_size) + "; " + loop_var + "++) {\n";
                    std::string inner_indent = indent + std::string(indentation_spaces, ' ');
                    std::string init_index = init_offset + " + (" + loop_var + " * " + init_stride + ")";
                    ASR::ttype_t *element_type = ASRUtils::type_get_past_array(
                        ASRUtils::type_get_past_allocatable_pointer(var->m_type));
                    if (element_type != nullptr && ASRUtils::is_character(*element_type)) {
                        std::string init_elem = init_data + "[" + init_index + "]";
                        sub += inner_indent + v_m_name + "_data[" + loop_var + "] = NULL;\n";
                        sub += inner_indent
                            + "_lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &"
                            + v_m_name + "_data[" + loop_var + "], NULL, true, true, "
                            + init_elem + ", strlen(" + init_elem + "));\n";
                    } else {
                        sub += inner_indent + v_m_name + "_data[" + loop_var + "] = "
                            + init_data + "[" + init_index + "];\n";
                    }
                    sub += indent + "}\n";
                }
                if (sub.size() >= 2 && sub.substr(sub.size() - 2) == ";\n") {
                    sub.pop_back();
                    sub.pop_back();
                }
            }
        } else {
            if( m_abi == ASR::abiType::BindC ) {
                sub = format_type_c("", type_name_copy, v_m_name + "[]", use_ref, dummy);
            } else {
                sub = format_type_c("", type_name, v_m_name, use_ref, dummy);
            }
        }
    }

    ASR::expr_t* get_variable_init_expr_raw(const ASR::Variable_t &v) {
        if (v.m_value != nullptr) {
            return v.m_value;
        }
        return v.m_symbolic_value;
    }

    ASR::expr_t* get_variable_init_value_expr(const ASR::Variable_t &v) {
        ASR::expr_t *init_expr = get_variable_init_expr_raw(v);
        if (init_expr == nullptr) {
            return nullptr;
        }
        ASR::expr_t *value_expr = ASRUtils::expr_value(init_expr);
        if (value_expr != nullptr) {
            return value_expr;
        }
        return init_expr;
    }

    std::string get_array_wrapper_type_for_variable(const ASR::Variable_t &v,
            bool as_pointer) {
        ASR::ttype_t *elem_type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable(v.m_type));
        std::string type_name;
        std::string encoded_type_name;
        if (ASR::is_a<ASR::StructType_t>(*elem_type) && v.m_type_declaration) {
            ASR::symbol_t *struct_sym =
                ASRUtils::symbol_get_past_external(v.m_type_declaration);
            type_name = "struct "
                + CUtils::get_c_symbol_name(struct_sym);
            encoded_type_name = CUtils::sanitize_c_identifier(
                "x" + CUtils::get_c_symbol_name(struct_sym));
        } else if (ASR::is_a<ASR::UnionType_t>(*elem_type) && v.m_type_declaration) {
            ASR::symbol_t *union_sym =
                ASRUtils::symbol_get_past_external(v.m_type_declaration);
            type_name = "union "
                + CUtils::get_c_symbol_name(union_sym);
            encoded_type_name = CUtils::sanitize_c_identifier(
                "x" + CUtils::get_c_symbol_name(union_sym));
        } else if (ASR::is_a<ASR::EnumType_t>(*elem_type)) {
            ASR::EnumType_t *enum_type = ASR::down_cast<ASR::EnumType_t>(elem_type);
            type_name = "enum "
                + CUtils::get_c_symbol_name(
                    ASRUtils::symbol_get_past_external(enum_type->m_enum_type));
        } else {
            type_name = CUtils::get_c_type_from_ttype_t(elem_type);
        }
        if (encoded_type_name.empty()) {
            encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
        }
        return c_ds_api->get_array_type(type_name, encoded_type_name,
            array_types_decls, as_pointer);
    }

    void emit_static_array_member_descriptor_initializers(ASR::Struct_t* der_type_t,
        std::string& decls, std::vector<std::string>& initializers,
        std::string value_name, const std::string& member_prefix = "") {
        if (der_type_t == nullptr) {
            return;
        }
        if (der_type_t->m_parent != nullptr) {
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(der_type_t->m_parent);
            if (parent_sym != nullptr && ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                emit_static_array_member_descriptor_initializers(
                    ASR::down_cast<ASR::Struct_t>(parent_sym), decls,
                    initializers, value_name, member_prefix);
            }
        }
        for (size_t i = 0; i < der_type_t->n_members; i++) {
            if (der_type_t->m_members[i] == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym = der_type_t->m_symtab->get_symbol(der_type_t->m_members[i]);
            if (member_sym == nullptr) {
                continue;
            }
            member_sym = ASRUtils::symbol_get_past_external(member_sym);
            if (!ASR::is_a<ASR::Variable_t>(*member_sym)) {
                continue;
            }
            ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            std::string emitted_member_name = CUtils::get_c_member_name(member_sym);
            ASR::ttype_t *member_type =
                ASRUtils::type_get_past_allocatable_pointer(member_var->m_type);
            if (ASRUtils::is_array(member_var->m_type)) {
                ASR::dimension_t *m_dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(
                    member_var->m_type, m_dims);
                if (ASRUtils::is_fixed_size_array(m_dims, n_dims)) {
                    continue;
                }
                std::string wrapper_type =
                    get_array_wrapper_type_for_variable(*member_var, false);
                std::string desc_name = CUtils::sanitize_c_identifier(
                    value_name + "_" + member_prefix + emitted_member_name
                    + "_desc");
                std::string dims_init;
                for (size_t dim = 0; dim < n_dims; dim++) {
                    if (!dims_init.empty()) {
                        dims_init += ", ";
                    }
                    dims_init += "{1, 0, 1}";
                }
                decls += "static " + wrapper_type + " " + desc_name + " = { ";
                decls += ".data = NULL, .dims = {" + dims_init + "}, .n_dims = "
                    + std::to_string(n_dims)
                    + ", .offset = 0, .is_allocated = false };\n";
                initializers.push_back("." + member_prefix + emitted_member_name
                    + " = &" + desc_name);
            } else if (ASR::is_a<ASR::StructType_t>(*member_type)
                    && member_var->m_type_declaration != nullptr) {
                ASR::symbol_t *struct_sym =
                    ASRUtils::symbol_get_past_external(member_var->m_type_declaration);
                if (struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                    emit_static_array_member_descriptor_initializers(
                        ASR::down_cast<ASR::Struct_t>(struct_sym), decls,
                        initializers, value_name,
                        member_prefix + emitted_member_name + ".");
                }
            }
        }
    }

    void allocate_array_members_of_struct(ASR::Struct_t* der_type_t, std::string& sub,
        std::string indent, std::string name) {
        // File scope in C only permits declarations. Skip member-level runtime
        // initialization there; static objects are already zero-initialized.
        if (indentation_level == 0) {
            return;
        }
        for (size_t i = 0; i < der_type_t->n_members; i++) {
            if (der_type_t->m_members[i] == nullptr) {
                continue;
            }
            const std::string member_name = der_type_t->m_members[i];
            ASR::symbol_t *member_sym = der_type_t->m_symtab->get_symbol(member_name);
            if (member_sym == nullptr) {
                continue;
            }
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(member_sym);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                continue;
            }
            const std::string emitted_member_name = CUtils::get_c_member_name(sym);
            ASR::ttype_t* mem_type = ASRUtils::symbol_type(sym);
            if( ASRUtils::is_array(mem_type) &&
                        ASR::is_a<ASR::Variable_t>(*member_sym) ) {
                ASR::Variable_t* mem_var = ASR::down_cast<ASR::Variable_t>(member_sym);
                std::string safe_member_name = CUtils::get_c_member_name(sym);
                std::string mem_var_name = CUtils::sanitize_c_identifier(
                    "struct_member_" + safe_member_name + "_" + std::to_string(counter));
                counter += 1;
                ASR::dimension_t* m_dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(mem_type, m_dims);
                CDeclarationOptions c_decl_options_;
                c_decl_options_.pre_initialise_derived_type = true;
                c_decl_options_.use_ptr_for_derived_type = true;
                c_decl_options_.use_static = true;
                c_decl_options_.force_declare = true;
                c_decl_options_.force_declare_name = mem_var_name;
                c_decl_options_.do_not_initialize = true;
                sub += indent + convert_variable_decl(*mem_var, &c_decl_options_) + ";\n";
                if( !ASRUtils::is_fixed_size_array(m_dims, n_dims) ) {
                    sub += indent + mem_var_name + " = _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof(*" + mem_var_name + "));\n";
                    sub += indent + "memset(" + mem_var_name + ", 0, sizeof(*" + mem_var_name + "));\n";
                    sub += indent + name + "->" + emitted_member_name + " = " + mem_var_name + ";\n";
                }
            } else if( ASRUtils::is_character(*mem_type) ) {
                sub += indent + name + "->" + emitted_member_name + " = NULL;\n";
            } else if( ASR::is_a<ASR::StructType_t>(*mem_type) ) {
                ASR::Variable_t* mem_var = ASR::down_cast<ASR::Variable_t>(sym);
                if (mem_var->m_type_declaration != nullptr) {
                    ASR::symbol_t *struct_sym =
                        ASRUtils::symbol_get_past_external(mem_var->m_type_declaration);
                    if (struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                        allocate_array_members_of_struct(
                            ASR::down_cast<ASR::Struct_t>(struct_sym), sub, indent,
                            "(&(" + name + "->" + emitted_member_name + "))");
                    }
                }
            }
        }
    }

    void initialize_struct_instance_members(ASR::Struct_t* der_type_t, std::string& sub,
        std::string indent, std::string name) {
        if (der_type_t == nullptr) {
            return;
        }
        if (der_type_t->m_parent != nullptr) {
            ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(der_type_t->m_parent);
            if (parent_sym != nullptr && ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                initialize_struct_instance_members(
                    ASR::down_cast<ASR::Struct_t>(parent_sym), sub, indent, name);
            }
        }
        for (size_t i = 0; i < der_type_t->n_members; i++) {
            if (der_type_t->m_members[i] == nullptr) {
                continue;
            }
            ASR::symbol_t *member_sym = der_type_t->m_symtab->get_symbol(der_type_t->m_members[i]);
            if (member_sym == nullptr) {
                continue;
            }
            member_sym = ASRUtils::symbol_get_past_external(member_sym);
            if (!ASR::is_a<ASR::Variable_t>(*member_sym)) {
                continue;
            }
            ASR::Variable_t *member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            ASR::ttype_t *member_type = ASRUtils::type_get_past_allocatable_pointer(member_var->m_type);
            bool is_plain_struct_member = !ASRUtils::is_array(member_var->m_type)
                && !ASRUtils::is_allocatable(member_var->m_type)
                && !ASRUtils::is_pointer(member_var->m_type)
                && ASR::is_a<ASR::StructType_t>(*member_type)
                && !ASRUtils::is_class_type(member_type);
            if (is_plain_struct_member && member_var->m_type_declaration != nullptr) {
                ASR::symbol_t *struct_sym =
                    ASRUtils::symbol_get_past_external(member_var->m_type_declaration);
                if (struct_sym != nullptr && ASR::is_a<ASR::Struct_t>(*struct_sym)) {
                    std::string member_access = name + "->"
                        + CUtils::get_c_member_name(member_sym);
                    sub += indent + member_access + "."
                        + get_runtime_type_tag_member_name() + " = "
                        + std::to_string(get_struct_runtime_type_id(struct_sym)) + ";\n";
                    initialize_struct_instance_members(
                        ASR::down_cast<ASR::Struct_t>(struct_sym), sub, indent,
                        "(&(" + member_access + "))");
                }
                continue;
            }
            ASR::expr_t *init_expr = get_variable_init_value_expr(*member_var);
            if (init_expr == nullptr) {
                continue;
            }
            if (ASRUtils::is_array(member_var->m_type)
                    || ASRUtils::is_allocatable(member_var->m_type)
                    || ASRUtils::is_pointer(member_var->m_type)
                    || ASR::is_a<ASR::StructType_t>(*member_type)
                    || ASRUtils::is_class_type(member_type)
                    || ASR::is_a<ASR::UnionType_t>(*member_type)) {
                continue;
            }
            this->visit_expr(*init_expr);
            sub += indent + name + "->" + CUtils::get_c_member_name(member_sym)
                + " = " + src + ";\n";
        }
        allocate_array_members_of_struct(der_type_t, sub, indent, name);
    }

    void emit_function_arg_initialization(const ASR::Function_t &x,
            std::string &sub, const std::string &indent) {
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
            if (arg_var->m_intent != ASRUtils::intent_out) {
                continue;
            }
            if (ASRUtils::is_allocatable(arg_var->m_type)
                    || ASRUtils::is_pointer(arg_var->m_type)) {
                continue;
            }
            ASR::ttype_t *arg_type =
                ASRUtils::type_get_past_allocatable_pointer(arg_var->m_type);
            if (!ASR::is_a<ASR::StructType_t>(*arg_type)) {
                continue;
            }
            ASR::StructType_t *struct_type = ASR::down_cast<ASR::StructType_t>(arg_type);
            if (struct_type->m_is_unlimited_polymorphic) {
                continue;
            }
            if (arg_var->m_type_declaration == nullptr) {
                continue;
            }
            ASR::symbol_t *derived_type = ASRUtils::symbol_get_past_external(
                arg_var->m_type_declaration);
            if (derived_type == nullptr || !ASR::is_a<ASR::Struct_t>(*derived_type)) {
                continue;
            }
            headers.insert("string.h");
            std::string arg_name = CUtils::get_c_variable_name(*arg_var);
            // A Fortran `intent(out)` derived dummy becomes undefined on entry
            // except for default-initialized components. Zero the pointee first
            // so generated allocatable/pointer component checks start from the
            // unallocated state before we restore any explicit defaults below.
            sub += indent + "memset(" + arg_name + ", 0, sizeof(*" + arg_name + "));\n";
            sub += indent + arg_name + "->" + get_runtime_type_tag_member_name()
                + " = " + std::to_string(get_struct_runtime_type_id(derived_type)) + ";\n";
            initialize_struct_instance_members(
                ASR::down_cast<ASR::Struct_t>(derived_type), sub, indent,
                arg_name);
        }
    }

    void convert_variable_decl_util(const ASR::Variable_t &v,
        bool is_array, bool declare_as_constant, bool use_ref, bool dummy,
        bool force_declare, std::string &force_declare_name,
        size_t n_dims, ASR::dimension_t* m_dims, ASR::ttype_t* v_m_type,
        std::string &dims, std::string &sub) {
        std::string v_name = get_variable_c_name(v);
        std::string type_name;
        if (ASR::is_a<ASR::StructType_t>(*v_m_type) &&
                ASR::down_cast<ASR::StructType_t>(v_m_type)->m_is_unlimited_polymorphic) {
            type_name = CUtils::get_c_type_from_ttype_t(v_m_type);
        } else if (ASR::is_a<ASR::StructType_t>(*v_m_type) && v.m_type_declaration) {
            type_name = "struct "
                + CUtils::get_c_symbol_name(
                    ASRUtils::symbol_get_past_external(v.m_type_declaration));
        } else if (ASR::is_a<ASR::StructType_t>(*v_m_type)
                && get_variable_init_value_expr(v)
                && ASR::is_a<ASR::StructConstant_t>(*get_variable_init_value_expr(v))) {
            ASR::StructConstant_t *sc = ASR::down_cast<ASR::StructConstant_t>(
                get_variable_init_value_expr(v));
            type_name = "struct "
                + CUtils::get_c_symbol_name(
                    ASRUtils::symbol_get_past_external(sc->m_dt_sym));
        } else if (ASR::is_a<ASR::UnionType_t>(*v_m_type) && v.m_type_declaration) {
            type_name = "union "
                + CUtils::get_c_symbol_name(
                    ASRUtils::symbol_get_past_external(v.m_type_declaration));
        } else if (ASR::is_a<ASR::EnumType_t>(*v_m_type)) {
            ASR::EnumType_t *enum_type = ASR::down_cast<ASR::EnumType_t>(v_m_type);
            type_name = "enum "
                + CUtils::get_c_symbol_name(
                    ASRUtils::symbol_get_past_external(enum_type->m_enum_type));
        } else {
            type_name = CUtils::get_c_type_from_ttype_t(v_m_type);
        }
        if( is_array ) {
                bool is_fixed_size = true;
                bool is_struct_type_member = ASR::is_a<ASR::Struct_t>(
                    *ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner));
                bool is_module_var = ASR::is_a<ASR::Module_t>(
                    *ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner));
                bool use_dynamic_stack_storage = v.m_intent == ASRUtils::intent_local
                    && !dummy && !force_declare && !is_struct_type_member
                    && !is_module_var && v.m_storage != ASR::storage_typeType::Parameter
                    && !ASRUtils::is_allocatable(v.m_type)
                    && !ASRUtils::is_pointer(v.m_type)
                    && !ASRUtils::is_character(*v_m_type)
                    && !ASR::is_a<ASR::StructType_t>(*v_m_type)
                    && !ASR::is_a<ASR::UnionType_t>(*v_m_type)
                    && !ASR::is_a<ASR::EnumType_t>(*v_m_type);
                std::string dynamic_stack_size_expr;
                dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size,
                    true, use_dynamic_stack_storage ? &dynamic_stack_size_expr : nullptr);
                if( is_fixed_size && is_struct_type_member ) {
                    if( !force_declare ) {
                        force_declare_name = v_name;
                    }
                    sub = type_name + " " + force_declare_name + dims;
                } else if (is_struct_type_member) {
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    if( !force_declare ) {
                        force_declare_name = v_name;
                    }
                    std::string array_type =
                        c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                    sub = format_type_c("", array_type, force_declare_name, use_ref, dummy);
                } else {
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    if( !force_declare ) {
                        force_declare_name = v_name;
                    }
                bool is_simd_array = (ASR::is_a<ASR::Array_t>(*v.m_type) &&
                    ASR::down_cast<ASR::Array_t>(v.m_type)->m_physical_type
                        == ASR::array_physical_typeType::SIMDArray);
                generate_array_decl(sub, force_declare_name, type_name, dims,
                                    encoded_type_name, m_dims, n_dims,
                                    use_ref, dummy,
                                    (v.m_intent != ASRUtils::intent_in &&
                                    v.m_intent != ASRUtils::intent_inout &&
                                    v.m_intent != ASRUtils::intent_out &&
                                    v.m_intent != ASRUtils::intent_unspecified &&
                                    !is_struct_type_member && !is_module_var) || force_declare,
                                    is_fixed_size, false, ASR::abiType::Source, is_simd_array,
                                    &v, dynamic_stack_size_expr);
            }
        } else {
            bool is_fixed_size = true;
            std::string v_m_name = v_name;
            if( declare_as_constant ) {
                type_name = "const " + type_name;
                v_m_name = const_name;
            }
            dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
            sub = format_type_c(dims, type_name, v_m_name, use_ref, dummy);
        }
    }

    bool needs_null_init_for_local_pointer_like_variable(const ASR::Variable_t &v,
            ASR::ttype_t *storage_type, bool is_array, bool dummy,
            bool is_struct_type_member, bool do_not_initialize) {
        bool is_local_or_return_var = v.m_intent == ASRUtils::intent_local
            || v.m_intent == ASRUtils::intent_return_var;
        if (do_not_initialize || dummy || is_array || is_struct_type_member
                || !is_local_or_return_var) {
            return false;
        }
        ASR::expr_t *init_expr = get_variable_init_value_expr(v);
        if (init_expr != nullptr) {
            return false;
        }
        if (ASRUtils::is_character(*storage_type)) {
            return false;
        }
        if (ASRUtils::is_allocatable(v.m_type) || ASRUtils::is_pointer(v.m_type)) {
            return true;
        }
        if (ASRUtils::is_class_type(storage_type) || ASR::is_a<ASR::CPtr_t>(*storage_type)) {
            return true;
        }
        return false;
    }

    std::string get_function_pointer_type_from_type(const ASR::FunctionType_t *ft) {
        std::string ret_type = "void";
        if (ft->m_return_var_type) {
            ret_type = CUtils::get_c_type_from_ttype_t(ft->m_return_var_type);
        }
        std::string decl = ret_type + " (*)(";
        for (size_t i = 0; i < ft->n_arg_types; i++) {
            ASR::ttype_t *arg_type = ft->m_arg_types[i];
            if (ASR::is_a<ASR::FunctionType_t>(*arg_type)) {
                decl += get_function_pointer_type_from_type(
                    ASR::down_cast<ASR::FunctionType_t>(arg_type));
            } else if (ASR::is_a<ASR::Pointer_t>(*arg_type)
                    && ASR::is_a<ASR::FunctionType_t>(
                        *ASR::down_cast<ASR::Pointer_t>(arg_type)->m_type)) {
                decl += get_function_pointer_type_from_type(
                    ASR::down_cast<ASR::FunctionType_t>(
                        ASR::down_cast<ASR::Pointer_t>(arg_type)->m_type));
            } else if (ASRUtils::is_character(*arg_type) && ASRUtils::is_allocatable(arg_type)) {
                decl += "char **";
            } else if (ASR::is_a<ASR::Allocatable_t>(*arg_type)) {
                decl += CUtils::get_c_type_from_ttype_t(ASR::down_cast<ASR::Allocatable_t>(arg_type)->m_type) + "*";
            } else {
                decl += CUtils::get_c_type_from_ttype_t(arg_type);
            }
            if (i < ft->n_arg_types - 1) decl += ", ";
        }
        decl += ")";
        return decl;
    }

    std::string get_function_pointer_declaration_from_type(
            const ASR::FunctionType_t *ft, const std::string &name,
            bool extra_indirection=false) {
        std::string decl = get_function_pointer_type_from_type(ft);
        size_t marker = decl.find("(*)");
        if (marker == std::string::npos) {
            throw CodeGenError("Malformed function pointer declaration");
        }
        decl.replace(marker, 3, (extra_indirection ? "(**" : "(*") + name + ")");
        return decl;
    }

    std::string get_function_pointer_declaration_from_interface(
            const ASR::Function_t &iface_fn, const std::string &name,
            bool extra_indirection=false) {
        ASR::FunctionType_t *ft = ASRUtils::get_FunctionType(iface_fn);
        std::string ret_type = "void";
        if (iface_fn.m_return_var) {
            ASR::Variable_t *return_var = ASRUtils::EXPR2VAR(iface_fn.m_return_var);
            ret_type = get_return_var_type(return_var);
        } else if (ft->m_return_var_type) {
            ret_type = CUtils::get_c_type_from_ttype_t(ft->m_return_var_type) + " ";
        }
        std::string decl = ret_type + (extra_indirection ? " (**" : " (*") + name + ")(";
        for (size_t i = 0; i < iface_fn.n_args; i++) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(iface_fn.m_args[i])->m_v);
            if (!ASR::is_a<ASR::Variable_t>(*sym)) {
                throw CodeGenError("Unsupported function pointer interface argument");
            }
            ASR::Variable_t *arg = ASR::down_cast<ASR::Variable_t>(sym);
            CDeclarationOptions c_decl_options;
            c_decl_options.pre_initialise_derived_type = false;
            decl += convert_variable_decl(*arg, &c_decl_options);
            if (i < iface_fn.n_args - 1) decl += ", ";
        }
        decl += ")";
        return decl;
    }

    std::string convert_variable_decl(const ASR::Variable_t &v,
        DeclarationOptions* decl_options=nullptr)
    {
        bool pre_initialise_derived_type;
        bool use_ptr_for_derived_type;
        bool use_static;
        bool force_declare;
        std::string force_declare_name;
        bool declare_as_constant;
        std::string const_name;
        bool do_not_initialize;

        if( decl_options ) {
            CDeclarationOptions* c_decl_options = reinterpret_cast<CDeclarationOptions*>(decl_options);
            pre_initialise_derived_type = c_decl_options->pre_initialise_derived_type;
            use_ptr_for_derived_type = c_decl_options->use_ptr_for_derived_type;
            use_static = c_decl_options->use_static;
            force_declare = c_decl_options->force_declare;
            force_declare_name = c_decl_options->force_declare_name;
            declare_as_constant = c_decl_options->declare_as_constant;
            const_name = c_decl_options->const_name;
            do_not_initialize = c_decl_options->do_not_initialize;
        } else {
            pre_initialise_derived_type = true;
            use_ptr_for_derived_type = true;
            use_static = true;
            force_declare = false;
            force_declare_name = "";
            declare_as_constant = false;
            const_name = "";
            do_not_initialize = false;
        }
        std::string sub;
        bool use_ref = (v.m_intent == ASRUtils::intent_out ||
                        v.m_intent == ASRUtils::intent_inout);
        if (is_c && is_pointer_dummy_slot_type(&v)) {
            use_ref = true;
        }
        if (is_c
                && ASRUtils::is_arg_dummy(v.m_intent)
                && v.m_intent != ASRUtils::intent_in
                && ASRUtils::is_allocatable(v.m_type)
                && !ASRUtils::is_array(v.m_type)) {
            ASR::ttype_t *alloc_value_type =
                ASRUtils::type_get_past_allocatable_pointer(v.m_type);
            if (alloc_value_type
                    && !ASRUtils::is_character(*alloc_value_type)) {
                use_ref = true;
            }
        }
        ASR::asr_t *owner = v.m_parent_symtab ? v.m_parent_symtab->asr_owner : nullptr;
        ASR::symbol_t *owner_sym = CUtils::get_symbol_owner(owner);
        if (is_c && owner_sym && ASR::is_a<ASR::Function_t>(*owner_sym)
                && std::string(ASR::down_cast<ASR::Function_t>(owner_sym)->m_name)
                    .rfind("_lcompilers_move_alloc_", 0) == 0
                && get_variable_c_name(v) == "to"
                && (!ASRUtils::is_array(v.m_type)
                    || is_unlimited_polymorphic_storage_type(v.m_type))) {
            use_ref = true;
        }
        bool declaration_only = do_not_initialize;
        bool is_array = ASRUtils::is_array(v.m_type);
        bool dummy = ASRUtils::is_arg_dummy(v.m_intent);
        bool is_procedure_pointer_dummy_slot = is_c
            && dummy
            && (v.m_intent == ASRUtils::intent_out
                || v.m_intent == ASRUtils::intent_inout);
        std::string c_v_name = get_variable_c_name(v);
        std::string decl_name = (force_declare && !force_declare_name.empty())
            ? force_declare_name : c_v_name;
        if (is_c && owner_sym && ASR::is_a<ASR::Function_t>(*owner_sym)
                && std::string(ASR::down_cast<ASR::Function_t>(owner_sym)->m_name)
                    .rfind("_lcompilers_move_alloc_", 0) == 0
                && decl_name == "to"
                && is_unlimited_polymorphic_storage_type(v.m_type)) {
            return "void** " + decl_name;
        }
        bool is_struct_type_member = ASR::is_a<ASR::Struct_t>(
            *ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner));
        bool is_bindc_dummy = false;
        if (dummy && ASR::is_a<ASR::symbol_t>(*v.m_parent_symtab->asr_owner)) {
            ASR::symbol_t *owner_sym = ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner);
            owner_sym = ASRUtils::symbol_get_past_external(owner_sym);
            if (ASR::is_a<ASR::Function_t>(*owner_sym)) {
                ASR::Function_t *owner_fn = ASR::down_cast<ASR::Function_t>(owner_sym);
                is_bindc_dummy = ASRUtils::get_FunctionType(*owner_fn)->m_abi == ASR::abiType::BindC;
            }
        }
        bool is_len_one_char_array = CUtils::is_len_one_character_array_type(v.m_type);
        ASR::dimension_t* m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(v.m_type, m_dims);
        ASR::ttype_t* v_storage_type = ASRUtils::type_get_past_allocatable_pointer(v.m_type);
        bool force_pointer_backed_struct = ASRUtils::is_allocatable(v.m_type)
            || ASRUtils::is_pointer(v.m_type)
            || ASRUtils::is_class_type(v_storage_type);
        ASR::ttype_t* v_m_type = v.m_type;
        v_m_type = ASRUtils::type_get_past_array(ASRUtils::type_get_past_allocatable(v_m_type));
        ASR::expr_t *var_init_raw = get_variable_init_expr_raw(v);
        ASR::expr_t *var_init_value = get_variable_init_value_expr(v);
        bool is_bindc_optional_scalar = is_bindc_dummy
            && v.m_presence == ASR::presenceType::Optional
            && !is_array
            && !ASRUtils::is_aggregate_type(v.m_type)
            && !ASRUtils::is_pointer(v.m_type)
            && !ASRUtils::is_allocatable(v.m_type)
            && !ASRUtils::is_character(*ASRUtils::type_get_past_allocatable_pointer(v.m_type))
            && !ASR::is_a<ASR::CPtr_t>(*ASRUtils::type_get_past_allocatable_pointer(v.m_type));
        if (!is_array && v.m_storage == ASR::storage_typeType::Parameter
                && var_init_raw
                && var_init_raw->type == ASR::exprType::StructConstant) {
            ASR::StructConstant_t *struct_const =
                ASR::down_cast<ASR::StructConstant_t>(var_init_raw);
            ASR::symbol_t *struct_sym = ASRUtils::symbol_get_past_external(struct_const->m_dt_sym);
            sub = format_type_c("", "const struct " + CUtils::get_c_symbol_name(struct_sym),
                                c_v_name, false, false);
            this->visit_expr(*var_init_raw);
            sub += " = " + src;
            return sub;
        }
        if (ASR::is_a<ASR::FunctionType_t>(*v_m_type)) {
            ASR::Function_t *iface_fn = nullptr;
            if (v.m_type_declaration) {
                ASR::symbol_t *iface_sym = ASRUtils::symbol_get_past_external(v.m_type_declaration);
                if (ASR::is_a<ASR::Function_t>(*iface_sym)) {
                    iface_fn = ASR::down_cast<ASR::Function_t>(iface_sym);
                }
            }
            if (iface_fn) {
                return get_function_pointer_declaration_from_interface(*iface_fn,
                    c_v_name);
            }
            return get_function_pointer_declaration_from_type(
                ASR::down_cast<ASR::FunctionType_t>(v_m_type),
                c_v_name);
        }
        if (ASRUtils::is_allocatable(v.m_type) && !is_array
                && !ASRUtils::is_character(*v_m_type)
                && !ASR::is_a<ASR::StructType_t>(*v_m_type)
                && !ASR::is_a<ASR::UnionType_t>(*v_m_type)
                && !ASR::is_a<ASR::EnumType_t>(*v_m_type)) {
            std::string dims;
            bool null_init_local_pointer_like =
                needs_null_init_for_local_pointer_like_variable(
                    v, v_m_type, is_array, dummy, is_struct_type_member, do_not_initialize);
            if (ASRUtils::is_integer(*v_m_type)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v_m_type);
                sub = format_type_c(dims, "int" + std::to_string(t->m_kind * 8) + "_t *",
                    decl_name, use_ref, dummy);
            } else if (ASRUtils::is_unsigned_integer(*v_m_type)) {
                ASR::UnsignedInteger_t *t = ASR::down_cast<ASR::UnsignedInteger_t>(v_m_type);
                sub = format_type_c(dims, "uint" + std::to_string(t->m_kind * 8) + "_t *",
                    decl_name, use_ref, dummy);
            } else if (ASRUtils::is_real(*v_m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v_m_type);
                std::string type_name;
                if (t->m_kind == 4) {
                    type_name = "float *";
                } else if (t->m_kind == 8) {
                    type_name = "double *";
                } else {
                    diag.codegen_error_label("Real kind '"
                        + std::to_string(t->m_kind)
                        + "' not supported", {v.base.base.loc}, "");
                    throw Abort();
                }
                sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
            } else if (ASRUtils::is_complex(*v_m_type)) {
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v_m_type);
                std::string type_name;
                if (t->m_kind == 4) {
                    type_name = "float complex *";
                } else if (t->m_kind == 8) {
                    type_name = "double complex *";
                } else {
                    diag.codegen_error_label("Complex kind '"
                        + std::to_string(t->m_kind)
                        + "' not supported", {v.base.base.loc}, "");
                    throw Abort();
                }
                headers.insert("complex.h");
                sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
            } else if (ASRUtils::is_logical(*v_m_type)) {
                headers.insert("stdbool.h");
                sub = format_type_c(dims, "bool *", decl_name, use_ref, dummy);
            }
            if (null_init_local_pointer_like) {
                sub += " = NULL";
            }
            return sub;
        }
        if (is_bindc_optional_scalar) {
            std::string dims;
            if (ASRUtils::is_integer(*v_m_type)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v_m_type);
                return format_type_c(dims, "int" + std::to_string(t->m_kind * 8) + "_t *",
                    decl_name, false, dummy);
            } else if (ASRUtils::is_unsigned_integer(*v_m_type)) {
                ASR::UnsignedInteger_t *t = ASR::down_cast<ASR::UnsignedInteger_t>(v_m_type);
                return format_type_c(dims, "uint" + std::to_string(t->m_kind * 8) + "_t *",
                    decl_name, false, dummy);
            } else if (ASRUtils::is_real(*v_m_type)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(v_m_type);
                if (t->m_kind == 4) {
                    return format_type_c(dims, "float *", decl_name, false, dummy);
                } else if (t->m_kind == 8) {
                    return format_type_c(dims, "double *", decl_name, false, dummy);
                }
            } else if (ASRUtils::is_complex(*v_m_type)) {
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(v_m_type);
                headers.insert("complex.h");
                if (t->m_kind == 4) {
                    return format_type_c(dims, "float complex *", decl_name, false, dummy);
                } else if (t->m_kind == 8) {
                    return format_type_c(dims, "double complex *", decl_name, false, dummy);
                }
            } else if (ASRUtils::is_logical(*v_m_type)) {
                headers.insert("stdbool.h");
                return format_type_c(dims, "bool *", decl_name, false, dummy);
            }
        }
        if (ASRUtils::is_pointer(v_m_type)) {
            ASR::ttype_t *t2 = ASR::down_cast<ASR::Pointer_t>(v_m_type)->m_type;
            t2 = ASRUtils::type_get_past_array(t2);
            if (ASR::is_a<ASR::FunctionType_t>(*t2)) {
                ASR::Function_t *iface_fn = nullptr;
                if (v.m_type_declaration) {
                    ASR::symbol_t *iface_sym = ASRUtils::symbol_get_past_external(v.m_type_declaration);
                    if (ASR::is_a<ASR::Function_t>(*iface_sym)) {
                        iface_fn = ASR::down_cast<ASR::Function_t>(iface_sym);
                    }
                }
                if (iface_fn) {
                    return get_function_pointer_declaration_from_interface(*iface_fn,
                        c_v_name, is_procedure_pointer_dummy_slot);
                }
                return get_function_pointer_declaration_from_type(
                    ASR::down_cast<ASR::FunctionType_t>(t2),
                    c_v_name, is_procedure_pointer_dummy_slot);
            }
            if (ASR::is_a<ASR::StructType_t>(*t2) &&
                    ASR::down_cast<ASR::StructType_t>(t2)->m_is_unlimited_polymorphic) {
                return format_type_c("", "void*", decl_name, use_ref, dummy);
            }
            if (ASRUtils::is_integer(*t2)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(ASRUtils::type_get_past_array(t2));
                std::string type_name = "int" + std::to_string(t->m_kind * 8) + "_t";
                if( !is_array ) {
                    type_name.append(" *");
                }
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    if (declaration_only || dummy) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified, is_fixed_size, true, ASR::abiType::Source, false, &v);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_unsigned_integer(*t2)) {
                ASR::UnsignedInteger_t *t = ASR::down_cast<ASR::UnsignedInteger_t>(ASRUtils::type_get_past_array(t2));
                std::string type_name = "uint" + std::to_string(t->m_kind * 8) + "_t";
                if( !is_array ) {
                    type_name.append(" *");
                }
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    if (declaration_only || dummy) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, true, ASR::abiType::Source, false, &v);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_real(*t2)) {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(t2);
                std::string type_name;
                if (t->m_kind == 4) {
                    type_name = "float";
                } else if (t->m_kind == 8) {
                    type_name = "double";
                } else {
                    diag.codegen_error_label("Real kind '"
                        + std::to_string(t->m_kind)
                        + "' not supported", {v.base.base.loc}, "");
                    throw Abort();
                }
                if( !is_array ) {
                    type_name.append(" *");
                }
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    bool is_simd_array = (ASR::is_a<ASR::Array_t>(*v_m_type) &&
                        ASR::down_cast<ASR::Array_t>(v_m_type)->m_physical_type
                            == ASR::array_physical_typeType::SIMDArray);
                    if (declaration_only || dummy) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, true, ASR::abiType::Source, is_simd_array, &v);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_complex(*t2)) {
                ASR::Complex_t *t = ASR::down_cast<ASR::Complex_t>(t2);
                std::string type_name;
                if (t->m_kind == 4) {
                    type_name = "float complex";
                } else if (t->m_kind == 8) {
                    type_name = "double complex";
                } else {
                    diag.codegen_error_label("Complex kind '"
                        + std::to_string(t->m_kind)
                        + "' not supported", {v.base.base.loc}, "");
                    throw Abort();
                }
                headers.insert("complex.h");
                if( !is_array ) {
                    type_name.append(" *");
                }
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    if (declaration_only || dummy) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, true, ASR::abiType::Source, false, &v);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_logical(*t2)) {
                headers.insert("stdbool.h");
                std::string type_name = "bool";
                if( !is_array ) {
                    type_name.append(" *");
                }
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    if (declaration_only || dummy) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, true, ASR::abiType::Source, false, &v);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_character(*t2)) {
                std::string type_name = "char *";
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::get_c_array_type_code(v.m_type);
                    if (declaration_only || dummy) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, true, ASR::abiType::Source, false, &v);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if(ASR::is_a<ASR::StructType_t>(*t2)) {
                if (ASR::down_cast<ASR::StructType_t>(t2)->m_is_unlimited_polymorphic) {
                    sub = format_type_c("", "void*", decl_name, false, dummy);
                    return sub;
                }
                std::string der_type_name = CUtils::get_c_symbol_name(v.m_type_declaration);
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::sanitize_c_identifier("x" + der_type_name);
                    std::string type_name = std::string("struct ") + der_type_name;
                    if (is_struct_type_member || declaration_only || dummy) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, false, ASR::abiType::Source, false, &v);
                    }
                 } else {
                    std::string ptr_char = "*";
                    if( !use_ptr_for_derived_type && !ASRUtils::is_pointer(v.m_type) ) {
                        ptr_char.clear();
                    }
                    bool decl_use_ref = use_ref;
                    if (ptr_char == "*"
                            && !ASRUtils::is_pointer(v.m_type)
                            && !ASRUtils::is_allocatable(v.m_type)) {
                        decl_use_ref = false;
                    }
                    sub = format_type_c("", "struct " + der_type_name + ptr_char,
                                        decl_name, decl_use_ref, dummy);
                }
            } else if(ASR::is_a<ASR::CPtr_t>(*t2)) {
                sub = format_type_c("", "void**", decl_name, false, false);
            } else {
                diag.codegen_error_label("Type '"
                    + ASRUtils::type_to_str_python_symbol(t2, v.m_type_declaration)
                    + "' not supported", {v.base.base.loc}, "");
                throw Abort();
            }
        } else {
            std::string dims;
            use_ref = use_ref && !is_array;
            if (v.m_storage == ASR::storage_typeType::Parameter) {
                convert_variable_decl_util(v, is_array, declare_as_constant, use_ref, dummy,
                    force_declare, force_declare_name, n_dims, m_dims, v_m_type, dims, sub);
                if (v.m_intent != ASR::intentType::ReturnVar && !is_array) {
                    sub = "const " + sub;
                }
            } else if (ASRUtils::is_integer(*v_m_type)) {
                headers.insert("inttypes.h");
                convert_variable_decl_util(v, is_array, declare_as_constant, use_ref, dummy,
                    force_declare, force_declare_name, n_dims, m_dims, v_m_type, dims, sub);
            } else if (ASRUtils::is_unsigned_integer(*v_m_type)) {
                headers.insert("inttypes.h");
                convert_variable_decl_util(v, is_array, declare_as_constant, use_ref, dummy,
                    force_declare, force_declare_name, n_dims, m_dims, v_m_type, dims, sub);
            } else if (ASRUtils::is_real(*v_m_type)) {
                convert_variable_decl_util(v, is_array, declare_as_constant, use_ref, dummy,
                    force_declare, force_declare_name, n_dims, m_dims, v_m_type, dims, sub);
            } else if (ASRUtils::is_complex(*v_m_type)) {
                headers.insert("complex.h");
                convert_variable_decl_util(v, is_array, declare_as_constant, use_ref, dummy,
                    force_declare, force_declare_name, n_dims, m_dims, v_m_type, dims, sub);
            } else if (ASRUtils::is_logical(*v_m_type)) {
                convert_variable_decl_util(v, is_array, declare_as_constant, use_ref, dummy,
                    force_declare, force_declare_name, n_dims, m_dims, v_m_type, dims, sub);
            } else if (ASRUtils::is_character(*v_m_type)) {
                if (is_array) {
                    if (is_bindc_dummy && is_len_one_char_array) {
                        sub = format_type_c("", "char*", decl_name, use_ref, dummy);
                        return sub;
                    }
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::get_c_type_code(v.m_type);
                    std::string type_name = "char *";
                    if (declaration_only || dummy || is_struct_type_member) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, false, ASR::abiType::Source, false);
                    }
                } else {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, "char *", decl_name, use_ref, dummy);
                    if((v.m_intent == ASRUtils::intent_local
                            || v.m_intent == ASRUtils::intent_return_var) &&
                        !(ASR::is_a<ASR::symbol_t>(*v.m_parent_symtab->asr_owner) &&
                          ASR::is_a<ASR::Struct_t>(
                            *ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner))) &&
                        !(dims.size() == 0 && v.m_symbolic_value) && !do_not_initialize) {
                        sub += " = NULL";
                        return sub;
                    }
                }
            } else if (ASR::is_a<ASR::StructType_t>(*v_m_type)) {
                std::string indent(indentation_level*indentation_spaces, ' ');
                std::string der_type_name = CUtils::get_c_symbol_name(v.m_type_declaration);
                if (ASR::down_cast<ASR::StructType_t>(v_m_type)->m_is_unlimited_polymorphic) {
                    sub = format_type_c("", "void*", decl_name, use_ref, dummy);
                } else if( is_array ) {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::sanitize_c_identifier("x" + der_type_name);
                    std::string type_name = std::string("struct ") + der_type_name;
                    if (is_struct_type_member || declaration_only) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, decl_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, decl_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified,
                                            is_fixed_size, false, ASR::abiType::Source, false);
                    }
                } else if (declaration_only) {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    std::string ptr_char = force_pointer_backed_struct ? "*" : "";
                    sub = format_type_c(dims, "struct " + der_type_name + ptr_char,
                                        decl_name, false, dummy);
                } else if( v.m_intent == ASRUtils::intent_local && pre_initialise_derived_type) {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    bool is_file_scope_static = dims.empty()
                        && use_static
                        && indentation_level == 0;
                    bool force_value_struct_temp =
                        std::string(v.m_name).find("__libasr_created__struct_constructor_") != std::string::npos ||
                        std::string(v.m_name).find("temp_struct_var__") != std::string::npos;
                    if (force_value_struct_temp) {
                        sub = format_type_c(dims, "struct " + der_type_name,
                                            decl_name, false, dummy);
                        if (var_init_raw && !do_not_initialize) {
                            this->visit_expr(*var_init_raw);
                            std::string init = src;
                            sub += "=" + init;
                        }
                        sub += ";\n";
                        sub += indent + decl_name + "." + get_runtime_type_tag_member_name()
                            + " = " + std::to_string(get_struct_runtime_type_id(v.m_type_declaration))
                            + ";\n";
                        ASR::Struct_t* der_type_t = ASR::down_cast<ASR::Struct_t>(
                            ASRUtils::symbol_get_past_external(v.m_type_declaration));
                        allocate_array_members_of_struct(der_type_t, sub, indent, "(&(" + decl_name + "))");
                        sub.pop_back();
                        sub.pop_back();
                        return sub;
                    }
                    if (force_pointer_backed_struct) {
                        emitted_pointer_backed_struct_names.insert(decl_name);
                        sub = format_type_c(dims, "struct " + der_type_name + "*",
                                            decl_name, use_ref, dummy);
                        if (!do_not_initialize) {
                            sub += " = NULL";
                        }
                        return sub;
                    }
                    std::string value_var_name = v.m_parent_symtab->get_unique_name(decl_name + "_value");
                    if (is_file_scope_static) {
                        std::string static_member_descs;
                        std::vector<std::string> static_member_inits;
                        ASR::Struct_t* der_type_t = ASR::down_cast<ASR::Struct_t>(
                            ASRUtils::symbol_get_past_external(v.m_type_declaration));
                        emit_static_array_member_descriptor_initializers(
                            der_type_t, static_member_descs, static_member_inits,
                            value_var_name);
                        sub = static_member_descs;
                        sub += format_type_c(dims, "struct " + der_type_name,
                                             value_var_name, use_ref, dummy);
                        sub += " = { ." + get_runtime_type_tag_member_name()
                            + " = " + std::to_string(get_struct_runtime_type_id(v.m_type_declaration));
                        for (const std::string &member_init : static_member_inits) {
                            sub += ", " + member_init;
                        }
                        sub += " };\n";
                        std::string ptr_char = "*";
                        if( !use_ptr_for_derived_type ) {
                            ptr_char.clear();
                        }
                        sub += indent + format_type_c("", "struct " + der_type_name + ptr_char,
                            decl_name, use_ref, dummy);
                        emitted_pointer_backed_struct_names.insert(decl_name);
                        sub += " = &" + value_var_name;
                        return sub;
                    }
                    sub = format_type_c(dims, "struct " + der_type_name,
                                        value_var_name, use_ref, dummy);
                    bool has_decl_init = var_init_raw && !do_not_initialize;
                    if (var_init_raw && !do_not_initialize) {
                        this->visit_expr(*var_init_raw);
                        std::string init = src;
                        sub += "=" + init;
                    }
                    sub += ";\n";
                    ASR::Struct_t* der_type_t = ASR::down_cast<ASR::Struct_t>(
                        ASRUtils::symbol_get_past_external(v.m_type_declaration));
                    if (!has_decl_init) {
                        headers.insert("string.h");
                        sub += indent + "memset(&" + value_var_name + ", 0, sizeof("
                            + value_var_name + "));\n";
                    }
                    sub += indent + value_var_name + "." + get_runtime_type_tag_member_name()
                        + " = " + std::to_string(get_struct_runtime_type_id(v.m_type_declaration))
                        + ";\n";
                    if (!has_decl_init) {
                        initialize_struct_instance_members(
                            der_type_t, sub, indent, "(&(" + value_var_name + "))");
                    }
                    std::string ptr_char = "*";
                    if( !use_ptr_for_derived_type ) {
                        ptr_char.clear();
                    }
                    sub += indent + format_type_c("", "struct " + der_type_name + ptr_char, decl_name, use_ref, dummy);
                    if( n_dims != 0 ) {
                        sub += " = " + value_var_name;
                    } else {
                        emitted_pointer_backed_struct_names.insert(decl_name);
                        sub += " = &" + value_var_name + ";\n";
                        allocate_array_members_of_struct(der_type_t, sub, indent, decl_name);
                        sub.pop_back();
                        sub.pop_back();
                    }
                    return sub;
                } else {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    std::string ptr_char = "*";
                    if( !use_ptr_for_derived_type && !force_pointer_backed_struct ) {
                        ptr_char.clear();
                    }
                    bool decl_use_ref = use_ref;
                    if (ptr_char == "*"
                            && !ASRUtils::is_pointer(v.m_type)
                            && !ASRUtils::is_allocatable(v.m_type)) {
                        decl_use_ref = false;
                    }
                    std::string type_name = "struct " + der_type_name + ptr_char;
                    bool owner_is_bindc = false;
                    if (owner_sym && ASR::is_a<ASR::Function_t>(*owner_sym)) {
                        ASR::FunctionType_t *owner_ftype = ASRUtils::get_FunctionType(
                            ASR::down_cast<ASR::Function_t>(owner_sym));
                        owner_is_bindc = owner_ftype->m_abi == ASR::abiType::BindC;
                    }
                    if (dummy && ptr_char == "*" && !owner_is_bindc
                            && !ASRUtils::is_pointer(v.m_type)
                            && !ASRUtils::is_allocatable(v.m_type)) {
                        type_name += " restrict";
                    }
                    sub = format_type_c(dims, type_name, decl_name, decl_use_ref, dummy);
                }
            } else if (ASR::is_a<ASR::UnionType_t>(*v_m_type)) {
                std::string indent(indentation_level*indentation_spaces, ' ');
                std::string der_type_name = CUtils::get_c_symbol_name(
                    ASRUtils::symbol_get_past_external(v.m_type_declaration));
                if( is_array ) {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name = CUtils::sanitize_c_identifier("x" + der_type_name);
                    std::string type_name = std::string("union ") + der_type_name;
                    if (is_struct_type_member || declaration_only) {
                        std::string array_type =
                            c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                        sub = format_type_c("", array_type, c_v_name, use_ref, dummy);
                    } else {
                        generate_array_decl(sub, c_v_name, type_name, dims,
                                            encoded_type_name, m_dims, n_dims,
                                            use_ref, dummy,
                                            v.m_intent != ASRUtils::intent_in &&
                                            v.m_intent != ASRUtils::intent_inout &&
                                            v.m_intent != ASRUtils::intent_out &&
                                            v.m_intent != ASRUtils::intent_unspecified, is_fixed_size,
                                            false,
                                            ASR::abiType::Source, false, &v);
                    }
                } else if (declaration_only) {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, "union " + der_type_name,
                                        c_v_name, false, dummy);
                } else {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    if( v.m_intent == ASRUtils::intent_in ||
                        v.m_intent == ASRUtils::intent_inout ) {
                        use_ref = false;
                        dims = "";
                    }
                    sub = format_type_c(dims, "union " + der_type_name,
                                        c_v_name, use_ref, dummy);
                }
            } else if (ASR::is_a<ASR::List_t>(*v_m_type)) {
                ASR::List_t* t = ASR::down_cast<ASR::List_t>(v_m_type);
                std::string list_type_c = c_ds_api->get_list_type(t);
                std::string name = c_v_name;
                if (v.m_intent == ASRUtils::intent_out) {
                    name = "*" + name;
                }
                sub = format_type_c("", list_type_c, name,
                                    false, false);
            } else if (ASR::is_a<ASR::Tuple_t>(*v_m_type)) {
                ASR::Tuple_t* t = ASR::down_cast<ASR::Tuple_t>(v_m_type);
                std::string tuple_type_c = c_ds_api->get_tuple_type(t);
                std::string name = c_v_name;
                if (v.m_intent == ASRUtils::intent_out) {
                    name = "*" + name;
                }
                sub = format_type_c("", tuple_type_c, name,
                                    false, false);
            } else if (ASR::is_a<ASR::Dict_t>(*v_m_type)) {
                ASR::Dict_t* t = ASR::down_cast<ASR::Dict_t>(v_m_type);
                std::string dict_type_c = c_ds_api->get_dict_type(t);
                std::string name = c_v_name;
                if (v.m_intent == ASRUtils::intent_out) {
                    name = "*" + name;
                }
                sub = format_type_c("", dict_type_c, name,
                                    false, false);
            } else if (ASR::is_a<ASR::CPtr_t>(*v_m_type)) {
                ASR::expr_t* init_expr = var_init_value;
                if (init_expr && ASR::is_a<ASR::StructConstant_t>(*init_expr)) {
                    ASR::StructConstant_t *sc = ASR::down_cast<ASR::StructConstant_t>(init_expr);
                    std::string der_type_name = CUtils::get_c_symbol_name(
                        ASRUtils::symbol_get_past_external(sc->m_dt_sym));
                    sub = format_type_c("", "struct " + der_type_name, c_v_name, use_ref, dummy);
                } else {
                    sub = format_type_c("", "void*", c_v_name, use_ref, dummy);
                }
            } else if (ASR::is_a<ASR::EnumType_t>(*v_m_type)) {
                ASR::EnumType_t* enum_ = ASR::down_cast<ASR::EnumType_t>(v_m_type);
                ASR::Enum_t* enum_type = ASR::down_cast<ASR::Enum_t>(enum_->m_enum_type);
                sub = format_type_c("", "enum " + get_enum_c_name(*enum_type), c_v_name, false, false);
            } else if (ASR::is_a<ASR::TypeParameter_t>(*v_m_type)) {
                // Ignore type variables
                return "";
            } else {
                diag.codegen_error_label("Type '"
                    + ASRUtils::type_to_str_python_symbol(v_m_type, v.m_type_declaration)
                    + "' not supported", {v.base.base.loc}, "");
                throw Abort();
            }
            if (dims.size() == 0 && v.m_storage == ASR::storage_typeType::Save && use_static) {
                sub = "static " + sub;
            }
            bool emitted_null_init = false;
            if (dims.size() == 0
                    && needs_null_init_for_local_pointer_like_variable(
                        v, v_storage_type, is_array, dummy,
                        is_struct_type_member, do_not_initialize)) {
                sub += " = NULL";
                emitted_null_init = true;
            }
            if (dims.size() == 0 && var_init_raw && !do_not_initialize
                    && !(emitted_null_init && var_init_value
                        && ASR::is_a<ASR::PointerNullConstant_t>(*var_init_value))) {
                ASR::expr_t* init_expr = var_init_value;
                if( v.m_storage != ASR::storage_typeType::Parameter ) {
                    SymbolTable *dep_scope = v.m_parent_symtab ? v.m_parent_symtab : current_scope;
                    for( size_t i = 0; i < v.n_dependencies; i++ ) {
                        std::string variable_name = v.m_dependencies[i];
                        if (dep_scope == nullptr || variable_name.empty()) {
                            continue;
                        }
                        ASR::symbol_t* dep_sym = dep_scope->resolve_symbol(variable_name);
                        if( (dep_sym && ASR::is_a<ASR::Variable_t>(*dep_sym) &&
                            !ASR::down_cast<ASR::Variable_t>(dep_sym)->m_symbolic_value) )  {
                            init_expr = nullptr;
                            break;
                        }
                    }
                }
                if( init_expr ) {
                    if (is_c && ASR::is_a<ASR::StringChr_t>(*init_expr)) {
                        // TODO: Not supported yet
                    } else {
                        this->visit_expr(*init_expr);
                        std::string init = src;
                        sub += " = " + init;
                    }
                }
            }
        }
        return sub;
    }

    void visit_ExternalSymbol(const ASR::ExternalSymbol_t &x) {
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(x.m_external);
        if (sym == nullptr) {
            src.clear();
            return;
        }
        this->visit_symbol(*sym);
    }

    void visit_CustomOperator(const ASR::CustomOperator_t &x) {
        src.clear();
    }


    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        is_string_concat_present = false;
        global_scope = x.m_symtab;
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LCOMPILERS_ASSERT(x.n_items == 0);
        std::string unit_src = "";
        indentation_level = 0;
        indentation_spaces = 4;
        c_ds_api->set_indentation(indentation_level, indentation_spaces);
        c_ds_api->set_global_scope(global_scope);
        c_utils_functions->set_indentation(indentation_level, indentation_spaces);
        c_utils_functions->set_global_scope(global_scope);
        c_ds_api->set_c_utils_functions(c_utils_functions.get());
        bind_py_utils_functions->set_indentation(indentation_level, indentation_spaces);
        bind_py_utils_functions->set_global_scope(global_scope);
        std::string head =
R"(
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <lfortran_intrinsics.h>

)";
        if(compiler_options.target_offload_enabled) {
            head += R"(
#ifdef USE_GPU
#include<cuda_runtime.h>
#else
#include"cuda_cpu_runtime.h"
#endif
)";
        }

        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string tab(indentation_spaces, ' ');

        std::string unit_src_tmp;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                unit_src_tmp = convert_variable_decl(*v);
                unit_src += unit_src_tmp;
                if(unit_src_tmp.size() > 0) {
                    unit_src += ";\n";
                }
            }
        }


        std::map<std::string, std::vector<std::string>> struct_dep_graph;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Struct_t>(*item.second) ||
                ASR::is_a<ASR::Enum_t>(*item.second) ||
                ASR::is_a<ASR::Union_t>(*item.second)) {
                std::vector<std::string> struct_deps_vec;
                std::pair<char**, size_t> struct_deps_ptr = ASRUtils::symbol_dependencies(item.second);
                for( size_t i = 0; i < struct_deps_ptr.second; i++ ) {
                    struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                }
                struct_dep_graph[item.first] = struct_deps_vec;
            }
        }

        std::vector<std::string> struct_deps = ASRUtils::order_deps(struct_dep_graph);

        for (auto &item : struct_deps) {
            ASR::symbol_t* struct_sym = x.m_symtab->get_symbol(item);
            visit_symbol(*struct_sym);
            array_types_decls += src;
        }

        // Topologically sort all global functions
        // and then define them in the right order
        std::vector<ASR::Function_t*> global_functions =
            get_complete_function_definitions(x.m_symtab);

        unit_src += "\n";
        unit_src += "// Implementations\n";

        {
            // Process intrinsic modules in the right order
            std::vector<std::string> build_order
                = ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_symtab->get_scope().find(item)
                    != x.m_symtab->get_scope().end());
                if (startswith(item, "lfortran_intrinsic")) {
                    ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                    if( ASRUtils::get_body_size(mod) != 0 ) {
                        visit_symbol(*mod);
                        unit_src += src;
                    }
                }
            }
        }

        // Process global functions
        for (ASR::Function_t *function : global_functions) {
            visit_Function(*function);
            unit_src += src;
        }

        // Process modules in the right order
        std::vector<std::string> build_order
            = ASRUtils::determine_module_dependencies(x);
        for (auto &item : build_order) {
            LCOMPILERS_ASSERT(x.m_symtab->get_scope().find(item)
                != x.m_symtab->get_scope().end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                visit_symbol(*mod);
                unit_src += src;
            }
        }

        // Then the main program:
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                visit_symbol(*item.second);
                unit_src += src;
            }
        }

        forward_decl_functions = dedupe_decl_lines(forward_decl_functions);
        forward_decl_functions += "\n\n";
        src = get_final_combined_src(head, unit_src);

        if (!emit_headers.empty()) {
            std::string to_includes_1 = "";
            for (auto &s: headers) {
                to_includes_1 += "#include <" + s + ">\n";
            }
            for (auto &f_name: emit_headers) {
                std::ofstream out_file;
                std::string out_src = to_includes_1 + head + f_name.second;
                std::string ifdefs = f_name.first.substr(0, f_name.first.length() - 2);
                std::transform(ifdefs.begin(), ifdefs.end(), ifdefs.begin(), ::toupper);
                ifdefs += "_H";
                out_src = "#ifndef " + ifdefs + "\n#define " + ifdefs + "\n\n" + out_src;
                out_src += "\n\n#endif\n";
                out_file.open(f_name.first);
                out_file << out_src;
                out_file.close();
            }
        }
    }

    CTranslationUnitSplitResult emit_split_translation_unit(
            const ASR::TranslationUnit_t &x, const std::string &output_dir,
            const std::string &project_name) {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(output_dir, ec);
        if (ec) {
            throw std::runtime_error(
                "Unable to create split C output directory `" + output_dir
                + "`: " + ec.message());
        }

        struct UniqueIdGuard {
            std::string saved;
            UniqueIdGuard(const std::string &replacement)
                : saved(::lcompilers_unique_ID_separate_compilation) {
                ::lcompilers_unique_ID_separate_compilation = replacement;
            }
            ~UniqueIdGuard() {
                ::lcompilers_unique_ID_separate_compilation = saved;
            }
        };
        std::ostringstream split_suffix_builder;
        split_suffix_builder << std::hex
            << (get_stable_string_hash(output_dir) & 0xffffffffffffffffULL);
        UniqueIdGuard unique_id_guard(split_suffix_builder.str());

        is_string_concat_present = false;
        emit_compact_constant_data_units = true;
        compact_constant_data_count = 0;
        compact_constant_data_body.clear();
        compact_constant_data_decls.clear();
        global_scope = x.m_symtab;
        LCOMPILERS_ASSERT(x.n_items == 0);
        indentation_level = 0;
        indentation_spaces = 4;
        c_ds_api->set_indentation(indentation_level, indentation_spaces);
        c_ds_api->set_global_scope(global_scope);
        c_utils_functions->set_indentation(indentation_level, indentation_spaces);
        c_utils_functions->set_global_scope(global_scope);
        c_ds_api->set_c_utils_functions(c_utils_functions.get());
        bind_py_utils_functions->set_indentation(indentation_level, indentation_spaces);
        bind_py_utils_functions->set_global_scope(global_scope);
        std::string safe_project_name =
            CUtils::sanitize_c_identifier(project_name.empty() ? "lfortran_c_project" : project_name);

        std::string global_var_defs;
        std::string global_var_decls;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(item.second);
                global_var_defs += get_global_definition(*v);
                global_var_decls += get_global_extern_decl(*v);
            }
        }

        std::map<std::string, std::vector<std::string>> struct_dep_graph;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Struct_t>(*item.second) ||
                ASR::is_a<ASR::Enum_t>(*item.second) ||
                ASR::is_a<ASR::Union_t>(*item.second)) {
                std::vector<std::string> struct_deps_vec;
                std::pair<char**, size_t> struct_deps_ptr = ASRUtils::symbol_dependencies(item.second);
                for( size_t i = 0; i < struct_deps_ptr.second; i++ ) {
                    struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                }
                struct_dep_graph[item.first] = struct_deps_vec;
            }
        }

        std::vector<std::string> struct_deps = ASRUtils::order_deps(struct_dep_graph);
        for (auto &item : struct_deps) {
            ASR::symbol_t* struct_sym = x.m_symtab->get_symbol(item);
            visit_symbol(*struct_sym);
            array_types_decls += src;
        }

        std::vector<std::pair<std::string, std::string>> unit_bodies;
        std::vector<ASR::Function_t*> global_functions =
            get_complete_function_definitions(x.m_symtab);

        std::vector<std::string> build_order =
            ASRUtils::determine_module_dependencies(x);

        for (auto &item : build_order) {
            LCOMPILERS_ASSERT(x.m_symtab->get_scope().find(item)
                != x.m_symtab->get_scope().end());
            if (startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                if( ASRUtils::get_body_size(mod) != 0 ) {
                    ASR::Module_t *module_t = ASR::down_cast<ASR::Module_t>(mod);
                    auto module_units = emit_module_split_units(*module_t);
                    for (auto &unit : module_units) {
                        if (!unit.second.empty()) {
                            unit_bodies.push_back(std::move(unit));
                        }
                    }
                }
            }
        }

        std::string global_functions_body;
        std::set<uint64_t> emitted_global_functions;
        for (ASR::Function_t *function : global_functions) {
            if (is_split_global_helper_function(function)) {
                continue;
            }
            emitted_global_functions.insert(
                get_hash(reinterpret_cast<ASR::asr_t*>(function)));
            visit_Function(*function);
            if (!src.empty()) {
                global_functions_body += src;
            }
        }

        for (auto &item : build_order) {
            LCOMPILERS_ASSERT(x.m_symtab->get_scope().find(item)
                != x.m_symtab->get_scope().end());
            if (!startswith(item, "lfortran_intrinsic")) {
                ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                if (ASR::is_a<ASR::Module_t>(*mod)
                        && ASR::down_cast<ASR::Module_t>(mod)->m_loaded_from_mod
                        && !ASR::down_cast<ASR::Module_t>(mod)->m_intrinsic) {
                    continue;
                }
                ASR::Module_t *module_t = ASR::down_cast<ASR::Module_t>(mod);
                auto module_units = emit_module_split_units(*module_t);
                for (auto &unit : module_units) {
                    if (!unit.second.empty()) {
                        unit_bodies.push_back(std::move(unit));
                    }
                }
            }
        }

        while (true) {
            std::vector<ASR::Function_t*> pending =
                consume_pending_function_definitions(
                    x.m_symtab, emitted_global_functions, true);
            if (pending.empty()) {
                break;
            }
            for (ASR::Function_t *function : pending) {
                visit_Function(*function);
                if (!src.empty()) {
                    global_functions_body += src;
                }
            }
        }

        bool has_main_program = false;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Program_t>(*item.second)) {
                ASR::Program_t *program_t = ASR::down_cast<ASR::Program_t>(item.second);
                auto program_units = emit_program_split_units(*program_t);
                for (auto &unit : program_units) {
                    if (!unit.second.empty()) {
                        unit_bodies.push_back(std::move(unit));
                        has_main_program = true;
                    }
                }
            }
        }
        while (true) {
            std::vector<ASR::Function_t*> pending =
                consume_pending_function_definitions(
                    x.m_symtab, emitted_global_functions, true);
            if (pending.empty()) {
                break;
            }
            for (ASR::Function_t *function : pending) {
                visit_Function(*function);
                if (!src.empty()) {
                    global_functions_body += src;
                }
            }
        }
        if (!global_functions_body.empty()) {
            unit_bodies.push_back({"global_procedures.c", global_functions_body});
        }

        std::vector<std::pair<std::string, std::string>> unique_unit_bodies;
        std::map<std::string, size_t> unit_index_by_name;
        for (auto &unit : unit_bodies) {
            auto it = unit_index_by_name.find(unit.first);
            if (it == unit_index_by_name.end()) {
                unit_index_by_name[unit.first] = unique_unit_bodies.size();
                unique_unit_bodies.push_back(std::move(unit));
                continue;
            }
            std::pair<std::string, std::string> &existing =
                unique_unit_bodies[it->second];
            if (existing.second != unit.second) {
                throw CodeGenError(
                    "Split C emission produced conflicting unit bodies for `"
                    + unit.first + "`."
                );
            }
        }
        unit_bodies = std::move(unique_unit_bodies);
        unit_bodies = pack_split_units_by_budget(unit_bodies, safe_project_name);

        forward_decl_functions += "\n\n";
        // Unit visitors mutate indentation and scope state. Reset before
        // synthesizing shared declarations/prototypes at file scope.
        indentation_level = 0;
        bracket_open = 0;
        current_scope = global_scope;
        std::string helper_defs;
        finalize_common_sections(helper_defs);
        std::string module_var_decls = collect_module_extern_decls(x);
        std::string split_func_decls = collect_split_function_decls(x);
        std::string header_name = safe_project_name + "_generated.h";
        std::string header_guard = safe_project_name;
        std::transform(header_guard.begin(), header_guard.end(), header_guard.begin(), ::toupper);
        header_guard += "_GENERATED_H";

        std::string module_aggregate_decls = collect_module_aggregate_decls(x);
        std::string header_array_type_decls = array_types_decls;
        const std::string dimension_descriptor_decl =
            "\nstruct dimension_descriptor\n"
            "{\n    int32_t lower_bound, length, stride;\n};\n";
        if (!header_array_type_decls.empty()
                && header_array_type_decls.find(dimension_descriptor_decl) == std::string::npos) {
            header_array_type_decls = dimension_descriptor_decl + header_array_type_decls;
        }
        std::string split_owned_defs;
        hoist_split_header_enum_name_defs(header_array_type_decls, split_owned_defs);
        std::string function_decls = dedupe_decl_lines(
            forward_decl_functions + split_func_decls + compact_constant_data_decls);
        if (is_string_concat_present) {
            function_decls += "static inline char* strcat_(const char* x, const char* y) {\n";
            function_decls += "    char* str_tmp = (char*) malloc((strlen(x) + strlen(y) + 2) * sizeof(char));\n";
            function_decls += "    strcpy(str_tmp, x);\n";
            function_decls += "    return strcat(str_tmp, y);\n";
            function_decls += "}\n\n";
        }

        std::string header_src = "#ifndef " + header_guard + "\n#define "
            + header_guard + "\n\n"
            + get_include_block() + get_default_head() + "\n"
            + header_array_type_decls + module_aggregate_decls + function_decls + global_var_decls
            + module_var_decls
            + "\n#endif\n";
        write_text_file(fs::path(output_dir) / header_name, header_src);

        std::vector<std::string> source_files;
        std::string shared_body;
        if (!split_owned_defs.empty()) {
            shared_body += split_owned_defs + "\n";
        }
        if (!global_var_defs.empty()) {
            shared_body += global_var_defs + "\n";
        }
        if (!helper_defs.empty()) {
            shared_body += helper_defs;
        }
        if (!shared_body.empty()) {
            std::string shared_name = safe_project_name + "_shared.c";
            write_text_file(fs::path(output_dir) / shared_name,
                get_unit_file_prelude(header_name) + shared_body);
            source_files.push_back(shared_name);
        }

        if (!compact_constant_data_body.empty()) {
            std::string data_name = safe_project_name + "_constants_data.c";
            write_text_file(fs::path(output_dir) / data_name,
                get_unit_file_prelude(header_name) + compact_constant_data_body);
            source_files.push_back(data_name);
        }

        for (const auto &unit : unit_bodies) {
            write_text_file(fs::path(output_dir) / unit.first,
                get_unit_file_prelude(header_name) + unit.second);
            source_files.push_back(unit.first);
        }

        if (!emit_headers.empty()) {
            std::string to_includes_1 = "";
            for (auto &s: headers) {
                to_includes_1 += "#include <" + s + ">\n";
            }
            for (auto &f_name: emit_headers) {
                std::string out_src = to_includes_1 + get_default_head() + f_name.second;
                std::string ifdefs = f_name.first.substr(0, f_name.first.length() - 2);
                std::transform(ifdefs.begin(), ifdefs.end(), ifdefs.begin(), ::toupper);
                ifdefs += "_H";
                out_src = "#ifndef " + ifdefs + "\n#define " + ifdefs + "\n\n"
                    + out_src + "\n\n#endif\n";
                write_text_file(fs::path(output_dir) / f_name.first, out_src);
            }
        }

        return {source_files, header_name, has_main_program};
    }

    void visit_Module(const ASR::Module_t &x) {
        if (x.m_intrinsic) {
            intrinsic_module = true;
        } else {
            intrinsic_module = false;
        }

        if(to_lower(x.m_name) == "omp_lib") {
            headers.insert("omp.h");
            return;
        } else if (to_lower(x.m_name) == "iso_c_binding" || to_lower(x.m_name) == "lfortran_intrinsic_iso_c_binding") {
            return;
        }

        std::string unit_src = "";
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Variable_t>(*item.second)) {
                std::string unit_src_tmp;
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(
                    item.second);
                unit_src_tmp = convert_variable_decl(*v);
                unit_src += unit_src_tmp;
                if(unit_src_tmp.size() > 0) {
                    unit_src += ";\n";
                }
            }
        }
        std::map<std::string, std::vector<std::string>> struct_dep_graph;
        for (auto &item : x.m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Struct_t>(*item.second) ||
                    ASR::is_a<ASR::Enum_t>(*item.second) ||
                    ASR::is_a<ASR::Union_t>(*item.second)) {
                std::vector<std::string> struct_deps_vec;
                std::pair<char**, size_t> struct_deps_ptr = ASRUtils::symbol_dependencies(item.second);
                for( size_t i = 0; i < struct_deps_ptr.second; i++ ) {
                    struct_deps_vec.push_back(std::string(struct_deps_ptr.first[i]));
                }
                struct_dep_graph[item.first] = struct_deps_vec;
            }
        }

        std::vector<std::string> struct_deps = ASRUtils::order_deps(struct_dep_graph);
        for (auto &item : struct_deps) {
            ASR::symbol_t* struct_sym = x.m_symtab->get_symbol(item);
            if (struct_sym == nullptr) {
                continue;
            }
            if (!(ASR::is_a<ASR::Struct_t>(*struct_sym) ||
                  ASR::is_a<ASR::Enum_t>(*struct_sym) ||
                  ASR::is_a<ASR::Union_t>(*struct_sym))) {
                continue;
            }
            visit_symbol(*struct_sym);
        }

        // Topologically sort all module functions
        // and then define them in the right order
        std::vector<ASR::Function_t*> functions =
            get_complete_function_definitions(x.m_symtab);
        for (ASR::Function_t *s : functions) {
            visit_Function(*s);
            unit_src += src;
        }
        src = unit_src;
        intrinsic_module = false;
    }

    void visit_Program(const ASR::Program_t &x) {
        SymbolTable *current_scope_copy = current_scope;
        current_scope = x.m_symtab;
        // Topologically sort all program functions
        // and then define them in the right order
        std::vector<ASR::Function_t*> functions =
            get_complete_function_definitions(x.m_symtab);

        // Generate code for nested subroutines and functions first:
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
            visit_symbol(*struct_sym);
        }
        for (ASR::Function_t *function : functions) {
            visit_Function(*function);
            contains += src;
        }

        // Generate code for the main program
        indentation_level += 1;
        std::string indent1(indentation_level*indentation_spaces, ' ');
        std::string decl;
        // Topologically sort all program functions
        // and then define them in the right order
        std::vector<std::string> var_order = ASRUtils::determine_variable_declaration_order(x.m_symtab);
        std::string decl_tmp;
        for (auto &item : var_order) {
            ASR::symbol_t* var_sym = x.m_symtab->get_symbol(item);
            if (ASR::is_a<ASR::Variable_t>(*var_sym)) {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var_sym);
                decl += indent1;
                decl_tmp = convert_variable_decl(*v);
                decl += decl_tmp;
                if(decl_tmp.size() > 0) {
                    decl += ";\n";
                }
            }
        }

        std::string body;
        if (compiler_options.enable_cpython) {
            headers.insert("Python.h");
            body += R"(
    Py_Initialize();
    wchar_t* argv1 = Py_DecodeLocale("", NULL);
    wchar_t** argv_ = {&argv1};
    PySys_SetArgv(1, argv_);
)";
            body += "\n";
        }

        if (compiler_options.link_numpy) {
            user_defines.insert("NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION");
            headers.insert("numpy/arrayobject.h");
            body +=
R"(    // Initialise Numpy
    if (_import_array() < 0) {
        PyErr_Print();
        PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
        fprintf(stderr, "Failed to import numpy Python module(s)\n");
        return -1;
    }
)";
            body += "\n";
        }

        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            body += src;
        }

        if (compiler_options.enable_cpython) {
            body += R"(
    if (Py_FinalizeEx() < 0) {
        fprintf(stderr,"BindPython: Unknown Error\n");
        exit(1);
    }
)";
            body += "\n";
        }

        if(compiler_options.target_offload_enabled) {
            contains += kernel_func_code;
        }

        if (target_offload_enabled && !kernel_func_names.empty()) {
            std::string dispatch_code = "#ifndef USE_GPU\n";
            dispatch_code += "void compute_kernel_wrapper(void **args, void *func) {\n";
            for (const auto &kname : kernel_func_names) {
                dispatch_code += "    if (func == (void*)" + kname + ") {\n";
                dispatch_code += "        " + kname + "_wrapper(args);\n";
                dispatch_code += "        return;\n";
                dispatch_code += "    }\n";
            }
            dispatch_code += "    fprintf(stderr, \"Unknown kernel function\\n\");\n";
            dispatch_code += "    exit(1);\n";
            dispatch_code += "}\n#endif\n";
            contains += dispatch_code;
        }

        src = contains
                + "int main(int argc, char* argv[])\n{\n"
                + indent1 + "_lpython_set_argv(argc, argv);\n"
                + decl + body
                + indent1 + "return 0;\n}\n";
        indentation_level -= 2;
        current_scope = current_scope_copy;
    }

    template <typename T>
    void visit_AggregateTypeUtil(const T& x, std::string c_type_name,
        std::string& src_dest) {
        std::string body = "";
        int indendation_level_copy = indentation_level;
        for( auto itr: x.m_symtab->get_scope() ) {
            if( ASR::is_a<ASR::Union_t>(*itr.second) ) {
                ASR::Union_t *union_t = ASR::down_cast<ASR::Union_t>(itr.second);
                if (should_emit_union_definition(*union_t)) {
                    visit_AggregateTypeUtil(*union_t, "union", src_dest);
                }
            } else if( ASR::is_a<ASR::Struct_t>(*itr.second) ) {
                ASR::Struct_t *struct_t = ASR::down_cast<ASR::Struct_t>(itr.second);
                if (should_emit_struct_definition(*struct_t)) {
                    std::string struct_c_type_name = get_StructTypeCTypeName(*struct_t);
                    visit_AggregateTypeUtil(*struct_t, struct_c_type_name, src_dest);
                }
            }
        }
        indentation_level = indendation_level_copy;
        std::string indent(indentation_level*indentation_spaces, ' ');
        indentation_level += 1;
        std::string open_struct = indent + c_type_name + " "
            + get_aggregate_c_name(x) + " {\n";
        if (src_dest.find(open_struct) != std::string::npos) {
            indentation_level -= 1;
            src = "";
            return;
        }
        indent.push_back(' ');
        if constexpr (std::is_same_v<std::decay_t<T>, ASR::Struct_t>) {
            body += indent + "int64_t " + get_runtime_type_tag_member_name() + ";\n";
        }
        CDeclarationOptions c_decl_options_;
        c_decl_options_.pre_initialise_derived_type = false;
        c_decl_options_.use_ptr_for_derived_type = false;
        c_decl_options_.use_static = false;
        c_decl_options_.do_not_initialize = true;
        auto append_struct_members = [&](auto&& self_append, const ASR::Struct_t& struct_t) -> void {
            if (struct_t.m_parent) {
                ASR::symbol_t *parent_sym = ASRUtils::symbol_get_past_external(struct_t.m_parent);
                if (ASR::is_a<ASR::Struct_t>(*parent_sym)) {
                    self_append(self_append, *ASR::down_cast<ASR::Struct_t>(parent_sym));
                }
            }
            for (size_t i = 0; i < struct_t.n_members; i++) {
                ASR::symbol_t* member = struct_t.m_symtab->get_symbol(struct_t.m_members[i]);
                LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member));
                body += indent + convert_variable_decl(
                            *ASR::down_cast<ASR::Variable_t>(member),
                            &c_decl_options_);
                body += ";\n";
            }
        };
        if constexpr (std::is_same_v<std::decay_t<T>, ASR::Struct_t>) {
            append_struct_members(append_struct_members, x);
        } else {
            for( size_t i = 0; i < x.n_members; i++ ) {
                ASR::symbol_t* member = x.m_symtab->get_symbol(x.m_members[i]);
                LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member));
                body += indent + convert_variable_decl(
                            *ASR::down_cast<ASR::Variable_t>(member),
                            &c_decl_options_);
                body += ";\n";
            }
        }
        indentation_level -= 1;
        std::string end_struct = "};\n\n";
        src_dest += open_struct + body + end_struct;
        if constexpr (std::is_same_v<std::decay_t<T>, ASR::Struct_t>) {
            src_dest += emit_c_tbp_parent_registration(x);
        }
    }

    std::string get_StructTypeCTypeName(const ASR::Struct_t& x) {
        std::string c_type_name = "struct";
        if( x.m_is_packed ) {
            std::string attr_args = "(packed";
            if( x.m_alignment ) {
                LCOMPILERS_ASSERT(ASRUtils::expr_value(x.m_alignment));
                ASR::expr_t* alignment_value = ASRUtils::expr_value(x.m_alignment);
                int64_t alignment_int = -1;
                if( !ASRUtils::extract_value(alignment_value, alignment_int) ) {
                    LCOMPILERS_ASSERT(false);
                }
                attr_args += ", aligned(" + std::to_string(alignment_int) + ")";
            }
            attr_args += ")";
            c_type_name += " __attribute__(" + attr_args + ")";
        }
        return c_type_name;
    }

    void visit_Struct(const ASR::Struct_t& x) {
        src = "";
        if (!should_emit_struct_definition(x)) {
            return;
        }
        std::string c_type_name = get_StructTypeCTypeName(x);
        visit_AggregateTypeUtil(x, c_type_name, array_types_decls);
        src = "";
    }

    void visit_Union(const ASR::Union_t& x) {
        if (!should_emit_union_definition(x)) {
            src = "";
            return;
        }
        visit_AggregateTypeUtil(x, "union", array_types_decls);
    }

    void visit_Enum(const ASR::Enum_t& x) {
        if( x.m_enum_value_type == ASR::enumtypeType::NonInteger ) {
            throw CodeGenError("C backend only supports integer valued EnumType. " +
                std::string(x.m_name) + " is not integer valued.");
        }
        if( x.m_enum_value_type == ASR::enumtypeType::IntegerNotUnique ) {
            throw CodeGenError("C backend only supports uniquely valued integer EnumType. " +
                std::string(x.m_name) + " EnumType is having duplicate values for its members.");
        }
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string tab(indentation_spaces, ' ');
        std::string meta_data = " = {";
        std::string enum_name = get_enum_c_name(x);
        if (!emitted_aggregate_names.insert("enum " + enum_name).second) {
            src = "";
            return;
        }
        std::string open_struct = indent + "enum " + enum_name + " {\n";
        std::string body = "";
        int64_t min_value = INT64_MAX;
        int64_t max_value = INT64_MIN;
        size_t max_name_len = 0;
        for( size_t i = 0; i < x.n_members; i++ ) {
            ASR::symbol_t* member = x.m_symtab->get_symbol(x.m_members[i]);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member));
            ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member);
            ASR::expr_t* value = ASRUtils::expr_value(member_var->m_symbolic_value);
            int64_t value_int64 = -1;
            ASRUtils::extract_value(value, value_int64);
            min_value = std::min(value_int64, min_value);
            max_value = std::max(value_int64, max_value);
            max_name_len = std::max(max_name_len, std::string(x.m_members[i]).size());
            this->visit_expr(*member_var->m_symbolic_value);
            body += indent + tab + get_enum_member_c_name(*member_var) + " = " + src + ",\n";
        }
        size_t max_names = max_value - min_value + 1;
        std::vector<std::string> enum_names(max_names, "\"\"");
        for( size_t i = 0; i < x.n_members; i++ ) {
            ASR::symbol_t* member = x.m_symtab->get_symbol(x.m_members[i]);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member));
            ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member);
            ASR::expr_t* value = ASRUtils::expr_value(member_var->m_symbolic_value);
            int64_t value_int64 = -1;
            ASRUtils::extract_value(value, value_int64);
            min_value = std::min(value_int64, min_value);
            enum_names[value_int64 - min_value] = "\"" + std::string(member_var->m_name) + "\"";
        }
        for( auto enum_name: enum_names ) {
            meta_data += enum_name + ", ";
        }
        meta_data.pop_back();
        meta_data.pop_back();
        meta_data += "};\n";
        std::string end_struct = "};\n\n";
        std::string enum_names_type = "char " + global_scope->get_unique_name(
            "enum_names_" + enum_name) +
         + "[" + std::to_string(max_names) + "][" + std::to_string(max_name_len + 1) + "] ";
        array_types_decls += enum_names_type + meta_data + open_struct + body + end_struct;
        src = "";
    }

    void visit_EnumConstructor(const ASR::EnumConstructor_t& x) {
        LCOMPILERS_ASSERT(x.n_args == 1);
        ASR::expr_t* m_arg = x.m_args[0];
        this->visit_expr(*m_arg);
        ASR::Enum_t* enum_type = ASR::down_cast<ASR::Enum_t>(x.m_dt_sym);
        src = "(enum " + get_enum_c_name(*enum_type) + ") (" + src + ")";
    }

    void visit_UnionConstructor(const ASR::UnionConstructor_t& /*x*/) {

    }

    void visit_EnumStaticMember(const ASR::EnumStaticMember_t& x) {
        CHECK_FAST_C(compiler_options, x)
        ASR::Variable_t* enum_var = ASR::down_cast<ASR::Variable_t>(x.m_m);
        src = get_enum_member_c_name(*enum_var);
    }

    void visit_EnumValue(const ASR::EnumValue_t& x) {
        CHECK_FAST_C(compiler_options, x)
        visit_expr(*x.m_v);
    }

    void visit_EnumName(const ASR::EnumName_t& x) {
        CHECK_FAST_C(compiler_options, x)
        int64_t min_value = INT64_MAX;
        ASR::EnumType_t* enum_t = ASR::down_cast<ASR::EnumType_t>(x.m_enum_type);
        ASR::Enum_t* enum_type = ASR::down_cast<ASR::Enum_t>(enum_t->m_enum_type);
        for( auto itr: enum_type->m_symtab->get_scope() ) {
            ASR::Variable_t* itr_var = ASR::down_cast<ASR::Variable_t>(itr.second);
            ASR::expr_t* value = ASRUtils::expr_value(itr_var->m_symbolic_value);
            int64_t value_int64 = -1;
            ASRUtils::extract_value(value, value_int64);
            min_value = std::min(value_int64, min_value);
        }
        visit_expr(*x.m_v);
        std::string enum_var_name = src;
        src = global_scope->get_unique_name("enum_names_" + std::string(enum_type->m_name)) +
                "[" + std::string(enum_var_name) + " - " + std::to_string(min_value) + "]";
    }

    void visit_ComplexConstant(const ASR::ComplexConstant_t &x) {
        headers.insert("complex.h");
        std::string re = std::to_string(x.m_re);
        std::string im = std::to_string(x.m_im);
        src = "CMPLX(" + re + ", " + im + ")";

        last_expr_precedence = 2;
    }

    void visit_LogicalConstant(const ASR::LogicalConstant_t &x) {
        if (x.m_value == true) {
            src = "true";
        } else {
            src = "false";
        }
        last_expr_precedence = 2;
    }

    void visit_Assert(const ASR::Assert_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string out = indent;
        bracket_open++;
        visit_expr(*x.m_test);
        std::string test_condition = src;
        if (x.m_msg) {
            this->visit_expr(*x.m_msg);
            std::string tmp_gen = "";
            ASR::ttype_t* value_type = ASRUtils::expr_type(x.m_msg);
            if( ASR::is_a<ASR::List_t>(*value_type) ||
                ASR::is_a<ASR::Tuple_t>(*value_type)) {
                std::string p_func = c_ds_api->get_print_func(value_type);
                tmp_gen += indent + p_func + "(" + src + ");\n";
            } else {
                tmp_gen += "\"";
                tmp_gen += c_ds_api->get_print_type(value_type, ASR::is_a<ASR::ArrayItem_t>(*x.m_msg));
                tmp_gen += "\", ";
                if( ASRUtils::is_array(value_type) ) {
                    src += "->data";
                }
                if (ASR::is_a<ASR::Complex_t>(*value_type)) {
                    tmp_gen += "creal(" + src + ")";
                    tmp_gen += ", ";
                    tmp_gen += "cimag(" + src + ")";
                } else {
                    tmp_gen += src;
                }
            }
            out += "ASSERT_MSG(";
            out += test_condition + ", ";
            out += tmp_gen + ");\n";
        } else {
            out += "ASSERT(";
            out += test_condition + ");\n";
        }
        bracket_open--;
        src = check_tmp_buffer() + out;
    }

    void visit_FileInquire(const ASR::FileInquire_t &x) {
        headers.insert("string.h");
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string setup_code;
        std::string post_code;

        auto visit_scalar_ptr = [&](ASR::expr_t *expr) -> std::string {
            if (!expr) return "NULL";
            this->visit_expr(*expr);
            return "&(" + src + ")";
        };

        auto visit_string_arg = [&](ASR::expr_t *expr) -> std::pair<std::string, std::string> {
            if (!expr) return {"NULL", "0"};
            this->visit_expr(*expr);
            std::string value = src;
            ASR::String_t *str_type = ASRUtils::get_string_type(expr);
            if (str_type && str_type->m_len) {
                this->visit_expr(*str_type->m_len);
                return {value, src};
            }
            return {value, "((" + value + ") != NULL ? strlen(" + value + ") : 0)"};
        };

        auto visit_string_output_arg = [&](ASR::expr_t *expr) -> std::pair<std::string, std::string> {
            if (!expr) return {"NULL", "0"};
            this->visit_expr(*expr);
            std::string value = src;
            ASR::String_t *str_type = ASRUtils::get_string_type(expr);
            std::string value_len = "strlen(" + value + ")";
            bool has_fixed_char_len = false;
            if (str_type && str_type->m_len) {
                this->visit_expr(*str_type->m_len);
                value_len = src;
                has_fixed_char_len = true;
            }
            std::string tmp_name, setup_readback, post_readback;
            if (prepare_string_readback_target(expr, value_len,
                    tmp_name, setup_readback, post_readback)) {
                setup_code += setup_readback;
                post_code += post_readback;
                return {tmp_name, value_len};
            }
            value = get_c_mutable_scalar_expr(expr);
            if (has_fixed_char_len) {
                setup_code += indent + "if (" + value + " == NULL || ((int64_t)strlen(" + value
                    + ")) < ((int64_t)(" + value_len + "))) " + value
                    + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                    + value_len + ");\n";
            } else {
                setup_code += indent + "if (" + value + " == NULL) " + value
                    + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                    + value_len + ");\n";
            }
            return {value, value_len};
        };

        std::pair<std::string, std::string> file_arg = visit_string_arg(x.m_file);
        std::pair<std::string, std::string> write_arg = visit_string_output_arg(x.m_write);
        std::pair<std::string, std::string> read_arg = visit_string_output_arg(x.m_read);
        std::pair<std::string, std::string> readwrite_arg = visit_string_output_arg(x.m_readwrite);
        std::pair<std::string, std::string> access_arg = visit_string_output_arg(x.m_access);
        std::pair<std::string, std::string> name_arg = visit_string_output_arg(x.m_name);
        std::pair<std::string, std::string> blank_arg = visit_string_output_arg(x.m_blank);
        std::pair<std::string, std::string> sequential_arg = visit_string_output_arg(x.m_sequential);
        std::pair<std::string, std::string> direct_arg = visit_string_output_arg(x.m_direct);
        std::pair<std::string, std::string> form_arg = visit_string_output_arg(x.m_form);
        std::pair<std::string, std::string> formatted_arg = visit_string_output_arg(x.m_formatted);
        std::pair<std::string, std::string> unformatted_arg = visit_string_output_arg(x.m_unformatted);
        std::pair<std::string, std::string> decimal_arg = visit_string_output_arg(x.m_decimal);
        std::pair<std::string, std::string> sign_arg = visit_string_output_arg(x.m_sign);
        std::pair<std::string, std::string> encoding_arg = visit_string_output_arg(x.m_encoding);
        std::pair<std::string, std::string> stream_arg = visit_string_output_arg(x.m_stream);
        std::pair<std::string, std::string> iomsg_arg = visit_string_output_arg(x.m_iomsg);
        std::pair<std::string, std::string> round_arg = visit_string_output_arg(x.m_round);
        std::pair<std::string, std::string> pad_arg = visit_string_output_arg(x.m_pad);
        std::pair<std::string, std::string> asynchronous_arg = visit_string_output_arg(x.m_asynchronous);
        std::pair<std::string, std::string> action_arg = visit_string_output_arg(x.m_action);
        std::pair<std::string, std::string> position_arg = visit_string_output_arg(x.m_position);
        std::pair<std::string, std::string> delim_arg = visit_string_output_arg(x.m_delim);

        std::string unit = "-1";
        if (x.m_unit) {
            this->visit_expr(*x.m_unit);
            unit = src;
        }

        src = setup_code + indent + "_lfortran_inquire("
            + file_arg.first + ", " + file_arg.second + ", "
            + visit_scalar_ptr(x.m_exist) + ", " + unit + ", "
            + visit_scalar_ptr(x.m_opened) + ", "
            + visit_scalar_ptr(x.m_size) + ", "
            + visit_scalar_ptr(x.m_pos) + ", "
            + write_arg.first + ", " + write_arg.second + ", "
            + read_arg.first + ", " + read_arg.second + ", "
            + readwrite_arg.first + ", " + readwrite_arg.second + ", "
            + access_arg.first + ", " + access_arg.second + ", "
            + name_arg.first + ", " + name_arg.second + ", "
            + blank_arg.first + ", " + blank_arg.second + ", "
            + visit_scalar_ptr(x.m_recl) + ", "
            + visit_scalar_ptr(x.m_number) + ", "
            + visit_scalar_ptr(x.m_named) + ", "
            + sequential_arg.first + ", " + sequential_arg.second + ", "
            + direct_arg.first + ", " + direct_arg.second + ", "
            + form_arg.first + ", " + form_arg.second + ", "
            + formatted_arg.first + ", " + formatted_arg.second + ", "
            + unformatted_arg.first + ", " + unformatted_arg.second + ", "
            + visit_scalar_ptr(x.m_iostat) + ", "
            + visit_scalar_ptr(x.m_nextrec) + ", "
            + decimal_arg.first + ", " + decimal_arg.second + ", "
            + sign_arg.first + ", " + sign_arg.second + ", "
            + encoding_arg.first + ", " + encoding_arg.second + ", "
            + stream_arg.first + ", " + stream_arg.second + ", "
            + iomsg_arg.first + ", " + iomsg_arg.second + ", "
            + round_arg.first + ", " + round_arg.second + ", "
            + pad_arg.first + ", " + pad_arg.second + ", "
            + visit_scalar_ptr(x.m_pending) + ", "
            + asynchronous_arg.first + ", " + asynchronous_arg.second + ", "
            + action_arg.first + ", " + action_arg.second + ", "
            + position_arg.first + ", " + position_arg.second + ", "
            + delim_arg.first + ", " + delim_arg.second + ");\n"
            + post_code;
    }

    void visit_FileOpen(const ASR::FileOpen_t &x) {
        headers.insert("string.h");
        std::string indent(indentation_level * indentation_spaces, ' ');

        auto visit_scalar_ptr = [&](ASR::expr_t *expr) -> std::string {
            if (!expr) return "NULL";
            this->visit_expr(*expr);
            return "&(" + src + ")";
        };

        auto visit_string_arg = [&](ASR::expr_t *expr) -> std::pair<std::string, std::string> {
            if (!expr) return {"NULL", "0"};
            this->visit_expr(*expr);
            std::string value = src;
            return {value, "strlen(" + value + ")"};
        };

        std::string unit = "-1";
        if (x.m_newunit) {
            this->visit_expr(*x.m_newunit);
            unit = src;
        }

        std::pair<std::string, std::string> file_arg = visit_string_arg(x.m_filename);
        std::pair<std::string, std::string> status_arg = visit_string_arg(x.m_status);
        std::pair<std::string, std::string> form_arg = visit_string_arg(x.m_form);
        std::pair<std::string, std::string> access_arg = visit_string_arg(x.m_access);
        std::pair<std::string, std::string> iomsg_arg = visit_string_arg(x.m_iomsg);
        std::pair<std::string, std::string> action_arg = visit_string_arg(x.m_action);
        std::pair<std::string, std::string> delim_arg = visit_string_arg(x.m_delim);
        std::pair<std::string, std::string> position_arg = visit_string_arg(x.m_position);
        std::pair<std::string, std::string> blank_arg = visit_string_arg(x.m_blank);
        std::pair<std::string, std::string> encoding_arg = visit_string_arg(x.m_encoding);
        std::pair<std::string, std::string> sign_arg = visit_string_arg(x.m_sign);
        std::pair<std::string, std::string> decimal_arg = visit_string_arg(x.m_decimal);
        std::pair<std::string, std::string> round_arg = visit_string_arg(x.m_round);
        std::pair<std::string, std::string> pad_arg = visit_string_arg(x.m_pad);

        src = indent + "_lfortran_open("
            + unit + ", "
            + file_arg.first + ", " + file_arg.second + ", "
            + status_arg.first + ", " + status_arg.second + ", "
            + form_arg.first + ", " + form_arg.second + ", "
            + access_arg.first + ", " + access_arg.second + ", "
            + iomsg_arg.first + ", " + iomsg_arg.second + ", "
            + visit_scalar_ptr(x.m_iostat) + ", "
            + action_arg.first + ", " + action_arg.second + ", "
            + delim_arg.first + ", " + delim_arg.second + ", "
            + position_arg.first + ", " + position_arg.second + ", "
            + blank_arg.first + ", " + blank_arg.second + ", "
            + encoding_arg.first + ", " + encoding_arg.second + ", "
            + visit_scalar_ptr(x.m_recl) + ", "
            + sign_arg.first + ", " + sign_arg.second + ", "
            + decimal_arg.first + ", " + decimal_arg.second + ", "
            + round_arg.first + ", " + round_arg.second + ", "
            + pad_arg.first + ", " + pad_arg.second + ");\n";
    }

    void visit_FileClose(const ASR::FileClose_t &x) {
        headers.insert("string.h");
        std::string indent(indentation_level * indentation_spaces, ' ');

        auto visit_scalar_ptr = [&](ASR::expr_t *expr) -> std::string {
            if (!expr) return "NULL";
            this->visit_expr(*expr);
            return "&(" + src + ")";
        };

        auto visit_string_arg = [&](ASR::expr_t *expr) -> std::pair<std::string, std::string> {
            if (!expr) return {"NULL", "0"};
            this->visit_expr(*expr);
            std::string value = src;
            return {value, "strlen(" + value + ")"};
        };

        this->visit_expr(*x.m_unit);
        std::string unit = src;
        std::pair<std::string, std::string> status_arg = visit_string_arg(x.m_status);

        src = indent + "_lfortran_close("
            + unit + ", "
            + status_arg.first + ", " + status_arg.second + ", "
            + visit_scalar_ptr(x.m_iostat) + ");\n";
    }

    void visit_FileRead(const ASR::FileRead_t &x) {
        if (x.m_overloaded) {
            this->visit_stmt(*x.m_overloaded);
            return;
        }

        headers.insert("string.h");
        std::string indent(indentation_level * indentation_spaces, ' ');

        auto visit_string_arg = [&](ASR::expr_t *expr) -> std::pair<std::string, std::string> {
            if (!expr) return {"NULL", "0"};
            this->visit_expr(*expr);
            std::string value = src;
            ASR::String_t *str_type = ASRUtils::get_string_type(expr);
            if (str_type && str_type->m_len) {
                this->visit_expr(*str_type->m_len);
                return {value, src};
            }
            return {value, "((" + value + ") != NULL ? strlen(" + value + ") : 0)"};
        };

        std::string unit = "-1";
        if (x.m_unit) {
            this->visit_expr(*x.m_unit);
            unit = src;
        }

        std::string iostat_ptr = "NULL";
        std::string iostat_val = "0";
        if (x.m_iostat) {
            this->visit_expr(*x.m_iostat);
            iostat_val = src;
            iostat_ptr = "&(" + iostat_val + ")";
        }

        std::string size_ptr = "NULL";
        if (x.m_size) {
            this->visit_expr(*x.m_size);
            size_ptr = "&(" + src + ")";
        }

        std::pair<std::string, std::string> advance_arg;
        if (x.m_advance) {
            advance_arg = visit_string_arg(x.m_advance);
        } else {
            advance_arg = {"\"yes\"", "3"};
        }

        std::pair<std::string, std::string> fmt_arg = visit_string_arg(x.m_fmt);
        std::pair<std::string, std::string> iomsg_arg = visit_string_arg(x.m_iomsg);

        ASR::ttype_t *unit_type = x.m_unit ? ASRUtils::expr_type(x.m_unit) : nullptr;
        bool is_internal_string_read = unit_type && ASRUtils::is_character(*unit_type);

        if (is_internal_string_read) {
            auto unit_arg = visit_string_arg(x.m_unit);
            std::string format_arg = x.m_fmt ? fmt_arg.first : "NULL";
            std::string offset_name = get_unique_local_name("__lfortran_read_offset");
            std::string internal_read_code;
            if (x.m_iomsg) {
                internal_read_code += indent + "if (" + iomsg_arg.first + " == NULL) " + iomsg_arg.first
                    + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                    + iomsg_arg.second + ");\n";
            }

            if (x.m_is_formatted && x.m_fmt) {
                std::string formatted_args;
                std::string formatted_post;
                int formatted_arg_count = 0;

                auto append_internal_formatted_arg = [&](ASR::expr_t *value_expr,
                                                         std::string &args,
                                                         std::string &post,
                                                         int &arg_count) {
                    ASR::expr_t *value_expr_unwrapped = value_expr;
                    while (value_expr_unwrapped != nullptr) {
                        if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*value_expr_unwrapped)) {
                            value_expr_unwrapped = ASR::down_cast<ASR::ArrayPhysicalCast_t>(value_expr_unwrapped)->m_arg;
                        } else if (ASR::is_a<ASR::GetPointer_t>(*value_expr_unwrapped)) {
                            value_expr_unwrapped = ASR::down_cast<ASR::GetPointer_t>(value_expr_unwrapped)->m_arg;
                        } else if (ASR::is_a<ASR::Cast_t>(*value_expr_unwrapped)) {
                            value_expr_unwrapped = ASR::down_cast<ASR::Cast_t>(value_expr_unwrapped)->m_arg;
                        } else if (ASR::is_a<ASR::BitCast_t>(*value_expr_unwrapped)) {
                            value_expr_unwrapped = ASR::down_cast<ASR::BitCast_t>(value_expr_unwrapped)->m_source;
                        } else if (ASR::is_a<ASR::StringPhysicalCast_t>(*value_expr_unwrapped)) {
                            value_expr_unwrapped = ASR::down_cast<ASR::StringPhysicalCast_t>(value_expr_unwrapped)->m_arg;
                        } else {
                            break;
                        }
                    }

                    ASR::ttype_t *value_type = ASRUtils::expr_type(value_expr_unwrapped);
                    ASR::ttype_t *value_type_past_allocatable =
                        ASRUtils::type_get_past_allocatable_pointer(value_type);

                    if (ASR::is_a<ASR::ArraySection_t>(*value_expr_unwrapped) ||
                        ASR::is_a<ASR::Array_t>(*value_type_past_allocatable)) {
                        ASR::ArraySection_t *section = nullptr;
                        ASR::expr_t *base_expr = value_expr_unwrapped;
                        if (ASR::is_a<ASR::ArraySection_t>(*value_expr_unwrapped)) {
                            section = ASR::down_cast<ASR::ArraySection_t>(value_expr_unwrapped);
                            if (section->n_args != 1) {
                                throw CodeGenError("C backend FileRead currently supports only rank-1 array sections for formatted internal string reads",
                                    value_expr->base.loc);
                            }
                            base_expr = section->m_v;
                        }

                        ASR::ttype_t *base_type = ASRUtils::expr_type(base_expr);
                        ASR::ttype_t *element_type = ASRUtils::extract_type(value_type_past_allocatable);
                        int n_dims = ASRUtils::extract_n_dims_from_ttype(value_type_past_allocatable);
                        if (n_dims != 1) {
                            throw CodeGenError("C backend FileRead currently supports only rank-1 arrays for formatted internal string reads",
                                value_expr->base.loc);
                        }

                        this->visit_expr(*base_expr);
                        std::string base = src;
                        ASR::array_physical_typeType base_phys = ASRUtils::extract_physical_type(base_type);
                        std::string data_ptr = get_c_array_data_expr(base_expr, base);
                        std::string base_offset_expr = get_c_array_offset_expr(base_expr, base);
                        std::string base_stride_expr = get_c_array_stride_expr(base_expr, base);

                        std::string lower_bound_expr;
                        if (base_phys == ASR::array_physical_typeType::DescriptorArray ||
                            base_phys == ASR::array_physical_typeType::PointerArray ||
                            base_phys == ASR::array_physical_typeType::UnboundedPointerArray) {
                            lower_bound_expr = base + "->dims[0].lower_bound";
                        } else {
                            ASR::dimension_t *base_dims = nullptr;
                            int base_n_dims = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
                            if (base_n_dims < 1) {
                                throw CodeGenError("C backend FileRead expected array dimensions for formatted internal string-read array base",
                                    value_expr->base.loc);
                            }
                            if (base_dims[0].m_start) {
                                this->visit_expr(*base_dims[0].m_start);
                                lower_bound_expr = src;
                            } else {
                                lower_bound_expr = std::to_string(lower_bound);
                            }
                        }

                        std::string start_expr = lower_bound_expr;
                        if (section && section->m_args[0].m_left) {
                            this->visit_expr(*section->m_args[0].m_left);
                            start_expr = src;
                        }

                        std::string end_expr;
                        if (section && section->m_args[0].m_right) {
                            this->visit_expr(*section->m_args[0].m_right);
                            end_expr = src;
                        } else if (base_phys == ASR::array_physical_typeType::DescriptorArray ||
                                   base_phys == ASR::array_physical_typeType::PointerArray ||
                                   base_phys == ASR::array_physical_typeType::UnboundedPointerArray) {
                            end_expr = "(" + base + "->dims[0].lower_bound + " + base + "->dims[0].length - 1)";
                        } else {
                            ASR::dimension_t *base_dims = nullptr;
                            int base_n_dims = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
                            if (base_n_dims < 1) {
                                throw CodeGenError("C backend FileRead expected array dimensions for formatted internal string-read array base",
                                    value_expr->base.loc);
                            }
                            if (base_dims[0].m_length) {
                                this->visit_expr(*base_dims[0].m_length);
                                end_expr = "((" + lower_bound_expr + ") + (" + src + ") - 1)";
                            } else {
                                throw CodeGenError("C backend FileRead expected array extent for formatted internal string-read array base",
                                    value_expr->base.loc);
                            }
                        }

                        std::string step_expr = "1";
                        if (section && section->m_args[0].m_step) {
                            this->visit_expr(*section->m_args[0].m_step);
                            step_expr = src;
                        }

                        std::string array_size = "(((" + end_expr + ") - (" + start_expr + ")) / (" + step_expr + ") + 1)";
                        std::string start_offset = "((" + start_expr + ") - (" + lower_bound_expr + "))";
                        std::string array_data_ptr = "(" + data_ptr + " + (" + base_offset_expr
                            + " + (" + base_stride_expr + ") * (" + start_offset + ")))";
                        std::string array_step = "((" + base_stride_expr + ") * (" + step_expr + "))";

                        if (ASR::is_a<ASR::Integer_t>(*element_type)) {
                            int kind = ASR::down_cast<ASR::Integer_t>(element_type)->m_kind;
                            if (kind == 4) {
                                args += ", 1, 2, " + array_data_ptr + ", " + array_size + ", " + array_step;
                            } else if (kind == 8) {
                                args += ", 1, 3, " + array_data_ptr + ", " + array_size + ", " + array_step;
                            } else {
                                throw CodeGenError("C backend FileRead supports formatted internal integer-array reads only for kind=4 or kind=8",
                                    value_expr->base.loc);
                            }
                            arg_count++;
                            return;
                        }

                        if (ASR::is_a<ASR::Real_t>(*element_type)) {
                            int kind = ASR::down_cast<ASR::Real_t>(element_type)->m_kind;
                            if (kind == 4) {
                                args += ", 1, 4, " + array_data_ptr + ", " + array_size + ", " + array_step;
                            } else if (kind == 8) {
                                args += ", 1, 5, " + array_data_ptr + ", " + array_size + ", " + array_step;
                            } else {
                                throw CodeGenError("C backend FileRead supports formatted internal real-array reads only for kind=4 or kind=8",
                                    value_expr->base.loc);
                            }
                            arg_count++;
                            return;
                        }

                        if (ASR::is_a<ASR::Complex_t>(*element_type)) {
                            int kind = ASR::down_cast<ASR::Complex_t>(element_type)->m_kind;
                            if (kind == 4) {
                                args += ", 1, 6, " + array_data_ptr + ", " + array_size + ", " + array_step;
                            } else if (kind == 8) {
                                args += ", 1, 7, " + array_data_ptr + ", " + array_size + ", " + array_step;
                            } else {
                                throw CodeGenError("C backend FileRead supports formatted internal complex-array reads only for kind=4 or kind=8",
                                    value_expr->base.loc);
                            }
                            arg_count++;
                            return;
                        }

                        if (ASR::is_a<ASR::Logical_t>(*element_type)) {
                            int kind = ASR::down_cast<ASR::Logical_t>(element_type)->m_kind;
                            args += ", 1, 1, " + array_data_ptr + ", " + array_size + ", " + array_step;
                            if (kind != 4) {
                                throw CodeGenError("C backend FileRead supports formatted internal logical-array reads only for kind=4",
                                    value_expr->base.loc);
                            }
                            arg_count++;
                            return;
                        }

                        throw CodeGenError("C backend FileRead currently supports only numeric/logical arrays for formatted internal string reads",
                            value_expr->base.loc);
                    }

                    this->visit_expr(*value_expr);
                    std::string value = src;

                    if (ASRUtils::is_character(*value_type_past_allocatable)) {
                        ASR::String_t *value_str_type = ASRUtils::get_string_type(value_expr);
                        std::string value_len = "strlen(" + value + ")";
                        if (value_str_type && value_str_type->m_len) {
                            this->visit_expr(*value_str_type->m_len);
                            value_len = src;
                        }
                        std::string tmp_name, setup_readback, post_readback;
                        if (prepare_string_readback_target(value_expr, value_len,
                                tmp_name, setup_readback, post_readback)) {
                            internal_read_code += setup_readback;
                            args += ", 0, 0, &(" + tmp_name + "), " + value_len;
                            post += post_readback;
                            arg_count++;
                            return;
                        }
                        value = get_c_mutable_scalar_expr(value_expr);
                        internal_read_code += indent + "if (" + value + " == NULL) " + value
                            + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                            + value_len + ");\n";
                        args += ", 0, 0, &(" + value + "), " + value_len;
                        arg_count++;
                        return;
                    }

                    if (ASR::is_a<ASR::Logical_t>(*value_type_past_allocatable)) {
                        std::string tmp_name = get_unique_local_name("__lfortran_fmt_logical");
                        internal_read_code += indent + "int32_t " + tmp_name + " = 0;\n";
                        args += ", 0, 1, &" + tmp_name;
                        post += indent + value + " = (" + tmp_name + " != 0);\n";
                        arg_count++;
                        return;
                    }

                    if (ASR::is_a<ASR::Integer_t>(*value_type_past_allocatable)) {
                        int kind = ASR::down_cast<ASR::Integer_t>(value_type_past_allocatable)->m_kind;
                        if (kind == 4) {
                            args += ", 0, 2, &(" + value + ")";
                        } else if (kind == 8) {
                            args += ", 0, 3, &(" + value + ")";
                        } else {
                            throw CodeGenError("C backend FileRead supports formatted internal scalar integer reads only for kind=4 or kind=8",
                                value_expr->base.loc);
                        }
                        arg_count++;
                        return;
                    }

                    if (ASR::is_a<ASR::Real_t>(*value_type_past_allocatable)) {
                        int kind = ASR::down_cast<ASR::Real_t>(value_type_past_allocatable)->m_kind;
                        if (kind == 4) {
                            args += ", 0, 4, &(" + value + ")";
                        } else if (kind == 8) {
                            args += ", 0, 5, &(" + value + ")";
                        } else {
                            throw CodeGenError("C backend FileRead supports formatted internal scalar real reads only for kind=4 or kind=8",
                                value_expr->base.loc);
                        }
                        arg_count++;
                        return;
                    }

                    if (ASR::is_a<ASR::Complex_t>(*value_type_past_allocatable)) {
                        int kind = ASR::down_cast<ASR::Complex_t>(value_type_past_allocatable)->m_kind;
                        if (kind == 4) {
                            args += ", 0, 6, &(" + value + ")";
                        } else if (kind == 8) {
                            args += ", 0, 7, &(" + value + ")";
                        } else {
                            throw CodeGenError("C backend FileRead supports formatted internal scalar complex reads only for kind=4 or kind=8",
                                value_expr->base.loc);
                        }
                        arg_count++;
                        return;
                    }

                    throw CodeGenError("C backend FileRead currently supports only scalar integer/real/complex/logical/character values for formatted internal string reads",
                        value_expr->base.loc);
                };

                for (size_t i = 0; i < x.n_values; i++) {
                    append_internal_formatted_arg(x.m_values[i], formatted_args, formatted_post,
                        formatted_arg_count);
                }

                internal_read_code += indent + "_lfortran_string_formatted_read("
                    + unit_arg.first + ", " + unit_arg.second + ", "
                    + iostat_ptr + ", " + size_ptr + ", "
                    + advance_arg.first + ", " + advance_arg.second + ", "
                    + fmt_arg.first + ", " + fmt_arg.second + ", "
                    + std::to_string(formatted_arg_count) + ", NULL, 0"
                    + formatted_args + ");\n";
                internal_read_code += formatted_post;
                if (x.m_iomsg && x.m_iostat) {
                    internal_read_code += indent + "_lfortran_set_read_iomsg(" + iostat_val + ", "
                        + iomsg_arg.first + ", " + iomsg_arg.second + ");\n";
                }
                src = std::move(internal_read_code);
                return;
            }

            internal_read_code += indent + "int64_t " + offset_name + " = 0;\n";

            for (size_t i = 0; i < x.n_values; i++) {
                ASR::expr_t *value_expr = x.m_values[i];
                ASR::expr_t *value_expr_unwrapped = value_expr;
                while (value_expr_unwrapped != nullptr) {
                    if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*value_expr_unwrapped)) {
                        value_expr_unwrapped = ASR::down_cast<ASR::ArrayPhysicalCast_t>(value_expr_unwrapped)->m_arg;
                    } else if (ASR::is_a<ASR::GetPointer_t>(*value_expr_unwrapped)) {
                        value_expr_unwrapped = ASR::down_cast<ASR::GetPointer_t>(value_expr_unwrapped)->m_arg;
                    } else if (ASR::is_a<ASR::Cast_t>(*value_expr_unwrapped)) {
                        value_expr_unwrapped = ASR::down_cast<ASR::Cast_t>(value_expr_unwrapped)->m_arg;
                    } else if (ASR::is_a<ASR::BitCast_t>(*value_expr_unwrapped)) {
                        value_expr_unwrapped = ASR::down_cast<ASR::BitCast_t>(value_expr_unwrapped)->m_source;
                    } else if (ASR::is_a<ASR::StringPhysicalCast_t>(*value_expr_unwrapped)) {
                        value_expr_unwrapped = ASR::down_cast<ASR::StringPhysicalCast_t>(value_expr_unwrapped)->m_arg;
                    } else {
                        break;
                    }
                }
                ASR::ttype_t *value_type = ASRUtils::expr_type(value_expr_unwrapped);
                ASR::ttype_t *value_type_past_allocatable = ASRUtils::type_get_past_allocatable_pointer(value_type);
                auto emit_scalar_read = [&](const std::string &helper, const std::string &target_expr) {
                    internal_read_code += indent + helper + "("
                        + unit_arg.first + ", " + unit_arg.second + ", "
                        + format_arg + ", " + target_expr + ", "
                        + iostat_ptr + ", &" + offset_name + ");\n";
                };

                if (ASR::is_a<ASR::ArraySection_t>(*value_expr_unwrapped) ||
                    ASR::is_a<ASR::Array_t>(*value_type_past_allocatable)) {
                    ASR::ArraySection_t *section = nullptr;
                    ASR::expr_t *base_expr = value_expr_unwrapped;
                    if (ASR::is_a<ASR::ArraySection_t>(*value_expr_unwrapped)) {
                        section = ASR::down_cast<ASR::ArraySection_t>(value_expr_unwrapped);
                        if (section->n_args != 1) {
                            throw CodeGenError("C backend FileRead currently supports only rank-1 array sections for internal string reads",
                                value_expr->base.loc);
                        }
                        base_expr = section->m_v;
                    }
                    ASR::ttype_t *element_type = ASRUtils::extract_type(value_type_past_allocatable);
                    if (!(ASR::is_a<ASR::Integer_t>(*element_type) || ASR::is_a<ASR::Real_t>(*element_type) ||
                          ASR::is_a<ASR::Complex_t>(*element_type) || ASR::is_a<ASR::Logical_t>(*element_type))) {
                        throw CodeGenError("C backend FileRead currently supports only numeric/logical array sections for internal string reads",
                            value_expr->base.loc);
                    }

                    this->visit_expr(*base_expr);
                    std::string base = src;
                    ASR::ttype_t *base_type = ASRUtils::expr_type(base_expr);
                    ASR::array_physical_typeType base_phys = ASRUtils::extract_physical_type(base_type);
                    std::string data_ptr = get_c_array_data_expr(base_expr, base);
                    std::string base_offset_expr = get_c_array_offset_expr(base_expr, base);
                    std::string base_stride_expr = get_c_array_stride_expr(base_expr, base);

                    std::string lower_bound_expr;
                    if (base_phys == ASR::array_physical_typeType::DescriptorArray ||
                        base_phys == ASR::array_physical_typeType::PointerArray ||
                        base_phys == ASR::array_physical_typeType::UnboundedPointerArray) {
                        lower_bound_expr = base + "->dims[0].lower_bound";
                    } else {
                        ASR::dimension_t *base_dims = nullptr;
                        int base_n_dims = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
                        if (base_n_dims < 1) {
                            throw CodeGenError("C backend FileRead expected array dimensions for array section base",
                                value_expr->base.loc);
                        }
                        if (base_dims[0].m_start) {
                            this->visit_expr(*base_dims[0].m_start);
                            lower_bound_expr = src;
                        } else {
                            lower_bound_expr = std::to_string(lower_bound);
                        }
                    }

                    std::string start_expr = lower_bound_expr;
                    if (section && section->m_args[0].m_left) {
                        this->visit_expr(*section->m_args[0].m_left);
                        start_expr = src;
                    }

                    std::string end_expr;
                    if (section && section->m_args[0].m_right) {
                        this->visit_expr(*section->m_args[0].m_right);
                        end_expr = src;
                    } else if (base_phys == ASR::array_physical_typeType::DescriptorArray ||
                               base_phys == ASR::array_physical_typeType::PointerArray ||
                               base_phys == ASR::array_physical_typeType::UnboundedPointerArray) {
                        end_expr = "(" + base + "->dims[0].lower_bound + " + base + "->dims[0].length - 1)";
                    } else {
                        ASR::dimension_t *base_dims = nullptr;
                        int base_n_dims = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
                        if (base_n_dims < 1) {
                            throw CodeGenError("C backend FileRead expected array dimensions for array section base",
                                value_expr->base.loc);
                        }
                        if (base_dims[0].m_length) {
                            this->visit_expr(*base_dims[0].m_length);
                            end_expr = "((" + lower_bound_expr + ") + (" + src + ") - 1)";
                        } else {
                            throw CodeGenError("C backend FileRead expected array extent for fixed-size array section base",
                                value_expr->base.loc);
                        }
                    }

                    std::string step_expr = "1";
                    if (section && section->m_args[0].m_step) {
                        this->visit_expr(*section->m_args[0].m_step);
                        step_expr = src;
                    }

                    std::string idx_name = get_unique_local_name("__lfortran_read_idx");
                    std::string off_name = get_unique_local_name("__lfortran_read_off");
                    internal_read_code += indent + "for (int32_t " + idx_name + " = " + start_expr + ", "
                        + off_name + " = ((" + start_expr + ") - (" + lower_bound_expr + ")); "
                        + "(((" + step_expr + ") >= 0) && (" + idx_name + " <= " + end_expr + ")) || "
                        + "(((" + step_expr + ") < 0) && (" + idx_name + " >= " + end_expr + ")); "
                        + idx_name + " += (" + step_expr + "), " + off_name + " += (" + step_expr + ")) {\n";

                    std::string elem_index = "(" + base_offset_expr + " + (" + base_stride_expr
                        + ") * (" + off_name + "))";
                    std::string elem_ptr = "&(" + data_ptr + "[" + elem_index + "])";
                    std::string inner_indent = indent + std::string(indentation_spaces, ' ');
                    if (ASR::is_a<ASR::Integer_t>(*element_type)) {
                        int kind = ASR::down_cast<ASR::Integer_t>(element_type)->m_kind;
                        if (kind == 1) {
                            internal_read_code += inner_indent + "_lfortran_string_read_i8(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (int8_t*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else if (kind == 2) {
                            internal_read_code += inner_indent + "_lfortran_string_read_i16(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (int16_t*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else if (kind == 4) {
                            internal_read_code += inner_indent + "_lfortran_string_read_i32(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (int32_t*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else if (kind == 8) {
                            internal_read_code += inner_indent + "_lfortran_string_read_i64(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (int64_t*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else {
                            throw CodeGenError("C backend FileRead does not support this integer kind for internal string-read array sections",
                                value_expr->base.loc);
                        }
                    } else if (ASR::is_a<ASR::Real_t>(*element_type)) {
                        int kind = ASR::down_cast<ASR::Real_t>(element_type)->m_kind;
                        if (kind == 4) {
                            internal_read_code += inner_indent + "_lfortran_string_read_f32(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (float*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else if (kind == 8) {
                            internal_read_code += inner_indent + "_lfortran_string_read_f64(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (double*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else {
                            throw CodeGenError("C backend FileRead does not support this real kind for internal string-read array sections",
                                value_expr->base.loc);
                        }
                    } else if (ASR::is_a<ASR::Complex_t>(*element_type)) {
                        int kind = ASR::down_cast<ASR::Complex_t>(element_type)->m_kind;
                        if (kind == 4) {
                            internal_read_code += inner_indent + "_lfortran_string_read_c32(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (struct _lfortran_complex_32*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else if (kind == 8) {
                            internal_read_code += inner_indent + "_lfortran_string_read_c64(" + unit_arg.first + ", " + unit_arg.second + ", "
                                + format_arg + ", (struct _lfortran_complex_64*)" + elem_ptr + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        } else {
                            throw CodeGenError("C backend FileRead does not support this complex kind for internal string-read array sections",
                                value_expr->base.loc);
                        }
                    } else if (ASR::is_a<ASR::Logical_t>(*element_type)) {
                        std::string tmp_name = get_unique_local_name("__lfortran_read_logical");
                        internal_read_code += inner_indent + "int32_t " + tmp_name + " = 0;\n";
                        internal_read_code += inner_indent + "_lfortran_string_read_bool(" + unit_arg.first + ", " + unit_arg.second + ", "
                            + format_arg + ", &" + tmp_name + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        internal_read_code += inner_indent + data_ptr + "[" + elem_index + "] = (" + tmp_name + " != 0);\n";
                    }
                    internal_read_code += indent + "}\n";
                    continue;
                }

                this->visit_expr(*value_expr);
                std::string value = src;

                if (ASR::is_a<ASR::Integer_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Integer_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 1) {
                        emit_scalar_read("_lfortran_string_read_i8", "&(" + value + ")");
                    } else if (kind == 2) {
                        emit_scalar_read("_lfortran_string_read_i16", "&(" + value + ")");
                    } else if (kind == 4) {
                        emit_scalar_read("_lfortran_string_read_i32", "&(" + value + ")");
                    } else if (kind == 8) {
                        emit_scalar_read("_lfortran_string_read_i64", "&(" + value + ")");
                    } else {
                        throw CodeGenError("C backend FileRead does not support this integer kind for internal string reads",
                            value_expr->base.loc);
                    }
                } else if (ASR::is_a<ASR::Real_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Real_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        emit_scalar_read("_lfortran_string_read_f32", "&(" + value + ")");
                    } else if (kind == 8) {
                        emit_scalar_read("_lfortran_string_read_f64", "&(" + value + ")");
                    } else {
                        throw CodeGenError("C backend FileRead does not support this real kind for internal string reads",
                            value_expr->base.loc);
                    }
                } else if (ASR::is_a<ASR::Complex_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Complex_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        emit_scalar_read("_lfortran_string_read_c32", "&(" + value + ")");
                    } else if (kind == 8) {
                        emit_scalar_read("_lfortran_string_read_c64", "&(" + value + ")");
                    } else {
                        throw CodeGenError("C backend FileRead does not support this complex kind for internal string reads",
                            value_expr->base.loc);
                    }
                } else if (ASR::is_a<ASR::Logical_t>(*value_type_past_allocatable)) {
                    std::string tmp_name = get_unique_local_name("__lfortran_read_logical");
                    internal_read_code += indent + "int32_t " + tmp_name + " = 0;\n";
                    emit_scalar_read("_lfortran_string_read_bool", "&" + tmp_name);
                    internal_read_code += indent + value + " = (" + tmp_name + " != 0);\n";
                } else if (ASRUtils::is_character(*value_type_past_allocatable)) {
                    ASR::String_t *value_str_type = ASRUtils::get_string_type(value_expr);
                    std::string value_len = "strlen(" + value + ")";
                    if (value_str_type && value_str_type->m_len) {
                        this->visit_expr(*value_str_type->m_len);
                        value_len = src;
                    }
                    std::string tmp_name, setup_readback, post_readback;
                    if (prepare_string_readback_target(value_expr, value_len,
                            tmp_name, setup_readback, post_readback)) {
                        internal_read_code += setup_readback;
                        internal_read_code += indent + "_lfortran_string_read_str("
                            + unit_arg.first + ", " + unit_arg.second + ", "
                            + tmp_name + ", " + value_len + ", "
                            + "&" + offset_name + ");\n";
                        internal_read_code += post_readback;
                        if (x.m_iostat) {
                            internal_read_code += indent + iostat_val + " = 0;\n";
                        }
                        continue;
                    }
                    value = get_c_mutable_scalar_expr(value_expr);
                    internal_read_code += indent + "if (" + value + " == NULL) " + value
                        + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                        + value_len + ");\n";
                    internal_read_code += indent + "_lfortran_string_read_str("
                        + unit_arg.first + ", " + unit_arg.second + ", "
                        + value + ", " + value_len + ", "
                        + "&" + offset_name + ");\n";
                    if (x.m_iostat) {
                        internal_read_code += indent + iostat_val + " = 0;\n";
                    }
                } else {
                    throw CodeGenError("C backend FileRead currently supports only scalar integer/real/complex/logical/character values for internal string reads [node="
                        + std::to_string(static_cast<int>(value_expr->type)) + ", type="
                        + ASRUtils::type_to_str_python_expr(value_type, value_expr) + "]",
                        value_expr->base.loc);
                }
            }

            if (x.m_iomsg && x.m_iostat) {
                internal_read_code += indent + "_lfortran_set_read_iomsg(" + iostat_val + ", "
                    + iomsg_arg.first + ", " + iomsg_arg.second + ");\n";
            }
            src = std::move(internal_read_code);
            return;
        }

        src.clear();
        if (x.m_iomsg) {
            src += indent + "if (" + iomsg_arg.first + " == NULL) " + iomsg_arg.first
                + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                + iomsg_arg.second + ");\n";
        }

        std::string read_code;
        std::string setup_code = src;
        src.clear();

        auto emit_external_read_for_value = [&](ASR::expr_t *value_expr) {
            ASR::ttype_t *value_type = ASRUtils::expr_type(value_expr);
            ASR::ttype_t *value_type_past_allocatable = ASRUtils::type_get_past_allocatable_pointer(value_type);

            this->visit_expr(*value_expr);
            std::string value = src;

            if (!x.m_is_formatted) {
                if (ASRUtils::is_array(value_type_past_allocatable)) {
                    ASR::ttype_t *element_type = ASRUtils::extract_type(value_type_past_allocatable);
                    ASR::array_physical_typeType phys = ASRUtils::extract_physical_type(value_type_past_allocatable);
                    std::string data_ptr = value;
                    if (phys == ASR::array_physical_typeType::DescriptorArray ||
                        phys == ASR::array_physical_typeType::PointerArray ||
                        phys == ASR::array_physical_typeType::UnboundedPointerArray) {
                        data_ptr += "->data";
                    }

                    std::string array_size;
                    if (ASRUtils::is_fixed_size_array(value_type_past_allocatable)) {
                        array_size = std::to_string(ASRUtils::get_fixed_size_of_array(value_type_past_allocatable));
                    } else {
                        ASR::dimension_t *m_dims = nullptr;
                        int n_dims = ASRUtils::extract_dimensions_from_ttype(value_type_past_allocatable, m_dims);
                        std::string array_size_func = c_utils_functions->get_array_size();
                        array_size = "((int32_t) " + array_size_func + "(" + value + "->dims, " + std::to_string(n_dims) + "))";
                    }

                    if (ASR::is_a<ASR::Integer_t>(*element_type)) {
                        int kind = ASR::down_cast<ASR::Integer_t>(element_type)->m_kind;
                        if (kind == 1) {
                            read_code += indent + "_lfortran_read_array_int8((int8_t*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else if (kind == 2) {
                            read_code += indent + "_lfortran_read_array_int16((int16_t*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else if (kind == 4) {
                            read_code += indent + "_lfortran_read_array_int32((int32_t*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else if (kind == 8) {
                            read_code += indent + "_lfortran_read_array_int64((int64_t*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else {
                            throw CodeGenError("C backend FileRead does not support this integer-array kind for list-directed external reads",
                                value_expr->base.loc);
                        }
                    } else if (ASR::is_a<ASR::Real_t>(*element_type)) {
                        int kind = ASR::down_cast<ASR::Real_t>(element_type)->m_kind;
                        if (kind == 4) {
                            read_code += indent + "_lfortran_read_array_float((float*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else if (kind == 8) {
                            read_code += indent + "_lfortran_read_array_double((double*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else {
                            throw CodeGenError("C backend FileRead does not support this real-array kind for list-directed external reads",
                                value_expr->base.loc);
                        }
                    } else if (ASR::is_a<ASR::Complex_t>(*element_type)) {
                        int kind = ASR::down_cast<ASR::Complex_t>(element_type)->m_kind;
                        if (kind == 4) {
                            read_code += indent + "_lfortran_read_array_complex_float((struct _lfortran_complex_32*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else if (kind == 8) {
                            read_code += indent + "_lfortran_read_array_complex_double((struct _lfortran_complex_64*)" + data_ptr + ", " + array_size + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                        } else {
                            throw CodeGenError("C backend FileRead does not support this complex-array kind for list-directed external reads",
                                value_expr->base.loc);
                        }
                    } else if (ASR::is_a<ASR::Logical_t>(*element_type)) {
                        int kind = ASR::down_cast<ASR::Logical_t>(element_type)->m_kind;
                        read_code += indent + "_lfortran_read_array_logical((void*)" + data_ptr + ", " + array_size + ", "
                            + std::to_string(kind) + ", 1, " + unit + ", " + iostat_ptr + ");\n";
                    } else if (ASRUtils::is_character(*element_type)) {
                        ASR::String_t *str_type = ASRUtils::get_string_type(value_expr);
                        std::string value_len = "strlen(" + data_ptr + ")";
                        if (str_type && str_type->m_len) {
                            this->visit_expr(*str_type->m_len);
                            value_len = src;
                        }
                        read_code += indent + "_lfortran_read_array_char(" + data_ptr + ", " + value_len + ", "
                            + array_size + ", " + unit + ", " + iostat_ptr + ");\n";
                    } else {
                        throw CodeGenError("C backend FileRead currently supports only numeric/logical/character arrays for list-directed external reads",
                            value_expr->base.loc);
                    }
                } else if (ASR::is_a<ASR::Integer_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Integer_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 2) {
                        read_code += indent + "_lfortran_read_int16((int16_t*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                    } else if (kind == 4) {
                        read_code += indent + "_lfortran_read_int32((int32_t*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                    } else if (kind == 8) {
                        read_code += indent + "_lfortran_read_int64((int64_t*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                    } else {
                        throw CodeGenError("C backend FileRead does not support this integer kind for list-directed external reads",
                            value_expr->base.loc);
                    }
                } else if (ASR::is_a<ASR::Real_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Real_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        read_code += indent + "_lfortran_read_float((float*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                    } else if (kind == 8) {
                        read_code += indent + "_lfortran_read_double((double*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                    } else {
                        throw CodeGenError("C backend FileRead does not support this real kind for list-directed external reads",
                            value_expr->base.loc);
                    }
                } else if (ASR::is_a<ASR::Complex_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Complex_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        read_code += indent + "_lfortran_read_complex_float((struct _lfortran_complex_32*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                    } else if (kind == 8) {
                        read_code += indent + "_lfortran_read_complex_double((struct _lfortran_complex_64*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                    } else {
                        throw CodeGenError("C backend FileRead does not support this complex kind for list-directed external reads",
                            value_expr->base.loc);
                    }
                } else if (ASR::is_a<ASR::Logical_t>(*value_type_past_allocatable)) {
                    read_code += indent + "_lfortran_read_logical((bool*)&(" + value + "), " + unit + ", " + iostat_ptr + ");\n";
                } else if (ASRUtils::is_character(*value_type_past_allocatable)) {
                    ASR::String_t *value_str_type = ASRUtils::get_string_type(value_expr);
                    std::string value_len = "strlen(" + value + ")";
                    if (value_str_type && value_str_type->m_len) {
                        this->visit_expr(*value_str_type->m_len);
                        value_len = src;
                    }
                    std::string tmp_name, setup_readback, post_readback;
                    if (prepare_string_readback_target(value_expr, value_len,
                            tmp_name, setup_readback, post_readback)) {
                        read_code += setup_readback;
                        read_code += indent + "_lfortran_read_char(&(" + tmp_name + "), "
                            + value_len + ", " + unit + ", " + iostat_ptr + ");\n";
                        read_code += post_readback;
                        return;
                    }
                    value = get_c_mutable_scalar_expr(value_expr);
                    read_code += indent + "if (" + value + " == NULL) " + value
                        + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                        + value_len + ");\n";
                    read_code += indent + "_lfortran_read_char(&(" + value + "), " + value_len + ", " + unit + ", " + iostat_ptr + ");\n";
                } else {
                    throw CodeGenError("C backend FileRead currently supports only scalar integer/real/complex/logical/character values for list-directed external reads",
                        value_expr->base.loc);
                }
            } else {
                if (!ASRUtils::is_character(*value_type)) {
                    throw CodeGenError("C backend FileRead currently supports only scalar character values for formatted external reads",
                        x.base.base.loc);
                }
                ASR::String_t *value_str_type = ASRUtils::get_string_type(value_expr);
                std::string value_len = "strlen(" + value + ")";
                if (value_str_type && value_str_type->m_len) {
                    this->visit_expr(*value_str_type->m_len);
                    value_len = src;
                }
                read_code += indent + "if (" + value + " == NULL) " + value
                    + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                    + value_len + ");\n";
                read_code += indent + "_lfortran_formatted_read("
                    + unit + ", " + iostat_ptr + ", " + size_ptr + ", "
                    + advance_arg.first + ", " + advance_arg.second + ", "
                    + fmt_arg.first + ", " + fmt_arg.second + ", "
                    + "1, 0, 0, &(" + value + "), (int64_t)(" + value_len + "));\n";
            }
        };

        if (x.m_is_formatted) {
            std::string formatted_setup = src;
            std::string formatted_args;
            std::string formatted_post;
            int formatted_arg_count = 0;

            if (x.m_pos) {
                this->visit_expr(*x.m_pos);
                std::string pos = src;
                formatted_setup += indent + "_lfortran_file_seek(" + unit + ", (int64_t)(" + pos + "), " + iostat_ptr + ");\n";
            }

            auto append_formatted_scalar = [&](ASR::expr_t *value_expr,
                                              std::string &setup,
                                              std::string &args,
                                              std::string &post,
                                              int &arg_count) {
                ASR::ttype_t *value_type = ASRUtils::expr_type(value_expr);
                ASR::ttype_t *value_type_past_allocatable = ASRUtils::type_get_past_allocatable_pointer(value_type);

                this->visit_expr(*value_expr);
                std::string value = src;

                if (ASRUtils::is_character(*value_type_past_allocatable)) {
                    ASR::String_t *value_str_type = ASRUtils::get_string_type(value_expr);
                    std::string value_len = "strlen(" + value + ")";
                    if (value_str_type && value_str_type->m_len) {
                        this->visit_expr(*value_str_type->m_len);
                        value_len = src;
                    }
                    std::string tmp_name, setup_readback, post_readback;
                    if (prepare_string_readback_target(value_expr, value_len,
                            tmp_name, setup_readback, post_readback)) {
                        setup += setup_readback;
                        args += ", 0, 0, &(" + tmp_name + "), (int64_t)("
                            + value_len + ")";
                        post += post_readback;
                        arg_count++;
                        return;
                    }
                    value = get_c_mutable_scalar_expr(value_expr);
                    setup += indent + "if (" + value + " == NULL) " + value
                        + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                        + value_len + ");\n";
                    args += ", 0, 0, &(" + value + "), (int64_t)("
                        + value_len + ")";
                    arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Logical_t>(*value_type_past_allocatable)) {
                    std::string tmp_name = get_unique_local_name("__lfortran_fmt_logical");
                    setup += indent + "int32_t " + tmp_name + " = 0;\n";
                    args += ", 0, 1, &" + tmp_name;
                    post += indent + value + " = (" + tmp_name + " != 0);\n";
                    arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Integer_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Integer_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        args += ", 0, 2, &(" + value + ")";
                    } else if (kind == 8) {
                        args += ", 0, 3, &(" + value + ")";
                    } else {
                        throw CodeGenError("C backend FileRead supports formatted scalar integer reads only for kind=4 or kind=8",
                            value_expr->base.loc);
                    }
                    arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Real_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Real_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        args += ", 0, 4, &(" + value + ")";
                    } else if (kind == 8) {
                        args += ", 0, 5, &(" + value + ")";
                    } else {
                        throw CodeGenError("C backend FileRead supports formatted scalar real reads only for kind=4 or kind=8",
                            value_expr->base.loc);
                    }
                    arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Complex_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Complex_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        args += ", 0, 6, &(" + value + ")";
                    } else if (kind == 8) {
                        args += ", 0, 7, &(" + value + ")";
                    } else {
                        throw CodeGenError("C backend FileRead supports formatted scalar complex reads only for kind=4 or kind=8",
                            value_expr->base.loc);
                    }
                    arg_count++;
                    return;
                }

                throw CodeGenError("C backend FileRead currently supports only scalar integer/real/complex/logical/character values for formatted external reads",
                    value_expr->base.loc);
            };

            bool has_implied_do = false;
            for (size_t i = 0; i < x.n_values; i++) {
                if (ASR::is_a<ASR::ImpliedDoLoop_t>(*x.m_values[i])) {
                    has_implied_do = true;
                    break;
                }
            }

            if (has_implied_do) {
                auto emit_formatted_scalar_call = [&](ASR::expr_t *value_expr) {
                    std::string accumulated_src = src;
                    std::string single_setup;
                    std::string single_args;
                    std::string single_post;
                    int single_arg_count = 0;
                    append_formatted_scalar(value_expr, single_setup, single_args, single_post, single_arg_count);
                    std::string emitted = single_setup;
                    emitted += std::string(indentation_level * indentation_spaces, ' ')
                        + "_lfortran_formatted_read("
                        + unit + ", " + iostat_ptr + ", " + size_ptr + ", "
                        + advance_arg.first + ", " + advance_arg.second + ", "
                        + fmt_arg.first + ", " + fmt_arg.second + ", "
                        + std::to_string(single_arg_count) + ", NULL, 0"
                        + single_args + ");\n";
                    emitted += single_post;
                    src = accumulated_src + emitted;
                };

                src = formatted_setup;
                for (size_t i = 0; i < x.n_values; i++) {
                    if (ASR::is_a<ASR::ImpliedDoLoop_t>(*x.m_values[i])) {
                        ASR::ImpliedDoLoop_t *idl = ASR::down_cast<ASR::ImpliedDoLoop_t>(x.m_values[i]);
                        std::string accumulated_src = src;
                        this->visit_expr(*idl->m_var);
                        std::string loop_var = src;
                        this->visit_expr(*idl->m_start);
                        std::string loop_start = src;
                        this->visit_expr(*idl->m_end);
                        std::string loop_end = src;
                        std::string loop_inc = "1";
                        if (idl->m_increment) {
                            this->visit_expr(*idl->m_increment);
                            loop_inc = src;
                        }
                        src = accumulated_src;
                        src += std::string(indentation_level * indentation_spaces, ' ')
                            + "for (" + loop_var + " = " + loop_start + "; "
                            + "((" + loop_inc + ") >= 0 ? (" + loop_var + " <= " + loop_end + ") : ("
                            + loop_var + " >= " + loop_end + ")); "
                            + loop_var + " += " + loop_inc + ") {\n";
                        indentation_level++;
                        for (size_t j = 0; j < idl->n_values; j++) {
                            if (ASR::is_a<ASR::ImpliedDoLoop_t>(*idl->m_values[j])) {
                                throw CodeGenError("C backend FileRead does not support nested implied do loops for formatted external reads",
                                    idl->m_values[j]->base.loc);
                            }
                            emit_formatted_scalar_call(idl->m_values[j]);
                        }
                        indentation_level--;
                        src += std::string(indentation_level * indentation_spaces, ' ') + "}\n";
                    } else {
                        emit_formatted_scalar_call(x.m_values[i]);
                    }
                }
                if (x.m_iomsg && x.m_iostat) {
                    src += indent + "_lfortran_set_read_iomsg(" + iostat_val + ", "
                        + iomsg_arg.first + ", " + iomsg_arg.second + ");\n";
                }
                return;
            }

            for (size_t i = 0; i < x.n_values; i++) {
                append_formatted_scalar(x.m_values[i], formatted_setup, formatted_args,
                    formatted_post, formatted_arg_count);
            }

            src = formatted_setup;
            src += indent + "_lfortran_formatted_read("
                + unit + ", " + iostat_ptr + ", " + size_ptr + ", "
                + advance_arg.first + ", " + advance_arg.second + ", "
                + fmt_arg.first + ", " + fmt_arg.second + ", "
                + std::to_string(formatted_arg_count) + ", NULL, 0"
                + formatted_args + ");\n";
            src += formatted_post;
            if (x.m_iomsg && x.m_iostat) {
                src += indent + "_lfortran_set_read_iomsg(" + iostat_val + ", "
                    + iomsg_arg.first + ", " + iomsg_arg.second + ");\n";
            }
            return;
        }

        if (x.m_pos) {
            this->visit_expr(*x.m_pos);
            std::string pos = src;
            setup_code += indent + "_lfortran_file_seek(" + unit + ", (int64_t)(" + pos + "), " + iostat_ptr + ");\n";
        }

        for (size_t i = 0; i < x.n_values; i++) {
            emit_external_read_for_value(x.m_values[i]);
        }
        src = setup_code + read_code;
        if (x.m_iomsg && x.m_iostat) {
            src += indent + "_lfortran_set_read_iomsg(" + iostat_val + ", "
                + iomsg_arg.first + ", " + iomsg_arg.second + ");\n";
        }
    }

    void visit_CPtrToPointer(const ASR::CPtrToPointer_t& x) {
        visit_expr(*x.m_cptr);
        std::string source_src = std::move(src);
        visit_expr(*x.m_ptr);
        std::string dest_src = std::move(src);
        src = "";
        std::string indent(indentation_level*indentation_spaces, ' ');
        ASR::ArrayConstant_t* lower_bounds = nullptr;
        if( x.m_lower_bounds ) {
            LCOMPILERS_ASSERT(ASR::is_a<ASR::ArrayConstant_t>(*x.m_lower_bounds));
            lower_bounds = ASR::down_cast<ASR::ArrayConstant_t>(x.m_lower_bounds);
        }
        if( ASRUtils::is_array(ASRUtils::expr_type(x.m_ptr)) ) {
            std::string dim_set_code = "";
            ASR::dimension_t* m_dims = nullptr;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(x.m_ptr), m_dims);
            dim_set_code = indent + dest_src + "->n_dims = " + std::to_string(n_dims) + ";\n";
            dim_set_code = indent + dest_src + "->offset = 0;\n";
            std::string stride = "1";
            for (int i = 0; i < n_dims; i++) {
                std::string start = "0", length = "0";
                if( lower_bounds ) {
                    start = ASRUtils::fetch_ArrayConstant_value(lower_bounds, i);
                }
                if( m_dims[i].m_length ) {
                    this->visit_expr(*m_dims[i].m_length);
                    length = src;
                }
                dim_set_code += indent + dest_src +
                    "->dims[" + std::to_string(i) + "].lower_bound = " + start + ";\n";
                dim_set_code += indent + dest_src +
                    "->dims[" + std::to_string(i) + "].length = " + length + ";\n";
                dim_set_code += indent + dest_src +
                    "->dims[" + std::to_string(i) + "].stride = " + stride + ";\n";
                stride = "(" + stride + "*" + length + ")";
            }
            src.clear();
            src += dim_set_code;
            dest_src += "->data";
        }
        ASR::ttype_t *ptr_type = ASRUtils::expr_type(x.m_ptr);
        ASR::symbol_t *ptr_type_decl = get_expr_type_declaration_symbol(x.m_ptr);
        std::string type_src;
        if (ASR::is_a<ASR::Pointer_t>(*ptr_type)) {
            ASR::ttype_t *pointee_type = ASR::down_cast<ASR::Pointer_t>(ptr_type)->m_type;
            if (ASR::is_a<ASR::FunctionType_t>(*pointee_type)) {
                type_src = get_function_pointer_type_from_type(
                    ASR::down_cast<ASR::FunctionType_t>(pointee_type));
            } else {
                type_src = get_c_concrete_type_from_ttype_t(ptr_type, ptr_type_decl);
            }
        } else {
            type_src = get_c_concrete_type_from_ttype_t(ptr_type, ptr_type_decl);
        }
        src += indent + dest_src + " = (" + type_src + ") " + source_src + ";\n";
    }

    void visit_FileRewind(const ASR::FileRewind_t &x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string unit = "-1";
        if (x.m_unit) {
            this->visit_expr(*x.m_unit);
            unit = src;
        }
        src = indent + "_lfortran_rewind(" + unit + ", NULL, NULL, 0);\n";
    }

    void visit_FileEndfile(const ASR::FileEndfile_t &x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string unit = "-1";
        if (x.m_unit) {
            this->visit_expr(*x.m_unit);
            unit = src;
        }
        src = indent + "_lfortran_endfile(" + unit + ");\n";
    }

    void visit_FileBackspace(const ASR::FileBackspace_t &x) {
        std::string indent(indentation_level * indentation_spaces, ' ');
        std::string unit = "-1";
        if (x.m_unit) {
            this->visit_expr(*x.m_unit);
            unit = src;
        }
        src = indent + "_lfortran_backspace(" + unit + ");\n";
    }

    void visit_Print(const ASR::Print_t &x) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string tmp_gen = indent + "printf(\"", out = "";
        bracket_open++;
        std::vector<std::string> v;
        std::string separator;
        separator = "\" \"";
        //HACKISH way to handle print refactoring (always using stringformat).
        // TODO : Implement stringformat visitor.
        ASR::StringFormat_t* str_fmt;
        size_t n_values = 0;
        if(ASR::is_a<ASR::StringFormat_t>(*x.m_text)){
            str_fmt = ASR::down_cast<ASR::StringFormat_t>(x.m_text);
            n_values = str_fmt->n_args;
        } else if (ASR::is_a<ASR::String_t>(*ASRUtils::expr_type(x.m_text))) {
            this->visit_expr(*x.m_text);
            src = indent + "printf(\"%s\\n\"," + src + ");\n";
            return;
        } else {
            throw CodeGenError("print statment supported for stringformat and single character argument",
            x.base.base.loc);
        }

        for (size_t i=0; i<n_values; i++) {
            this->visit_expr(*(str_fmt->m_args[i]));
            ASR::ttype_t* value_type = ASRUtils::expr_type(str_fmt->m_args[i]);
            if (ASRUtils::is_array(value_type)) {
                tmp_gen += "\"";
                if (!v.empty()) {
                    for (auto &s: v) {
                        tmp_gen += ", " + s;
                    }
                    tmp_gen += ");\n";
                    out += tmp_gen;
                    v.clear();
                }
                tmp_gen = indent + "printf(\"";
                if (i != 0) {
                    out += indent + "printf(\" \");\n";
                }
                std::string p_func = c_ds_api->get_print_func(value_type);
                out += indent + p_func + "(" + src + ");\n";
                if (i + 1 != n_values) {
                    out += indent + "printf(\" \");\n";
                } else {
                    out += indent + "printf(\"\\n\");\n";
                }
                continue;
            }
            if( ASRUtils::is_array(value_type) ) {
                src += "->data";
            }
            if( ASR::is_a<ASR::List_t>(*value_type) ||
                ASR::is_a<ASR::Tuple_t>(*value_type)) {
                tmp_gen += "\"";
                if (!v.empty()) {
                    for (auto &s: v) {
                        tmp_gen += ", " + s;
                    }
                }
                tmp_gen += ");\n";
                out += tmp_gen;
                tmp_gen = indent + "printf(\"";
                v.clear();
                std::string p_func = c_ds_api->get_print_func(value_type);
                out += indent + p_func + "(" + src + ");\n";
                continue;
            }
            tmp_gen +=c_ds_api->get_print_type(value_type, ASR::is_a<ASR::ArrayItem_t>(*(str_fmt->m_args[i])));
            v.push_back(src);
            if (ASR::is_a<ASR::Complex_t>(*value_type)) {
                v.pop_back();
                v.push_back("creal(" + src + ")");
                v.push_back("cimag(" + src + ")");
            }
            if (i+1!=n_values) {
                tmp_gen += "\%s";
                v.push_back(separator);
            }
        }
        tmp_gen += "\\n\"";
        if (!v.empty()) {
            for (auto &s: v) {
                tmp_gen += ", " + s;
            }
        }
        tmp_gen += ");\n";
        bracket_open--;
        out += tmp_gen;
        src = this->check_tmp_buffer() + out;
    }

    void visit_FileWrite(const ASR::FileWrite_t &x) {
        if (x.m_overloaded) {
            this->visit_stmt(*x.m_overloaded);
            return;
        }

        headers.insert("stdio.h");
        headers.insert("stdlib.h");
        headers.insert("string.h");

        std::string indent(indentation_level*indentation_spaces, ' ');

        auto visit_string_arg = [&](ASR::expr_t *expr) -> std::pair<std::string, std::string> {
            if (!expr) return {"NULL", "0"};
            this->visit_expr(*expr);
            std::string value = src;
            ASR::String_t *str_type = ASRUtils::get_string_type(expr);
            if (str_type && str_type->m_len) {
                this->visit_expr(*str_type->m_len);
                return {value, src};
            }
            return {value, "strlen(" + value + ")"};
        };

        bool is_integer_unit = x.m_unit && ASRUtils::is_integer(*ASRUtils::expr_type(x.m_unit));
        bool is_string_unit = x.m_unit && ASRUtils::is_character(*ASRUtils::expr_type(x.m_unit));

        if (!x.m_unit || (!is_integer_unit && !is_string_unit)) {
            throw CodeGenError("C backend FileWrite currently supports only integer or character units",
                x.base.base.loc);
        }

        std::string unit;
        std::string unit_len = "0";
        std::string unit_len_name;
        std::string unit_setup;
        bool is_unit_allocatable = false;
        bool is_unit_deferred = false;

        if (is_string_unit) {
            auto unit_arg = visit_string_arg(x.m_unit);
            unit = unit_arg.first;
            unit_len = unit_arg.second;
            unit_len_name = get_unique_local_name("__lfortran_unit_len");
            ASR::ttype_t *unit_type = ASRUtils::expr_type(x.m_unit);
            is_unit_allocatable = ASRUtils::is_allocatable(unit_type);
            is_unit_deferred = ASRUtils::is_deferredLength_string(unit_type);
            if (is_unit_allocatable && is_unit_deferred) {
                unit_len = "((" + unit + ") != NULL ? strlen(" + unit + ") : 0)";
            }
            unit_setup += indent + "int64_t " + unit_len_name + " = (int64_t)(" + unit_len + ");\n";
            if (ASR::is_a<ASR::Var_t>(*x.m_unit) && !(is_unit_allocatable && is_unit_deferred)) {
                unit_setup += indent + "if (" + unit + " == NULL) " + unit
                    + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                    + unit_len_name + ");\n";
            }
        } else {
            this->visit_expr(*x.m_unit);
            unit = src;
        }

        std::string iostat_ptr = "NULL";
        if (x.m_iostat) {
            this->visit_expr(*x.m_iostat);
            iostat_ptr = "&(" + src + ")";
        }

        std::pair<std::string, std::string> end_arg = x.m_end
            ? visit_string_arg(x.m_end)
            : std::make_pair("\"\\n\"", "1");

        if (!x.m_is_formatted) {
            if (!is_integer_unit) {
                throw CodeGenError("C backend FileWrite currently supports unformatted writes only to integer units",
                    x.base.base.loc);
            }

            std::string code;
            if (x.m_pos) {
                this->visit_expr(*x.m_pos);
                std::string pos = src;
                code += indent + "_lfortran_file_seek(" + unit + ", (int64_t)(" + pos + "), " + iostat_ptr + ");\n";
            }

            auto emit_unformatted_chunk = [&](ASR::expr_t *expr) {
                this->visit_expr(*expr);
                std::string value = src;
                ASR::ttype_t *value_type = ASRUtils::expr_type(expr);
                ASR::ttype_t *past_type = ASRUtils::type_get_past_allocatable_pointer(value_type);

                if (ASRUtils::is_array(past_type)) {
                    ASR::ttype_t *element_type = ASRUtils::extract_type(past_type);
                    ASR::array_physical_typeType phys = ASRUtils::extract_physical_type(past_type);
                    std::string data_ptr = value;
                    if (phys == ASR::array_physical_typeType::DescriptorArray ||
                        phys == ASR::array_physical_typeType::PointerArray ||
                        phys == ASR::array_physical_typeType::UnboundedPointerArray) {
                        data_ptr += "->data";
                    }

                    std::string array_size;
                    if (ASRUtils::is_fixed_size_array(past_type)) {
                        array_size = std::to_string(ASRUtils::get_fixed_size_of_array(past_type));
                    } else {
                        ASR::dimension_t *m_dims = nullptr;
                        int n_dims = ASRUtils::extract_dimensions_from_ttype(past_type, m_dims);
                        std::string array_size_func = c_utils_functions->get_array_size();
                        array_size = "((int32_t) " + array_size_func + "(" + value + "->dims, " + std::to_string(n_dims) + "))";
                    }

                    if (ASRUtils::is_character(*element_type)) {
                        ASR::String_t *str_type = ASRUtils::get_string_type(expr);
                        std::string value_len = "strlen(" + data_ptr + ")";
                        if (str_type && str_type->m_len) {
                            this->visit_expr(*str_type->m_len);
                            value_len = src;
                        }
                        code += indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"\", 0, "
                            + "((int32_t)((" + array_size + ") * (" + value_len + "))), "
                            + "(void*)(" + data_ptr + "), -1);\n";
                    } else {
                        std::string elem_size = "sizeof(" + CUtils::get_c_type_from_ttype_t(element_type) + ")";
                        code += indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"\", 0, "
                            + "((int32_t)((" + array_size + ") * " + elem_size + ")), "
                            + "(void*)(" + data_ptr + "), -1);\n";
                    }
                    return;
                }

                if (ASRUtils::is_character(*past_type)) {
                    ASR::String_t *str_type = ASRUtils::get_string_type(expr);
                    std::string value_len = "strlen(" + value + ")";
                    if (str_type && str_type->m_len) {
                        this->visit_expr(*str_type->m_len);
                        value_len = src;
                    }
                    code += indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"\", 0, "
                        + "((int32_t)(" + value_len + ")), (void*)(" + value + "), -1);\n";
                    return;
                }

                if (ASR::is_a<ASR::Integer_t>(*past_type) ||
                    ASR::is_a<ASR::Real_t>(*past_type) ||
                    ASR::is_a<ASR::Complex_t>(*past_type) ||
                    ASR::is_a<ASR::Logical_t>(*past_type)) {
                    std::string c_type = CUtils::get_c_type_from_ttype_t(past_type);
                    std::string size_expr = "sizeof(" + c_type + ")";
                    code += indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"\", 0, "
                        + "((int32_t)" + size_expr + "), (void*)&((" + c_type + "){" + value + "}), -1);\n";
                    return;
                }

                throw CodeGenError("C backend FileWrite currently supports only scalar or array numeric/logical/character values for unformatted external writes",
                    expr->base.loc);
            };

            for (size_t i=0; i<x.n_values; i++) {
                emit_unformatted_chunk(x.m_values[i]);
            }
            src = code;
            return;
        }

        if (x.n_values != 1) {
            throw CodeGenError("C backend FileWrite currently supports formatted writes only for a single value to integer or character units",
                x.base.base.loc);
        }

        if (ASR::is_a<ASR::StringFormat_t>(*x.m_values[0])) {
            ASR::StringFormat_t *str_fmt = ASR::down_cast<ASR::StringFormat_t>(x.m_values[0]);
            bool has_non_character_arg = false;
            bool has_array_arg = false;
            for (size_t i=0; i<str_fmt->n_args; i++) {
                ASR::ttype_t *arg_type = ASRUtils::type_get_past_allocatable_pointer(
                    ASRUtils::expr_type(str_fmt->m_args[i]));
                if (ASRUtils::is_array(arg_type)) {
                    has_array_arg = true;
                    break;
                }
                if (!ASRUtils::is_character(*ASRUtils::extract_type(arg_type))) {
                    has_non_character_arg = true;
                }
            }
            if (str_fmt->m_kind == ASR::string_format_kindType::FormatFortran
                    && has_non_character_arg && !has_array_arg) {
                std::string format_value = "NULL";
                std::string format_len = "0";
                if (str_fmt->m_fmt) {
                    auto fmt_arg = visit_string_arg(str_fmt->m_fmt);
                    format_value = fmt_arg.first;
                    format_len = fmt_arg.second;
                }

                std::string serialization = serialize_fortran_format_args(str_fmt->m_args, str_fmt->n_args);
                std::vector<std::string> arg_ptrs;
                std::vector<std::string> string_lengths;
                for (size_t i=0; i<str_fmt->n_args; i++) {
                    ASR::expr_t *arg_expr = str_fmt->m_args[i];
                    ASR::ttype_t *arg_type = ASRUtils::expr_type(arg_expr);
                    ASR::ttype_t *past_type = ASRUtils::type_get_past_allocatable_pointer(arg_type);
                    if (ASRUtils::is_array(past_type)) {
                        throw CodeGenError("C backend FileWrite does not support Fortran formatted array writes yet",
                            arg_expr->base.loc);
                    }

                    this->visit_expr(*arg_expr);
                    std::string arg_value = src;
                    if (ASRUtils::is_character(*past_type)) {
                        ASR::String_t *str_type = ASRUtils::get_string_type(arg_expr);
                        if (!str_type || !str_type->m_len) {
                            string_lengths.push_back("(int64_t)strlen(" + arg_value + ")");
                        } else {
                            int64_t fixed_len = 0;
                            if (!ASRUtils::extract_value(str_type->m_len, fixed_len)) {
                                this->visit_expr(*str_type->m_len);
                                string_lengths.push_back("(int64_t)(" + src + ")");
                            }
                        }
                        arg_ptrs.push_back("&((char*){" + arg_value + "})");
                    } else {
                        std::string c_type = CUtils::get_c_type_from_ttype_t(past_type);
                        arg_ptrs.push_back("&((" + c_type + "){" + arg_value + "})");
                    }
                }

                std::string unique_suffix = std::to_string(counter);
                counter += 1;
                std::string size_name = "__lfortran_write_size_" + unique_suffix;
                std::string buffer_name = "__lfortran_write_buffer_" + unique_suffix;

                src = indent + "int64_t " + size_name + " = 0;\n";
                src += indent + "char *" + buffer_name
                    + " = _lcompilers_string_format_fortran(_lfortran_get_default_allocator(), "
                    + format_value + ", (int64_t)(" + format_len + "), \""
                    + CUtils::escape_c_string_literal(serialization) + "\", &"
                    + size_name + ", 0, " + std::to_string(string_lengths.size())
                    + ", 0, 0, 0";
                for (auto &length: string_lengths) {
                    src += ", " + length;
                }
                for (auto &arg: arg_ptrs) {
                    src += ", " + arg;
                }
                src += ");\n";
                if (is_string_unit) {
                    src += unit_setup;
                    src += indent + "_lfortran_string_write(_lfortran_get_default_allocator(), &(" + unit + "), "
                        + (is_unit_allocatable ? "true" : "false") + ", "
                        + (is_unit_deferred ? "true" : "false") + ", false, 1, &"
                        + unit_len_name + ", " + iostat_ptr + ", \"%s\", 2, "
                        + buffer_name + ", " + size_name + ");\n";
                } else {
                    src += indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"%s%s\", 4, "
                        + buffer_name + ", " + size_name + ", "
                        + end_arg.first + ", " + end_arg.second + ");\n";
                }
                src += indent + "_lfortran_free_alloc(_lfortran_get_default_allocator(), " + buffer_name + ");\n";
                return;
            }

            std::string snprintf_fmt = "\"";
            std::vector<std::string> fmt_args;
            bool has_hash_prefix = false;
            if (str_fmt->m_fmt) {
                ASR::expr_t *fmt_value = ASRUtils::expr_value(str_fmt->m_fmt);
                if (fmt_value && ASR::is_a<ASR::StringConstant_t>(*fmt_value)) {
                    std::string fmt_text = ASR::down_cast<ASR::StringConstant_t>(fmt_value)->m_s;
                    if (fmt_text == "(\"#\", *(1x, g0))") {
                        snprintf_fmt += "#";
                        has_hash_prefix = true;
                    }
                }
            }
            for (size_t i=0; i<str_fmt->n_args; i++) {
                this->visit_expr(*str_fmt->m_args[i]);
                std::string arg_value = src;
                ASR::ttype_t* value_type = ASRUtils::expr_type(str_fmt->m_args[i]);
                ASR::ttype_t* printable_type = ASRUtils::type_get_past_allocatable_pointer(value_type);
                if (ASR::is_a<ASR::List_t>(*value_type) || ASR::is_a<ASR::Tuple_t>(*value_type)) {
                    throw CodeGenError("C backend FileWrite does not support list/tuple values in StringFormat yet",
                        str_fmt->m_args[i]->base.loc);
                }
                if (ASRUtils::is_array(value_type)) {
                    printable_type = ASRUtils::extract_type(value_type);
                    int64_t fixed_size = ASRUtils::get_fixed_size_of_array(value_type);
                    if (fixed_size <= 0) fixed_size = 1;
                    for (int64_t j = 0; j < fixed_size; j++) {
                        if ((i != 0 || has_hash_prefix) || j != 0) {
                            snprintf_fmt += " ";
                        }
                        snprintf_fmt += c_ds_api->get_print_type(printable_type, false);
                        std::string elem_value = arg_value + "->data[" + arg_value + "->offset";
                        if (j != 0) {
                            elem_value += " + (" + arg_value + "->dims[0].stride * "
                                + std::to_string(j) + ")";
                        }
                        elem_value += "]";
                        if (ASR::is_a<ASR::Complex_t>(*printable_type)) {
                            fmt_args.push_back("creal(" + elem_value + ")");
                            fmt_args.push_back("cimag(" + elem_value + ")");
                        } else {
                            fmt_args.push_back(elem_value);
                        }
                    }
                } else {
                    if (i != 0 || has_hash_prefix) {
                        snprintf_fmt += " ";
                    }
                    snprintf_fmt += c_ds_api->get_print_type(printable_type,
                        ASR::is_a<ASR::ArrayItem_t>(*(str_fmt->m_args[i])));
                    if (ASR::is_a<ASR::Complex_t>(*printable_type)) {
                        fmt_args.push_back("creal(" + arg_value + ")");
                        fmt_args.push_back("cimag(" + arg_value + ")");
                    } else {
                        fmt_args.push_back(arg_value);
                    }
                }
            }
            snprintf_fmt += "\"";

            std::string unique_suffix = std::to_string(counter);
            counter += 1;
            std::string size_name = "__lfortran_write_size_" + unique_suffix;
            std::string buffer_name = "__lfortran_write_buffer_" + unique_suffix;

            src = indent + "int " + size_name + " = snprintf(NULL, 0, " + snprintf_fmt;
            for (auto &arg: fmt_args) {
                src += ", " + arg;
            }
            src += ");\n";
            src += indent + "char *" + buffer_name + " = (char*) malloc((size_t)" + size_name + " + 1);\n";
            src += indent + "snprintf(" + buffer_name + ", (size_t)" + size_name + " + 1, " + snprintf_fmt;
            for (auto &arg: fmt_args) {
                src += ", " + arg;
            }
            src += ");\n";
            if (is_string_unit) {
                src += unit_setup;
                src += indent + "_lfortran_string_write(_lfortran_get_default_allocator(), &(" + unit + "), "
                    + (is_unit_allocatable ? "true" : "false") + ", "
                    + (is_unit_deferred ? "true" : "false") + ", false, 1, &"
                    + unit_len_name + ", " + iostat_ptr + ", \"%s\", 2, "
                    + buffer_name + ", (int64_t)" + size_name + ");\n";
            } else {
                src += indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"%s%s\", 4, "
                    + buffer_name + ", (int64_t)" + size_name + ", "
                    + end_arg.first + ", " + end_arg.second + ");\n";
            }
            src += indent + "free(" + buffer_name + ");\n";
            return;
        }

        if (!ASRUtils::is_character(*ASRUtils::expr_type(x.m_values[0]))) {
            throw CodeGenError("C backend FileWrite currently supports only character or StringFormat values",
                x.base.base.loc);
        }

        auto value_arg = visit_string_arg(x.m_values[0]);
        if (is_string_unit) {
            src = unit_setup;
            src += indent + "_lfortran_string_write(_lfortran_get_default_allocator(), &(" + unit + "), "
                + (is_unit_allocatable ? "true" : "false") + ", "
                + (is_unit_deferred ? "true" : "false") + ", false, 1, &"
                + unit_len_name + ", " + iostat_ptr + ", \"%s\", 2, "
                + value_arg.first + ", " + value_arg.second + ");\n";
        } else {
            src = indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"%s%s\", 4, "
                + value_arg.first + ", " + value_arg.second + ", "
                + end_arg.first + ", " + end_arg.second + ");\n";
        }
    }

    void visit_ArrayBroadcast(const ASR::ArrayBroadcast_t &x) {
        /*
            !LF$ attributes simd :: A
            real :: A(8)
            A = 1
            We need to generate:
            a = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
        */
        size_t size = ASRUtils::get_fixed_size_of_array(x.m_type);
        std::string array_const_str = "{";
        for( size_t i = 0; i < size; i++ ) {
            this->visit_expr(*x.m_array);
            array_const_str += src;
            if (i < size - 1) array_const_str += ", ";
        }
        array_const_str += "}";
        src = array_const_str;
    }

    void visit_ArraySize(const ASR::ArraySize_t& x) {
        CHECK_FAST_C(compiler_options, x)
        visit_expr(*x.m_v);
        std::string var_name = src;
        std::string setup = drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(var_name);
        std::string args = "";
        std::string result_type = CUtils::get_c_type_from_ttype_t(x.m_type);
        ASR::expr_t *raw_array_expr = unwrap_c_lvalue_expr(x.m_v);
        bool inline_fixed_member_array = is_c && raw_array_expr != nullptr
            && ASR::is_a<ASR::StructInstanceMember_t>(*raw_array_expr)
            && ASRUtils::is_fixed_size_array(ASRUtils::expr_type(raw_array_expr));
        ASR::ttype_t *array_type = inline_fixed_member_array
            ? ASRUtils::expr_type(raw_array_expr) : ASRUtils::expr_type(x.m_v);
        ASR::dimension_t *m_dims = nullptr;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(array_type, m_dims);
        auto get_fixed_dim_expr = [&](size_t dim, bool want_length) -> std::string {
            ASR::expr_t *dim_expr = want_length ? m_dims[dim].m_length : m_dims[dim].m_start;
            if (dim_expr == nullptr) {
                return want_length ? "1" : "1";
            }
            visit_expr(*dim_expr);
            std::string dim_src = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(dim_src);
            return dim_src;
        };
        auto select_fixed_dim_expr = [&](bool want_length, const std::string &idx_expr) -> std::string {
            if (n_dims <= 1) {
                return get_fixed_dim_expr(0, want_length);
            }
            std::string expr = "((" + idx_expr + ") == " + std::to_string(n_dims)
                + " ? " + get_fixed_dim_expr(n_dims - 1, want_length) + " : 1)";
            for (int dim = n_dims - 2; dim >= 0; dim--) {
                expr = "((" + idx_expr + ") == " + std::to_string(dim + 1)
                    + " ? " + get_fixed_dim_expr(dim, want_length) + " : " + expr + ")";
            }
            return expr;
        };
        if (inline_fixed_member_array || (is_c && is_data_only_array_expr(x.m_v))) {
            if (x.m_dim == nullptr) {
                src = "((" + result_type + ") "
                    + std::to_string(ASRUtils::get_fixed_size_of_array(array_type)) + ")";
            } else {
                visit_expr(*x.m_dim);
                std::string idx = src;
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(idx);
                src = "((" + result_type + ")" + select_fixed_dim_expr(true, idx) + ")";
            }
            if (!setup.empty()) {
                tmp_buffer_src.push_back(setup);
            }
            return;
        }
        if (x.m_dim == nullptr) {
            std::string array_size_func = c_utils_functions->get_array_size();
            src = "((" + result_type + ") " + array_size_func + "(" + var_name + "->dims, " + std::to_string(n_dims) + "))";
        } else {
            visit_expr(*x.m_dim);
            std::string idx = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(idx);
            src = "((" + result_type + ")" + var_name + "->dims[" + idx + "-1].length)";
        }
        if (!setup.empty()) {
            tmp_buffer_src.push_back(setup);
        }
    }

    void visit_ArrayReshape(const ASR::ArrayReshape_t& x) {
        CHECK_FAST_C(compiler_options, x)
        visit_expr(*x.m_array);
        std::string array = src;
        visit_expr(*x.m_shape);
        std::string shape = src;

        ASR::ttype_t* array_type_asr = ASRUtils::expr_type(x.m_array);
        std::string array_type_name = CUtils::get_c_array_element_type_from_ttype_t(array_type_asr);
        std::string array_encoded_type_name = CUtils::get_c_array_type_code(array_type_asr, true, true);
        std::string array_type = c_ds_api->get_array_type(array_type_name, array_encoded_type_name, array_types_decls, true);
        ASR::ttype_t* return_type_asr = x.m_type;
        std::string return_type_name = CUtils::get_c_array_element_type_from_ttype_t(return_type_asr);
        std::string return_encoded_type_name = CUtils::get_c_array_type_code(return_type_asr, true, true);
        std::string return_type = c_ds_api->get_array_type(return_type_name, return_encoded_type_name, array_types_decls, false);

        ASR::ttype_t* shape_type_asr = ASRUtils::expr_type(x.m_shape);
        std::string shape_type_name = CUtils::get_c_array_element_type_from_ttype_t(shape_type_asr);
        std::string shape_encoded_type_name = CUtils::get_c_array_type_code(shape_type_asr, true, true);
        std::string shape_type = c_ds_api->get_array_type(shape_type_name, shape_encoded_type_name, array_types_decls, true);

        std::string array_reshape_func = c_utils_functions->get_array_reshape(array_type, shape_type,
            return_type, array_type_name, array_encoded_type_name);
        src = array_reshape_func + "(" + array + ", " + shape + ")";
    }

    void visit_ArrayBound(const ASR::ArrayBound_t& x) {
        CHECK_FAST_C(compiler_options, x)
        visit_expr(*x.m_v);
        std::string var_name = src;
        std::string setup = drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(var_name);
        std::string args = "";
        std::string result_type = CUtils::get_c_type_from_ttype_t(x.m_type);
        ASR::expr_t *raw_array_expr = unwrap_c_lvalue_expr(x.m_v);
        bool inline_fixed_member_array = is_c && raw_array_expr != nullptr
            && ASR::is_a<ASR::StructInstanceMember_t>(*raw_array_expr)
            && ASRUtils::is_fixed_size_array(ASRUtils::expr_type(raw_array_expr));
        ASR::ttype_t *array_type = inline_fixed_member_array
            ? ASRUtils::expr_type(raw_array_expr) : ASRUtils::expr_type(x.m_v);
        ASR::dimension_t *m_dims = nullptr;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(array_type, m_dims);
        auto get_fixed_dim_expr = [&](size_t dim, bool want_length) -> std::string {
            ASR::expr_t *dim_expr = want_length ? m_dims[dim].m_length : m_dims[dim].m_start;
            if (dim_expr == nullptr) {
                return want_length ? "1" : "1";
            }
            visit_expr(*dim_expr);
            std::string dim_src = src;
            setup += drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(dim_src);
            return dim_src;
        };
        auto select_fixed_dim_expr = [&](bool want_length, const std::string &idx_expr) -> std::string {
            if (n_dims <= 1) {
                return get_fixed_dim_expr(0, want_length);
            }
            std::string expr = "((" + idx_expr + ") == " + std::to_string(n_dims)
                + " ? " + get_fixed_dim_expr(n_dims - 1, want_length) + " : 1)";
            for (int dim = n_dims - 2; dim >= 0; dim--) {
                expr = "((" + idx_expr + ") == " + std::to_string(dim + 1)
                    + " ? " + get_fixed_dim_expr(dim, want_length) + " : " + expr + ")";
            }
            return expr;
        };
        visit_expr(*x.m_dim);
        std::string idx = src;
        setup += drain_tmp_buffer();
        setup += extract_stmt_setup_from_expr(idx);
        if (inline_fixed_member_array || (is_c && is_data_only_array_expr(x.m_v))) {
            std::string lower_bound = select_fixed_dim_expr(false, idx);
            if( x.m_bound == ASR::arrayboundType::LBound ) {
                src = "((" + result_type + ")" + lower_bound + ")";
            } else if( x.m_bound == ASR::arrayboundType::UBound ) {
                std::string length = select_fixed_dim_expr(true, idx);
                src = "((" + result_type + ")(" + length + " + " + lower_bound + " - 1))";
            }
            if (!setup.empty()) {
                tmp_buffer_src.push_back(setup);
            }
            return;
        }
        if( x.m_bound == ASR::arrayboundType::LBound ) {
            if (ASRUtils::is_simd_array(x.m_v)) {
                src = "0";
            } else {
                src = "((" + result_type + ")" + var_name + "->dims[" + idx + "-1].lower_bound)";
            }
        } else if( x.m_bound == ASR::arrayboundType::UBound ) {
            if (ASRUtils::is_simd_array(x.m_v)) {
                int64_t size = ASRUtils::get_fixed_size_of_array(ASRUtils::expr_type(x.m_v));
                src = std::to_string(size - 1);
            } else {
                std::string lower_bound = var_name + "->dims[" + idx + "-1].lower_bound";
                std::string length = var_name + "->dims[" + idx + "-1].length";
                std::string upper_bound = length + " + " + lower_bound + " - 1";
                src = "((" + result_type + ") " + upper_bound + ")";
            }
        }
        if (!setup.empty()) {
            tmp_buffer_src.push_back(setup);
        }
    }

    void visit_ArrayConstant(const ASR::ArrayConstant_t& x) {
        // TODO: Support and test for multi-dimensional array constants
        headers.insert("stdarg.h");
        std::string array_const = "";
        for( size_t i = 0; i < (size_t) ASRUtils::get_fixed_size_of_array(x.m_type); i++ ) {
            ASR::Array_t *array_type = ASR::down_cast<ASR::Array_t>(x.m_type);
            array_const += get_c_array_constant_init_element_for_c_index(
                const_cast<ASR::ArrayConstant_t*>(&x), x.m_type,
                array_type->m_type, i) + ", ";
        }
        array_const.pop_back();
        array_const.pop_back();

        ASR::ttype_t* array_type_asr = x.m_type;
        std::string array_type_name = CUtils::get_c_array_element_type_from_ttype_t(array_type_asr);
        std::string array_encoded_type_name = CUtils::get_c_array_type_code(array_type_asr, true, true);
        std::string return_type = c_ds_api->get_array_type(array_type_name, array_encoded_type_name,array_types_decls, false);

        src = c_utils_functions->get_array_constant(return_type, array_type_name, array_encoded_type_name) +
                "(" + std::to_string(ASRUtils::get_fixed_size_of_array(x.m_type)) + ", " + array_const + ")";
    }

    void visit_ArrayConstructor(const ASR::ArrayConstructor_t& x) {
        CHECK_FAST_C(compiler_options, x)
        headers.insert("stdarg.h");

        ASR::ttype_t* array_type_asr = x.m_type;
        ASR::ttype_t* element_type_asr = ASRUtils::type_get_past_array(array_type_asr);
        std::string array_type_name = CUtils::get_c_array_element_type_from_ttype_t(array_type_asr);
        std::string array_encoded_type_name = CUtils::get_c_array_type_code(array_type_asr, true, true);
        std::string return_type = c_ds_api->get_array_type(
            array_type_name, array_encoded_type_name, array_types_decls, false);

        bool has_implied_do = false;
        for (size_t i = 0; i < x.n_args; i++) {
            if (ASR::is_a<ASR::ImpliedDoLoop_t>(*x.m_args[i])) {
                has_implied_do = true;
                break;
            }
        }

        if (is_c && has_implied_do) {
            auto indent_at = [&](int level) -> std::string {
                return std::string(level * indentation_spaces, ' ');
            };
            auto visit_expr_at_level = [&](ASR::expr_t *expr, int level,
                                           std::string &setup, std::string &value) {
                int old_indentation_level = indentation_level;
                indentation_level = level;
                this->visit_expr(*expr);
                value = src;
                setup = drain_tmp_buffer();
                while (true) {
                    std::string extracted = extract_stmt_setup_from_expr(value);
                    if (!extracted.empty()) {
                        setup += extracted;
                    }
                    std::string deferred = drain_tmp_buffer();
                    if (deferred.empty()) {
                        break;
                    }
                    setup += deferred;
                }
                if (looks_like_non_expression_stmt(value)) {
                    std::string recovered_expr;
                    bool recovered_from_setup =
                        extract_trailing_stmt_expr_from_setup(setup, recovered_expr);
                    if (!value.empty()) {
                        setup += value;
                        if (!endswith(value, "\n")) {
                            setup += "\n";
                        }
                    }
                    value = recovered_expr;
                    if (!recovered_from_setup) {
                        value.clear();
                        extract_trailing_stmt_expr_from_setup(setup, value);
                    }
                }
                indentation_level = old_indentation_level;
            };
            auto try_emit_scalar_count_expr_at_level = [&](ASR::expr_t *expr, int level,
                                                           std::string &setup,
                                                           std::string &value) -> bool {
                if (!is_c) {
                    return false;
                }
                ASR::expr_t *mask_expr = nullptr;
                ASR::ttype_t *result_type_asr = nullptr;
                if (ASR::is_a<ASR::IntrinsicArrayFunction_t>(*expr)) {
                    ASR::IntrinsicArrayFunction_t *count_expr =
                        ASR::down_cast<ASR::IntrinsicArrayFunction_t>(expr);
                    if (count_expr->m_arr_intrinsic_id
                            != static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Count)
                            || count_expr->n_args == 0
                            || ASRUtils::is_array(count_expr->m_type)) {
                        return false;
                    }
                    mask_expr = count_expr->m_args[0];
                    result_type_asr = count_expr->m_type;
                } else if (ASR::is_a<ASR::FunctionCall_t>(*expr)) {
                    ASR::FunctionCall_t *call_expr = ASR::down_cast<ASR::FunctionCall_t>(expr);
                    if (call_expr->n_args == 0 || ASRUtils::is_array(call_expr->m_type)) {
                        return false;
                    }
                    std::string callee_name = ASRUtils::symbol_name(
                        ASRUtils::symbol_get_past_external(call_expr->m_name));
                    ASR::Function_t *fn = get_procedure_interface_function(call_expr->m_name);
                    if (fn != nullptr) {
                        callee_name = fn->m_name;
                    }
                    if (callee_name.find("count") == std::string::npos
                            && callee_name.find("_lcompilers_count") == std::string::npos) {
                        return false;
                    }
                    mask_expr = call_expr->m_args[0].m_value;
                    result_type_asr = call_expr->m_type;
                } else {
                    return false;
                }
                ASR::ttype_t *mask_type = ASRUtils::expr_type(mask_expr);
                mask_type = ASRUtils::type_get_past_allocatable_pointer(mask_type);
                if (!ASRUtils::is_array(mask_type)) {
                    return false;
                }
                ASR::ttype_t *mask_element_type = ASRUtils::type_get_past_array(mask_type);
                if (mask_element_type == nullptr) {
                    return false;
                }
                mask_element_type =
                    ASRUtils::type_get_past_allocatable_pointer(mask_element_type);
                if (!ASRUtils::is_logical(*mask_element_type)) {
                    return false;
                }
                int mask_n_dims = ASRUtils::extract_n_dims_from_ttype(mask_type);
                if (mask_n_dims > 1) {
                    return false;
                }

                std::string mask_setup;
                std::string mask_value;
                visit_expr_at_level(mask_expr, level, mask_setup, mask_value);
                if (mask_value.empty()) {
                    extract_trailing_stmt_expr_from_setup(mask_setup, mask_value);
                }
                if (mask_value.empty()) {
                    return false;
                }

                std::string result_type = CUtils::get_c_type_from_ttype_t(result_type_asr);
                std::string count_name =
                    get_unique_local_name("__libasr_created__count_value");
                std::string index_name =
                    get_unique_local_name("__libasr_created__count_index");
                std::string level_indent = indent_at(level);
                std::string inner_indent = indent_at(level + 1);
                std::string elem_index = "((0 + (" + mask_value + "->dims[0].stride * ("
                    + index_name + " - " + mask_value + "->dims[0].lower_bound))) + "
                    + mask_value + "->offset)";

                setup = mask_setup;
                setup += level_indent + result_type + " " + count_name + " = 0;\n";
                setup += level_indent + "for (int32_t " + index_name + " = "
                    + mask_value + "->dims[0].lower_bound; " + index_name + " <= "
                    + mask_value + "->dims[0].length + " + mask_value
                    + "->dims[0].lower_bound - 1; " + index_name + "++) {\n";
                setup += inner_indent + "if (" + mask_value + "->data[" + elem_index + "]) {\n";
                setup += indent_at(level + 2) + count_name + " += 1;\n";
                setup += inner_indent + "}\n";
                setup += level_indent + "}\n";
                value = count_name;
                return true;
            };

            std::string array_var_name =
                get_unique_local_name("__libasr_created__array_constructor");
            std::string array_value_name = array_var_name + "_value";
            std::string count_name = array_var_name + "__count";
            std::string index_name = array_var_name + "__index";
            bool element_needs_null_init =
                ASRUtils::is_character(*ASRUtils::type_get_past_allocatable_pointer(element_type_asr));

            std::function<void(ASR::expr_t **, size_t, int, std::string&)> emit_count;
            std::function<void(ASR::expr_t **, size_t, int, std::string&)> emit_fill;

            emit_count = [&](ASR::expr_t **values, size_t n_values, int level, std::string &code) {
                for (size_t i = 0; i < n_values; i++) {
                    ASR::expr_t *value_expr = values[i];
                    if (ASR::is_a<ASR::ImpliedDoLoop_t>(*value_expr)) {
                        ASR::ImpliedDoLoop_t *idl = ASR::down_cast<ASR::ImpliedDoLoop_t>(value_expr);
                        std::string loop_setup, loop_var;
                        visit_expr_at_level(idl->m_var, level, loop_setup, loop_var);
                        std::string start_setup, loop_start;
                        visit_expr_at_level(idl->m_start, level, start_setup, loop_start);
                        std::string end_setup, loop_end;
                        visit_expr_at_level(idl->m_end, level, end_setup, loop_end);
                        std::string inc_setup, loop_inc = "1";
                        if (idl->m_increment != nullptr) {
                            visit_expr_at_level(idl->m_increment, level, inc_setup, loop_inc);
                        }
                        std::string save_name = get_unique_local_name(loop_var + "__saved");
                        std::string loop_var_type = CUtils::get_c_type_from_ttype_t(
                            ASRUtils::expr_type(idl->m_var));
                        code += loop_setup + start_setup + end_setup + inc_setup;
                        code += indent_at(level) + loop_var_type + " " + save_name
                            + " = " + loop_var + ";\n";
                        code += indent_at(level) + "for (" + loop_var + " = " + loop_start + "; "
                            + "((" + loop_inc + ") >= 0 ? (" + loop_var + " <= " + loop_end + ") : ("
                            + loop_var + " >= " + loop_end + ")); "
                            + loop_var + " += " + loop_inc + ") {\n";
                        emit_count(idl->m_values, idl->n_values, level + 1, code);
                        code += indent_at(level) + "}\n";
                        code += indent_at(level) + loop_var + " = " + save_name + ";\n";
                    } else {
                        code += indent_at(level) + count_name + " += 1;\n";
                    }
                }
            };

            emit_fill = [&](ASR::expr_t **values, size_t n_values, int level, std::string &code) {
                for (size_t i = 0; i < n_values; i++) {
                    ASR::expr_t *value_expr = values[i];
                    if (ASR::is_a<ASR::ImpliedDoLoop_t>(*value_expr)) {
                        ASR::ImpliedDoLoop_t *idl = ASR::down_cast<ASR::ImpliedDoLoop_t>(value_expr);
                        std::string loop_setup, loop_var;
                        visit_expr_at_level(idl->m_var, level, loop_setup, loop_var);
                        std::string start_setup, loop_start;
                        visit_expr_at_level(idl->m_start, level, start_setup, loop_start);
                        std::string end_setup, loop_end;
                        visit_expr_at_level(idl->m_end, level, end_setup, loop_end);
                        std::string inc_setup, loop_inc = "1";
                        if (idl->m_increment != nullptr) {
                            visit_expr_at_level(idl->m_increment, level, inc_setup, loop_inc);
                        }
                        std::string save_name = get_unique_local_name(loop_var + "__saved");
                        std::string loop_var_type = CUtils::get_c_type_from_ttype_t(
                            ASRUtils::expr_type(idl->m_var));
                        code += loop_setup + start_setup + end_setup + inc_setup;
                        code += indent_at(level) + loop_var_type + " " + save_name
                            + " = " + loop_var + ";\n";
                        code += indent_at(level) + "for (" + loop_var + " = " + loop_start + "; "
                            + "((" + loop_inc + ") >= 0 ? (" + loop_var + " <= " + loop_end + ") : ("
                            + loop_var + " >= " + loop_end + ")); "
                            + loop_var + " += " + loop_inc + ") {\n";
                        emit_fill(idl->m_values, idl->n_values, level + 1, code);
                        code += indent_at(level) + "}\n";
                        code += indent_at(level) + loop_var + " = " + save_name + ";\n";
                    } else {
                        std::string value_setup, value_src;
                        if (!try_emit_scalar_count_expr_at_level(
                                value_expr, level, value_setup, value_src)) {
                            visit_expr_at_level(value_expr, level, value_setup, value_src);
                            if (looks_like_non_expression_stmt(value_src)
                                    && value_setup.find("__lfortran__lcompilers_count")
                                        != std::string::npos) {
                                std::string recovered_value;
                                if (extract_trailing_stmt_expr_from_setup(
                                            value_setup, recovered_value)) {
                                    if (!value_src.empty()) {
                                        value_setup += value_src;
                                        if (!endswith(value_src, "\n")) {
                                            value_setup += "\n";
                                        }
                                    }
                                    value_src = recovered_value;
                                }
                            }
                        }
                        code += value_setup;
                        if (element_needs_null_init) {
                            code += indent_at(level) + array_var_name + "->data[" + index_name + "] = NULL;\n";
                        }
                        code += indent_at(level)
                            + c_ds_api->get_deepcopy(
                                element_type_asr, value_src,
                                array_var_name + "->data[" + index_name + "]") + "\n";
                        code += indent_at(level) + index_name + " += 1;\n";
                    }
                }
            };

            std::string array_setup;
            int base_level = indentation_level;
            std::string base_indent = indent_at(base_level);
            array_setup += base_indent + return_type + " " + array_value_name + ";\n";
            array_setup += base_indent + return_type + "* " + array_var_name
                + " = &" + array_value_name + ";\n";
            array_setup += base_indent + "int32_t " + count_name + " = 0;\n";
            emit_count(x.m_args, x.n_args, base_level, array_setup);
            array_setup += base_indent + array_var_name + "->data = (" + array_type_name
                + "*) _lfortran_malloc_alloc(_lfortran_get_default_allocator(), sizeof("
                + array_type_name + ") * " + count_name + ");\n";
            array_setup += base_indent + array_var_name + "->n_dims = 1;\n";
            array_setup += base_indent + array_var_name + "->offset = 0;\n";
            array_setup += base_indent + array_var_name + "->is_allocated = true;\n";
            array_setup += base_indent + array_var_name + "->dims[0].lower_bound = "
                + std::to_string(lower_bound) + ";\n";
            array_setup += base_indent + array_var_name + "->dims[0].length = " + count_name + ";\n";
            array_setup += base_indent + array_var_name + "->dims[0].stride = 1;\n";
            array_setup += base_indent + "int32_t " + index_name + " = 0;\n";
            emit_fill(x.m_args, x.n_args, base_level, array_setup);

            src = array_var_name;
            tmp_buffer_src.push_back(array_setup);
            return;
        }

        std::string array_ctor = "";
        for (size_t i = 0; i < x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            array_ctor += src;
            if (i + 1 < x.n_args) {
                array_ctor += ", ";
            }
        }

        src = c_utils_functions->get_array_constant(return_type, array_type_name, array_encoded_type_name)
            + "(" + std::to_string(x.n_args);
        if (x.n_args > 0) {
            src += ", " + array_ctor;
        }
        src += ")";
    }

    void visit_ArrayItem(const ASR::ArrayItem_t &x) {
        CHECK_FAST_C(compiler_options, x)
        auto get_scalar_index_src = [&](ASR::expr_t *idx_expr) {
            this->visit_expr(*idx_expr);
            std::string idx_src = src;
            if (!ASRUtils::is_array(ASRUtils::expr_type(idx_expr))) {
                return idx_src;
            }
            ASR::expr_t *unwrapped_idx = ASRUtils::expr_value(idx_expr);
            if (unwrapped_idx == nullptr) {
                unwrapped_idx = idx_expr;
            }
            if (ASR::is_a<ASR::ArrayConstant_t>(*unwrapped_idx)) {
                return idx_src + "->data[" + idx_src + "->offset]";
            }
            if (is_data_only_array_expr(unwrapped_idx)) {
                return idx_src + "[0]";
            }
            return idx_src + "->data[" + idx_src + "->offset]";
        };
        auto get_dim_start_src = [&](const ASR::dimension_t &dim) {
            if (dim.m_start) {
                this->visit_expr(*dim.m_start);
                return src;
            }
            return std::to_string(lower_bound);
        };
        auto get_dim_length_src = [&](const ASR::dimension_t &dim) {
            if (dim.m_length) {
                this->visit_expr(*dim.m_length);
                return src;
            }
            return std::string("1");
        };
        ASR::expr_t *array_expr = x.m_v;
        ASR::ArraySection_t *array_section = nullptr;
        if (ASR::is_a<ASR::ArraySection_t>(*x.m_v)) {
            array_section = ASR::down_cast<ASR::ArraySection_t>(x.m_v);
            array_expr = array_section->m_v;
        }
        this->visit_expr(*array_expr);
        std::string array = src;
        ASR::ttype_t* x_mv_type = ASRUtils::expr_type(array_expr);
        ASR::dimension_t* m_dims;
        int n_dims = ASRUtils::extract_dimensions_from_ttype(x_mv_type, m_dims);
        ASR::symbol_t *array_owner = ASRUtils::get_asr_owner(array_expr);
        bool is_raw_array_constant = ASR::is_a<ASR::ArrayConstant_t>(*array_expr);
        bool is_data_only_array = ASRUtils::is_fixed_size_array(m_dims, n_dims) &&
                                  array_owner != nullptr &&
                                  ASR::is_a<ASR::Struct_t>(*array_owner);
        if( is_data_only_array || is_raw_array_constant || ASRUtils::is_simd_array(x.m_v)) {
            std::string index = "";
            std::string out = array;
            if (is_raw_array_constant) {
                out += "->data[";
            } else {
                out += "[";
            }
            for (size_t i=0; i<x.n_args; i++) {
                if (x.m_args[i].m_right) {
                    src = get_scalar_index_src(x.m_args[i].m_right);
                } else {
                    src = "/* FIXME right index */";
                }

                if (ASRUtils::is_simd_array(x.m_v)) {
                    index += src;
                } else {
                    std::string current_index = "";
                    if (is_data_only_array) {
                        std::string index_src = src;
                        std::string dim_start = get_dim_start_src(m_dims[i]);
                        current_index += "((" + index_src + ") - (" + dim_start + "))";
                    } else {
                        current_index += src;
                    }
                    for( size_t j = 0; j < i; j++ ) {
                        int64_t dim_size = 0;
                        ASRUtils::extract_value(m_dims[j].m_length, dim_size);
                        std::string length = std::to_string(dim_size);
                        current_index += " * " + length;
                    }
                    index += current_index;
                }
                if (i < x.n_args - 1) {
                    index += " + ";
                }
            }
            out += index + "]";
            last_expr_precedence = 2;
            src = out;
            return;
        }

        std::vector<std::string> indices;
        if (array_section == nullptr) {
            for( size_t r = 0; r < x.n_args; r++ ) {
                ASR::array_index_t curr_idx = x.m_args[r];
                indices.push_back(get_scalar_index_src(curr_idx.m_right));
            }
        } else {
            ASR::dimension_t *section_dims = nullptr;
            int section_rank = ASRUtils::extract_dimensions_from_ttype(array_section->m_type, section_dims);
            size_t sliced_dim = 0;
            for (size_t r = 0; r < array_section->n_args; r++) {
                ASR::array_index_t curr_idx = array_section->m_args[r];
                bool is_sliced_dim = curr_idx.m_left || curr_idx.m_step || curr_idx.m_right == nullptr;
                if (!is_sliced_dim) {
                    indices.push_back(get_scalar_index_src(curr_idx.m_right));
                    continue;
                }
                LCOMPILERS_ASSERT(sliced_dim < x.n_args);
                LCOMPILERS_ASSERT((int)sliced_dim < section_rank);
                std::string item_idx = get_scalar_index_src(x.m_args[sliced_dim].m_right);

                std::string section_lb = "1";
                if (section_dims && section_dims[sliced_dim].m_start) {
                    this->visit_expr(*section_dims[sliced_dim].m_start);
                    section_lb = src;
                }

                std::string base_lb;
                if (curr_idx.m_left) {
                    this->visit_expr(*curr_idx.m_left);
                    base_lb = src;
                } else {
                    base_lb = get_dim_start_src(m_dims[r]);
                }

                std::string step = "1";
                if (curr_idx.m_step) {
                    this->visit_expr(*curr_idx.m_step);
                    step = src;
                }

                indices.push_back("(" + base_lb + " + ((" + item_idx + ") - (" +
                    section_lb + ")) * (" + step + "))");
                sliced_dim++;
            }
        }

        ASR::ttype_t* x_mv_type_ = ASRUtils::type_get_past_allocatable(
                ASRUtils::type_get_past_pointer(x_mv_type));
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Array_t>(*x_mv_type_));
        ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(x_mv_type_);
        std::vector<std::string> diminfo;
        bool use_pointer_data_only_indexing =
            array_t->m_physical_type == ASR::array_physical_typeType::PointerArray
            && is_data_only_array_expr(array_expr);
        bool use_fixed_size_data_only_indexing =
            array_t->m_physical_type == ASR::array_physical_typeType::FixedSizeArray
            && (is_fixed_size_array_storage_expr(array_expr)
                || is_c_fixed_size_descriptor_storage_expr(array_expr));
        std::string fixed_size_data_name;
        if (use_fixed_size_data_only_indexing
                && is_c_local_fixed_size_descriptor_storage_expr(array_expr)) {
            fixed_size_data_name = array + "_data";
        }
        if( use_pointer_data_only_indexing ||
                use_fixed_size_data_only_indexing ) {
            for( size_t idim = 0; idim < x.n_args; idim++ ) {
                diminfo.push_back(get_dim_start_src(m_dims[idim]));
                diminfo.push_back(get_dim_length_src(m_dims[idim]));
            }
        } else if( array_t->m_physical_type == ASR::array_physical_typeType::UnboundedPointerArray ) {
            for( size_t idim = 0; idim < x.n_args; idim++ ) {
                diminfo.push_back(get_dim_start_src(m_dims[idim]));
                diminfo.push_back(get_dim_length_src(m_dims[idim]));
            }
        }

        LCOMPILERS_ASSERT(ASRUtils::extract_n_dims_from_ttype(x_mv_type) > 0);
        if (array_t->m_physical_type == ASR::array_physical_typeType::UnboundedPointerArray) {
            src = arr_get_single_element(array, indices, x.n_args,
                                                true,
                                                false,
                                                diminfo,
                                                true);
        } else {
            src = arr_get_single_element(array, indices, x.n_args,
                                                use_pointer_data_only_indexing,
                                                use_fixed_size_data_only_indexing,
                                                diminfo, false,
                                                fixed_size_data_name);
        }
        last_expr_precedence = 2;
    }

    void visit_ArraySection(const ASR::ArraySection_t &x) {
        CHECK_FAST_C(compiler_options, x)
        ASR::expr_t *raw_base_expr = unwrap_c_lvalue_expr(x.m_v);
        bool base_is_array_constant = raw_base_expr != nullptr
            && ASR::is_a<ASR::ArrayConstant_t>(*raw_base_expr);
        if (is_c && x.n_args == 1
                && (is_data_only_array_expr(x.m_v)
                    || is_fixed_size_array_storage_expr(x.m_v))
                && !base_is_array_constant) {
            this->visit_expr(*x.m_v);
            std::string base_expr = src;
            std::string setup = drain_tmp_buffer();
            setup += extract_stmt_setup_from_expr(base_expr);

            ASR::ttype_t *base_type = ASRUtils::expr_type(x.m_v);
            ASR::dimension_t *base_dims = nullptr;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(base_type, base_dims);
            LCOMPILERS_ASSERT(n_dims == 1);

            auto get_dim_expr = [&](ASR::expr_t *expr, const std::string &fallback) -> std::string {
                if (expr == nullptr) {
                    return fallback;
                }
                this->visit_expr(*expr);
                std::string expr_src = src;
                setup += drain_tmp_buffer();
                setup += extract_stmt_setup_from_expr(expr_src);
                return expr_src;
            };

            std::string base_lb = get_dim_expr(base_dims[0].m_start, "1");
            std::string base_len = get_dim_expr(base_dims[0].m_length, "1");
            std::string section_lb = get_dim_expr(x.m_args[0].m_left, base_lb);
            std::string section_ub = get_dim_expr(
                x.m_args[0].m_right, "(" + base_lb + " + " + base_len + " - 1)");
            std::string section_step = get_dim_expr(x.m_args[0].m_step, "1");

            std::string wrapper_type = get_c_declared_array_wrapper_type_name(x.m_type);
            src = "(&(" + wrapper_type + "){ .data = (" + base_expr + " + ((" + section_lb
                + ") - (" + base_lb + "))), .dims = {{1, ((((" + section_ub + ") - ("
                + section_lb + ")) / (" + section_step + ")) + 1), " + section_step
                + "}}, .n_dims = 1, .offset = 0, .is_allocated = true })";
            if (!setup.empty()) {
                tmp_buffer_src.push_back(setup);
            }
            last_expr_precedence = 2;
            return;
        }
        this->visit_expr(*x.m_v);
        last_expr_precedence = 2;
    }

    void visit_StringItem(const ASR::StringItem_t& x) {
        CHECK_FAST_C(compiler_options, x)
        this->visit_expr(*x.m_idx);
        std::string idx = std::move(src);
        this->visit_expr(*x.m_arg);
        std::string str = std::move(src);
        src = "_lfortran_str_item(" + str + ", strlen(" + str + "), "
            + idx + ", (char[2]){0})";
    }

    void visit_StringPhysicalCast(const ASR::StringPhysicalCast_t &x) {
        this->visit_expr(*x.m_arg);
    }

    std::string generate_map_clauses(ASR::OMPMap_t* m) {
        std::string result = " map(";
        
        std::string map_type;
        if (m->m_type == ASR::map_typeType::To) {
            map_type = "to";
        } else if (m->m_type == ASR::map_typeType::From) {
            map_type = "from";
        } else if (m->m_type == ASR::map_typeType::ToFrom) {
            map_type = "tofrom";
        } else if (m->m_type == ASR::map_typeType::Alloc) {
            map_type = "alloc";
        } else if (m->m_type == ASR::map_typeType::Release) {
            map_type = "release";
        } else if (m->m_type == ASR::map_typeType::Delete) {
            map_type = "delete";
        }
        
        std::vector<std::string> all_mappings;
        
        for (size_t j = 0; j < m->n_vars; j++) {
            if (m->m_vars[j]->type == ASR::exprType::Var) {
                ASR::Variable_t* var = ASRUtils::EXPR2VAR(m->m_vars[j]);
                std::string var_name;
                visit_expr(*m->m_vars[j]);
                var_name = src;
                
                if (is_allocatable_array(var)) {
                    std::vector<std::string> struct_mappings = generate_struct_mappings(var_name, var, map_type);
                    all_mappings.insert(all_mappings.end(), struct_mappings.begin(), struct_mappings.end());
                } else if (ASRUtils::is_array(var->m_type)) {
                    all_mappings.push_back(generate_array_mapping(var_name, var, map_type));
                } else {
                    all_mappings.push_back(map_type + ": " + var_name);
                }
            } else {
                throw CodeGenError("Unsupported OpenMP map variable type: " +
                    std::to_string((int)m->m_vars[j]->type));
            }
        }
        
        if (all_mappings.size() == 1) {
            result += all_mappings[0] + ")";
        } else {
            result = "";
            for (size_t i = 0; i < all_mappings.size(); i++) {
                result += " map(" + all_mappings[i] + ")";
            }
        }
        
        return result;
    }

    bool is_allocatable_array(ASR::Variable_t* var) {
        return ASR::is_a<ASR::Allocatable_t>(*var->m_type) || 
            (ASRUtils::is_array(var->m_type) && 
                ASR::down_cast<ASR::Array_t>(ASRUtils::type_get_past_allocatable(var->m_type))->m_physical_type == ASR::array_physical_typeType::DescriptorArray);
    }

    std::vector<std::string> generate_struct_mappings(const std::string& var_name, ASR::Variable_t* var, const std::string& base_map_type) {
        std::vector<std::string> mappings;
        
        ASR::ttype_t* type = ASRUtils::type_get_past_allocatable(var->m_type);
        if (ASRUtils::is_array(type)) {
            ASR::Array_t* arr_type = ASR::down_cast<ASR::Array_t>(type);
            
            // 1. Map the data array with proper slice notation
            std::string data_mapping;
            if (base_map_type == "to") {
                data_mapping = "to: " + var_name + "->data[0:";
            } else if (base_map_type == "from") {
                data_mapping = "from: " + var_name + "->data[0:";
            } else {
                data_mapping = "tofrom: " + var_name + "->data[0:";
            }
            
            if (arr_type->n_dims == 1) {
                // For 1D arrays, we need to determine the size at runtime
                data_mapping += var_name + "->dims[0].length-1]";
            } else {
                // Multi-dimensional arrays need more complex slice calculation
                std::string total_size = var_name + "->dims[0].length";
                for (size_t i = 1; i < arr_type->n_dims; i++) {
                    total_size += "*" + var_name + "->dims[" + std::to_string(i) + "].length";
                }
                data_mapping += total_size + "-1]";
            }
            mappings.push_back(data_mapping);
            
            // 2. Map dimension descriptors (usually 'to' since they're metadata)
            for (size_t i = 0; i < arr_type->n_dims; i++) {
                std::string dim_mapping = "to: " + var_name + "->dims[" + std::to_string(i) + "].lower_bound, " +
                                        var_name + "->dims[" + std::to_string(i) + "].length, " +
                                        var_name + "->dims[" + std::to_string(i) + "].stride";
                mappings.push_back(dim_mapping);
            }
            
            // 3. Map other struct members as needed
            mappings.push_back("to: " + var_name + "->n_dims");
            
            // Map offset - use 'from' if base is 'from' or 'tofrom', otherwise 'to'
            if (base_map_type == "from" || base_map_type == "tofrom") {
                mappings.push_back("from: " + var_name + "->offset");
            } else {
                mappings.push_back("to: " + var_name + "->offset");
            }
        }
        
        return mappings;
    }

    std::string generate_array_mapping(const std::string& var_name, ASR::Variable_t* var, const std::string& map_type) {
        ASR::Array_t* arr_type = ASR::down_cast<ASR::Array_t>(var->m_type);
        
        std::string mapping = map_type + ": " + var_name+ "->data";
        
        if (arr_type->n_dims == 1 && arr_type->m_dims[0].m_start && arr_type->m_dims[0].m_length) {
            mapping += "[";
            visit_expr(*arr_type->m_dims[0].m_start);
            std::string start = src;
            visit_expr(*arr_type->m_dims[0].m_length);
            std::string length = src;
            mapping += start + ":" + length + "]";
        }
        
        return mapping;
    }
    void visit_StringLen(const ASR::StringLen_t &x) {
        CHECK_FAST_C(compiler_options, x)
        this->visit_expr(*x.m_arg);
        std::string arg = src;
        src = "((" + arg + ") != NULL ? strlen(" + arg + ") : 0)";
    }

    void visit_OMPRegion(const ASR::OMPRegion_t &x) {
        if (!target_offload_enabled) {
            // Codegen for --show-c: Generate OpenMP pragmas
            std::string opening_pragma;
            if (x.m_region == ASR::omp_region_typeType::Parallel) {
                opening_pragma = "#pragma omp parallel ";
            } else if (x.m_region == ASR::omp_region_typeType::Do) {
                opening_pragma = "#pragma omp for ";
            } else if (x.m_region == ASR::omp_region_typeType::Sections) {
                opening_pragma = "#pragma omp sections ";
            } else if (x.m_region == ASR::omp_region_typeType::Single) {
                opening_pragma = "#pragma omp single ";
            } else if (x.m_region == ASR::omp_region_typeType::Critical) {
                opening_pragma = "#pragma omp critical ";
            } else if (x.m_region == ASR::omp_region_typeType::Atomic) {
                opening_pragma = "#pragma omp atomic ";
            } else if (x.m_region == ASR::omp_region_typeType::Barrier) {
                opening_pragma = "#pragma omp barrier ";
            } else if (x.m_region == ASR::omp_region_typeType::Task) {
                opening_pragma = "#pragma omp task ";
            } else if (x.m_region == ASR::omp_region_typeType::Taskwait) {
                opening_pragma = "#pragma omp taskwait ";
            } else if (x.m_region == ASR::omp_region_typeType::Master) {
                opening_pragma = "#pragma omp master ";
            } else if (x.m_region == ASR::omp_region_typeType::ParallelDo) {
                opening_pragma = "#pragma omp parallel for ";
            } else if (x.m_region == ASR::omp_region_typeType::ParallelSections) {
                opening_pragma = "#pragma omp parallel sections ";
            } else if (x.m_region == ASR::omp_region_typeType::Taskloop) {
                opening_pragma = "#pragma omp taskloop ";
            } else if (x.m_region == ASR::omp_region_typeType::Target) {
                opening_pragma = "#pragma omp target ";
            } else if (x.m_region == ASR::omp_region_typeType::Teams) {
                opening_pragma = "#pragma omp teams ";
            } else if (x.m_region == ASR::omp_region_typeType::DistributeParallelDo) {
                opening_pragma = "#pragma omp distribute parallel for ";
            } else if (x.m_region == ASR::omp_region_typeType::Distribute) {
                opening_pragma = "#pragma omp distribute ";
            } else {
                throw CodeGenError("Unsupported OpenMP region type: " + std::to_string((int)x.m_region));
            }

            std::string clauses;
            for(size_t i=0;i<x.n_clauses;i++) {
                ASR::omp_clause_t* clause = x.m_clauses[i];
                if (ASR::is_a<ASR::OMPPrivate_t>(*clause)) {
                    ASR::OMPPrivate_t* c = ASR::down_cast<ASR::OMPPrivate_t>(clause);
                    clauses += " private(";
                    std::string vars;
                    for (size_t j=0; j<c->n_vars; j++) {
                        visit_expr(*c->m_vars[j]);
                        vars += src;
                        if (j < c->n_vars - 1) {
                            vars += ", ";
                        }
                    }
                    clauses += vars + ")";
                } else if (ASR::is_a<ASR::OMPShared_t>(*clause)) {
                    ASR::OMPShared_t* c = ASR::down_cast<ASR::OMPShared_t>(clause);
                    clauses += " shared(";
                    std::string vars;
                    for (size_t j=0; j<c->n_vars; j++) {
                        visit_expr(*c->m_vars[j]);
                        vars += src;
                        if (j < c->n_vars - 1) {
                            vars += ", ";
                        }
                    }
                    clauses += vars + ")";
                } else if (ASR::is_a<ASR::OMPNumTeams_t>(*clause)) {
                    ASR::OMPNumTeams_t* c = ASR::down_cast<ASR::OMPNumTeams_t>(clause);
                    clauses += " num_teams(";
                    visit_expr(*c->m_num_teams);
                    clauses += src + ")";
                } else if (ASR::is_a<ASR::OMPThreadLimit_t>(*clause)) {
                    ASR::OMPThreadLimit_t* c = ASR::down_cast<ASR::OMPThreadLimit_t>(clause);
                    clauses += " thread_limit(";
                    visit_expr(*c->m_thread_limit);
                    clauses += src + ")";
                } else if (ASR::is_a<ASR::OMPSchedule_t>(*clause)) {
                    ASR::OMPSchedule_t* c = ASR::down_cast<ASR::OMPSchedule_t>(clause);
                    clauses += " schedule(";
                    if (c->m_kind == ASR::schedule_typeType::Static) {
                        clauses += "static";
                    } else if (c->m_kind == ASR::schedule_typeType::Dynamic) {
                        clauses += "dynamic";
                    } else if (c->m_kind == ASR::schedule_typeType::Guided) {
                        clauses += "guided";
                    } else if (c->m_kind == ASR::schedule_typeType::Auto) {
                        clauses += "auto";
                    } else if (c->m_kind == ASR::schedule_typeType::Runtime) {
                        clauses += "runtime";
                    }
                    if (c->m_chunk_size) {
                        clauses += ", ";
                        visit_expr(*c->m_chunk_size);
                        clauses += src;
                    }
                    clauses += ")";
                } else if (ASR::is_a<ASR::OMPReduction_t>(*clause)) {
                    ASR::OMPReduction_t* c = ASR::down_cast<ASR::OMPReduction_t>(clause);
                    clauses += " reduction(";
                    std::string op;
                    if(c->m_operator == ASR::reduction_opType::ReduceAdd) {
                        op += "+";
                    } else if(c->m_operator == ASR::reduction_opType::ReduceMul) {
                        op += "*";
                    } else if(c->m_operator == ASR::reduction_opType::ReduceSub) {
                        op += "-";
                    } else if(c->m_operator == ASR::reduction_opType::ReduceMAX) {
                        op += "max";
                    } else if(c->m_operator == ASR::reduction_opType::ReduceMIN) {
                        op += "min";
                    } else {
                        throw CodeGenError("Unsupported OpenMP reduction operator: " +
                            std::to_string((int)c->m_operator));
                    }
                    clauses += op + ": ";
                    std::string vars;
                    for (size_t j=0; j<c->n_vars; j++) {
                        visit_expr(*c->m_vars[j]);
                        vars += src;
                        if (j < c->n_vars - 1) {
                            vars += ", ";
                        }
                    }
                    clauses += vars + ")";
                } else if (ASR::is_a<ASR::OMPDevice_t>(*clause)) {
                    ASR::OMPDevice_t* c = ASR::down_cast<ASR::OMPDevice_t>(clause);
                    clauses += " device(";
                    visit_expr(*c->m_device);
                    clauses += src + ")";
                } else if (ASR::is_a<ASR::OMPMap_t>(*clause)) {
                    ASR::OMPMap_t* m = ASR::down_cast<ASR::OMPMap_t>(clause);
                    std::string map_clauses = generate_map_clauses(m);
                    clauses += map_clauses;
                }
            }
            
            opening_pragma += clauses;
            
            if (x.m_region == ASR::omp_region_typeType::Barrier || 
                x.m_region == ASR::omp_region_typeType::Taskwait) {
                // These are standalone directives
                src = opening_pragma + "\n";
                return;
            }
            

            std::string body;
            for(size_t i=0;i<x.n_body;i++) {
                this->visit_stmt(*x.m_body[i]);
                body += src;
            }
            if(x.m_region != ASR::omp_region_typeType::Target && x.m_region != ASR::omp_region_typeType::Teams &&
               x.m_region != ASR::omp_region_typeType::DistributeParallelDo &&
               x.m_region != ASR::omp_region_typeType::Distribute) {
                src = opening_pragma + "{\n" + body + "}\n";
            } else {
                src =  opening_pragma + "\n" + body + "\n";
            }
        } else {
            // CodeGen for --target-offload: Generate CUDA code
            if (x.m_region == ASR::omp_region_typeType::Target) {
                map_vars.clear();
                std::string target_code;
                std::string kernel_decls;
                std::string kernel_wrappers;
                std::string struct_copies;

                // Collect map clauses
                for (size_t i = 0; i < x.n_clauses; i++) {
                    if (ASR::is_a<ASR::OMPMap_t>(*x.m_clauses[i])) {
                        ASR::OMPMap_t* m = ASR::down_cast<ASR::OMPMap_t>(x.m_clauses[i]);
                        for (size_t j = 0; j < m->n_vars; j++) {
                            visit_expr(*m->m_vars[j]);
                            map_vars.push_back({src, m});
                        }
                    }
                }

                // Declare device pointers for data
                for (const auto &mv : map_vars) {
                    target_code += indent() + "float *d_" + mv.first + "_data = NULL;\n";
                }
                target_code += indent() + "cudaError_t err;\n";
                // Allocate device memory for data
                for (const auto &mv : map_vars) {
                    target_code += indent() + "size_t " + mv.first + "_data_size = " + mv.first + "->dims[0].length * sizeof(float);\n";
                    target_code += indent() + "err = cudaMalloc((void**)&d_" + mv.first + "_data, " + mv.first + "_data_size);\n";
                    target_code += indent() + "if (err != cudaSuccess) {\n";
                    target_code += indent() + "    fprintf(stderr, \"cudaMalloc failed for " + mv.first + "_data: %s\\n\", cudaGetErrorString(err));\n";
                    target_code += indent() + "    exit(1);\n";
                    target_code += indent() + "}\n";
                }

                // Copy data to device (based on map type)
                for (const auto &mv : map_vars) {
                    if (mv.second->m_type == ASR::map_typeType::To || mv.second->m_type == ASR::map_typeType::ToFrom) {
                        target_code += indent() + "err = cudaMemcpy(d_" + mv.first + "_data, " + mv.first + "->data, " + mv.first + "_data_size, cudaMemcpyHostToDevice);\n";
                        target_code += indent() + "if (err != cudaSuccess) {\n";
                        target_code += indent() + "    fprintf(stderr, \"cudaMemcpy H2D failed for " + mv.first + "_data: %s\\n\", cudaGetErrorString(err));\n";
                        target_code += indent() + "    exit(1);\n";
                        target_code += indent() + "}\n";
                    }
                }

                // Create host struct copies with device data pointers
                for (const auto &mv : map_vars) {
                    target_code += indent() + "struct r32 h_" + mv.first + "_copy = *" + mv.first + ";\n";
                    target_code += indent() + "h_" + mv.first + "_copy.data = d_" + mv.first + "_data;\n";
                }

                // Declare and allocate device struct pointers
                for (const auto &mv : map_vars) {
                    target_code += indent() + "struct r32 *d_" + mv.first + "_struct = NULL;\n";
                    target_code += indent() + "err = cudaMalloc((void**)&d_" + mv.first + "_struct, sizeof(struct r32));\n";
                    target_code += indent() + "if (err != cudaSuccess) {\n";
                    target_code += indent() + "    fprintf(stderr, \"cudaMalloc failed for d_" + mv.first + "_struct: %s\\n\", cudaGetErrorString(err));\n";
                    target_code += indent() + "    exit(1);\n";
                    target_code += indent() + "}\n";
                }

                // Copy host struct copies to device
                for (const auto &mv : map_vars) {
                    target_code += indent() + "err = cudaMemcpy(d_" + mv.first + "_struct, &h_" + mv.first + "_copy, sizeof(struct r32), cudaMemcpyHostToDevice);\n";
                    target_code += indent() + "if (err != cudaSuccess) {\n";
                    target_code += indent() + "    fprintf(stderr, \"cudaMemcpy H2D failed for d_" + mv.first + "_struct: %s\\n\", cudaGetErrorString(err));\n";
                    target_code += indent() + "    exit(1);\n";
                    target_code += indent() + "}\n";
                }

                // Process nested constructs (teams, distribute parallel do)
                for (size_t i = 0; i < x.n_body; i++) {
                    this->visit_stmt(*x.m_body[i]);
                    target_code += src;
                }

                // Copy back from device (data only, structs are metadata)
                for (const auto &mv : map_vars) {
                    if (mv.second->m_type == ASR::map_typeType::From || mv.second->m_type == ASR::map_typeType::ToFrom) {
                        target_code += indent() + "err = cudaMemcpy(" + mv.first + "->data, d_" + mv.first + "_data, " + mv.first + "_data_size, cudaMemcpyDeviceToHost);\n";
                        target_code += indent() + "if (err != cudaSuccess) {\n";
                        target_code += indent() + "    fprintf(stderr, \"cudaMemcpy D2H failed for " + mv.first + "_data: %s\\n\", cudaGetErrorString(err));\n";
                        target_code += indent() + "    exit(1);\n";
                        target_code += indent() + "}\n";
                    }
                }

                // Free device memory (data and structs)
                for (const auto &mv : map_vars) {
                    target_code += indent() + "cudaFree(d_" + mv.first + "_data);\n";
                    target_code += indent() + "cudaFree(d_" + mv.first + "_struct);\n";
                }

                src = kernel_decls + "\n" + target_code + "\n" + kernel_wrappers;
            } else if (x.m_region == ASR::omp_region_typeType::Teams) {
                // Teams: Set up grid dimensions (handled in distribute parallel do)
                std::string teams_code;
                for (size_t i = 0; i < x.n_body; i++) {
                    this->visit_stmt(*x.m_body[i]);
                    teams_code += src;
                }
                src = teams_code;
            } else if (x.m_region == ASR::omp_region_typeType::DistributeParallelDo ||
                       x.m_region == ASR::omp_region_typeType::Distribute ||
                       x.m_region == ASR::omp_region_typeType::ParallelDo) {
                // Parallel Do: Generate kernel and launch
                if (x.n_body != 1 || !ASR::is_a<ASR::DoLoop_t>(*x.m_body[0])) {
                    throw CodeGenError("Distribute Parallel Do must contain a single DoLoop");
                }

                ASR::DoLoop_t* loop = ASR::down_cast<ASR::DoLoop_t>(x.m_body[0]);
                std::string kernel_name = "compute_kernel_" + std::to_string(kernel_counter++);
                current_kernel_name = kernel_name;
                kernel_func_names.push_back(kernel_name);

                // Extract loop head
                std::string idx_var;
                visit_expr(*loop->m_head.m_v);
                idx_var = src;
                std::string n;
                visit_expr(*loop->m_head.m_end);
                n = src;

                // Generate kernel (pass struct pointers)
                std::string kernel_code = "#ifdef USE_GPU\n__global__\n#endif\n";
                kernel_code += "void " + kernel_name + "(";
                for (const auto &mv : map_vars) {
                    kernel_code += "struct r32 *" + mv.first + ", ";
                }
                kernel_code += "int " + idx_var + "_n) {\n";
                kernel_code += "    int " + idx_var + " = blockIdx.x * blockDim.x + threadIdx.x + 1;\n";
                kernel_code += "    if (" + idx_var + " <= " + idx_var + "_n) {\n";
                for (size_t i = 0; i < loop->n_body; i++) {
                    visit_stmt(*loop->m_body[i]);
                    kernel_code += "        " + src;
                }
                kernel_code += "    }\n}\n";

                // Generate CPU wrapper
                std::string wrapper_code = "#ifndef USE_GPU\n";
                wrapper_code += "void " + kernel_name + "_wrapper(void **args) {\n";
                int arg_idx = 0;
                for (const auto &mv : map_vars) {
                    wrapper_code += "    struct r32 *" + mv.first + " = *(struct r32**)args[" + std::to_string(arg_idx++) + "];\n";
                }
                wrapper_code += "    int " + idx_var + "_n = *(int*)args[" + std::to_string(arg_idx++) + "];\n";
                wrapper_code += "    " + kernel_name + "(";
                for (const auto &mv : map_vars) {
                    wrapper_code += mv.first + ", ";
                }
                wrapper_code += idx_var + "_n);\n";
                wrapper_code += "}\n#endif\n";

                // Generate kernel launch
                std::string launch_code = indent() + "int " + idx_var + "_n = " + n + ";\n";
                launch_code += indent() + "int threads_per_block = 256;\n";
                launch_code += indent() + "int blocks = (" + idx_var + "_n + threads_per_block - 1) / threads_per_block;\n";
                launch_code += indent() + "dim3 grid_dim = {blocks, 1, 1};\n";
                launch_code += indent() + "dim3 block_dim = {threads_per_block, 1, 1};\n";
                launch_code += indent() + "void *kernel_args[] = {";
                for (const auto &mv : map_vars) {
                    launch_code += "&d_" + mv.first + "_struct, ";
                }
                launch_code += "&" + idx_var + "_n};\n";
                launch_code += indent() + "err = cudaLaunchKernel((void*)" + kernel_name + ", grid_dim, block_dim, kernel_args, 0, NULL);\n";
                launch_code += indent() + "if (err != cudaSuccess) {\n";
                launch_code += indent() + "    fprintf(stderr, \"cudaLaunchKernel failed: %s\\n\", cudaGetErrorString(err));\n";
                launch_code += indent() + "    exit(1);\n";
                launch_code += indent() + "}\n";
                launch_code += indent() + "err = cudaDeviceSynchronize();\n";
                launch_code += indent() + "if (err != cudaSuccess) {\n";
                launch_code += indent() + "    fprintf(stderr, \"cudaDeviceSynchronize failed: %s\\n\", cudaGetErrorString(err));\n";
                launch_code += indent() + "    exit(1);\n";
                launch_code += indent() + "}\n";

                kernel_func_code = kernel_code + "\n" + wrapper_code + "\n" ;
                src = launch_code;
            } else {
                std::string body;
                for(size_t i=0;i<x.n_body;i++) {
                    this->visit_stmt(*x.m_body[i]);
                    body += src;
                }
                src = body;
            }
        }
    }

};

Result<std::string> asr_to_c(Allocator &al, ASR::TranslationUnit_t &asr,
    diag::Diagnostics &diagnostics, CompilerOptions &co,
    int64_t default_lower_bound)
{
    co.po.always_run = true;
    ASRToCVisitor v(diagnostics, co, default_lower_bound);
    try {
        v.visit_asr((ASR::asr_t &)asr);
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    } catch (const Abort &) {
        return Error();
    }
    return v.src;
}

Result<CTranslationUnitSplitResult> asr_to_c_split(Allocator &al,
    ASR::TranslationUnit_t &asr, diag::Diagnostics &diagnostics,
    CompilerOptions &co, int64_t default_lower_bound,
    const std::string &output_dir, const std::string &project_name)
{
    co.po.always_run = true;
    ASRToCVisitor v(diagnostics, co, default_lower_bound);
    try {
        return v.emit_split_translation_unit(asr, output_dir, project_name);
    } catch (const CodeGenError &e) {
        diagnostics.diagnostics.push_back(e.d);
        return Error();
    } catch (const std::exception &e) {
        diagnostics.semantic_error_label(
            "Split C emission failed: " + std::string(e.what()), {}, ""
        );
        return Error();
    } catch (const Abort &) {
        return Error();
    }
}

} // namespace LCompilers
