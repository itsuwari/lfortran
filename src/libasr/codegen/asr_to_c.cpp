#include <fstream>
#include <memory>

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

    bool target_offload_enabled;
    std::vector<std::string> kernel_func_names;
    int kernel_counter=0; // To generate unique kernel names
    std::string current_kernel_name; // Track current kernel for wrapper
    std::vector<std::pair<std::string, ASR::OMPMap_t*>> map_vars; // Track map vars for target offload
    std::string indent() {
        return std::string(indentation_level * indentation_spaces, ' ');
    }
    std ::string kernel_func_code;

    ASRToCVisitor(diag::Diagnostics &diag, CompilerOptions &co,
                  int64_t default_lower_bound)
         : BaseCCPPVisitor(diag, co.platform, co, false, false, true, default_lower_bound),
           counter{0} {
           target_offload_enabled = co.target_offload_enabled;
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
        const ASR::symbol_t *owner =
            ASR::down_cast<ASR::symbol_t>(member_var.m_parent_symtab->asr_owner);
        ASR::Enum_t *enum_type = ASR::down_cast<ASR::Enum_t>(
            const_cast<ASR::symbol_t*>(owner));
        return get_enum_c_name(*enum_type) + "__"
            + CUtils::sanitize_c_identifier(member_var.m_name);
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
                               bool convert_to_1d=false)
    {
        std::string dims = "";
        size_t size = 1;
        std::string array_size = "";
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
                             bool is_simd_array) {
        std::string indent(indentation_level*indentation_spaces, ' ');
        std::string type_name_copy = type_name;
        std::string original_type_name = type_name;
        type_name = c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls);
        std::string type_name_without_ptr = c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, false);
        if (is_simd_array) {
            int64_t size = ASRUtils::get_fixed_size_of_array(m_dims, n_dims);
            sub = original_type_name + " " + v_m_name + " __attribute__ (( vector_size(sizeof(" + original_type_name + ") * " + std::to_string(size) + ") ))";
            return;
        }
        if( declare_value ) {
            std::string variable_name = std::string(v_m_name) + "_value";
            sub = format_type_c("", type_name_without_ptr, variable_name, use_ref, dummy) + ";\n";
            sub += indent + format_type_c("", type_name, v_m_name, use_ref, dummy);
            sub += " = &" + variable_name;
            if( !is_pointer ) {
                sub += ";\n";
                if( !is_fixed_size ) {
                    sub += indent + format_type_c("*", type_name_copy, std::string(v_m_name) + "_data",
                                                use_ref, dummy);
                    if( dims.size() > 0 ) {
                        sub += " = " + dims + ";\n";
                    } else {
                        sub += ";\n";
                    }
                } else {
                    sub += indent + format_type_c(dims, type_name_copy, std::string(v_m_name) + "_data",
                                                use_ref, dummy) + ";\n";
                }
                sub += indent + std::string(v_m_name) + "->data = " + std::string(v_m_name) + "_data;\n";
                sub += indent + std::string(v_m_name) + "->n_dims = " + std::to_string(n_dims) + ";\n";
                sub += indent + std::string(v_m_name) + "->offset = " + std::to_string(0) + ";\n";
                std::string stride = "1";
                for (int i = n_dims - 1; i >= 0; i--) {
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
                sub.pop_back();
                sub.pop_back();
            }
        } else {
            if( m_abi == ASR::abiType::BindC ) {
                sub = format_type_c("", type_name_copy, v_m_name + "[]", use_ref, dummy);
            } else {
                sub = format_type_c("", type_name, v_m_name, use_ref, dummy);
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
            const std::string emitted_member_name = CUtils::get_c_symbol_name(sym);
            ASR::ttype_t* mem_type = ASRUtils::symbol_type(sym);
            if( ASRUtils::is_character(*mem_type) ) {
                sub += indent + name + "->" + emitted_member_name + " = NULL;\n";
            } else if( ASRUtils::is_array(mem_type) &&
                        ASR::is_a<ASR::Variable_t>(*member_sym) ) {
                ASR::Variable_t* mem_var = ASR::down_cast<ASR::Variable_t>(member_sym);
                std::string safe_member_name = CUtils::get_c_symbol_name(sym);
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
                    sub += indent + name + "->" + emitted_member_name + " = " + mem_var_name + ";\n";
                }
            } else if( ASR::is_a<ASR::StructType_t>(*mem_type) ) {
                // TODO: StructType
                // ASR::StructType_t* struct_t = ASR::down_cast<ASR::StructType_t>(mem_type);
                // ASR::Struct_t* struct_type_t = ASR::down_cast<ASR::Struct_t>(
                //     ASRUtils::symbol_get_past_external(struct_t->m_derived_type));
                // allocate_array_members_of_struct(struct_type_t, sub, indent, "(&(" + name + "->" + itr.first + "))");
            }
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
        } else if (ASR::is_a<ASR::StructType_t>(*v_m_type) && v.m_symbolic_value
                && ASR::is_a<ASR::StructConstant_t>(*ASRUtils::expr_value(v.m_symbolic_value))) {
            ASR::StructConstant_t *sc = ASR::down_cast<ASR::StructConstant_t>(
                ASRUtils::expr_value(v.m_symbolic_value));
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
                dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                bool is_struct_type_member = ASR::is_a<ASR::Struct_t>(
                    *ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner));
                if( is_fixed_size && is_struct_type_member ) {
                    if( !force_declare ) {
                        force_declare_name = v_name;
                    }
                    sub = type_name + " " + force_declare_name + dims;
                } else if (is_struct_type_member) {
                    std::string encoded_type_name = CUtils::get_c_type_code(v_m_type);
                    if( !force_declare ) {
                        force_declare_name = v_name;
                    }
                    std::string array_type =
                        c_ds_api->get_array_type(type_name, encoded_type_name, array_types_decls, true);
                    sub = format_type_c("", array_type, force_declare_name, use_ref, dummy);
                } else {
                    std::string encoded_type_name = CUtils::get_c_type_code(v_m_type);
                    if( !force_declare ) {
                        force_declare_name = v_name;
                    }
                bool is_module_var = ASR::is_a<ASR::Module_t>(
                    *ASR::down_cast<ASR::symbol_t>(v.m_parent_symtab->asr_owner));
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
                                    is_fixed_size, false, ASR::abiType::Source, is_simd_array);
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

    std::string get_function_pointer_declaration_from_type(
            const ASR::FunctionType_t *ft, const std::string &name) {
        std::string ret_type = "void";
        if (ft->m_return_var_type) {
            ret_type = CUtils::get_c_type_from_ttype_t(ft->m_return_var_type);
        }
        std::string decl = ret_type + " (*" + name + ")(";
        for (size_t i = 0; i < ft->n_arg_types; i++) {
            ASR::ttype_t *arg_type = ft->m_arg_types[i];
            if (ASRUtils::is_character(*arg_type) && ASRUtils::is_allocatable(arg_type)) {
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

    std::string get_function_pointer_declaration_from_interface(
            const ASR::Function_t &iface_fn, const std::string &name) {
        ASR::FunctionType_t *ft = ASRUtils::get_FunctionType(iface_fn);
        std::string ret_type = "void";
        if (iface_fn.m_return_var) {
            ASR::Variable_t *return_var = ASRUtils::EXPR2VAR(iface_fn.m_return_var);
            ret_type = get_return_var_type(return_var);
        } else if (ft->m_return_var_type) {
            ret_type = CUtils::get_c_type_from_ttype_t(ft->m_return_var_type) + " ";
        }
        std::string decl = ret_type + " (*" + name + ")(";
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
        bool declaration_only = do_not_initialize;
        bool is_array = ASRUtils::is_array(v.m_type);
        bool dummy = ASRUtils::is_arg_dummy(v.m_intent);
        std::string c_v_name = get_variable_c_name(v);
        std::string decl_name = (force_declare && !force_declare_name.empty())
            ? force_declare_name : c_v_name;
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
        if (!is_array && v.m_storage == ASR::storage_typeType::Parameter
                && v.m_symbolic_value
                && v.m_symbolic_value->type == ASR::exprType::StructConstant) {
            ASR::StructConstant_t *struct_const =
                ASR::down_cast<ASR::StructConstant_t>(v.m_symbolic_value);
            ASR::symbol_t *struct_sym = ASRUtils::symbol_get_past_external(struct_const->m_dt_sym);
            sub = format_type_c("", "const struct " + CUtils::get_c_symbol_name(struct_sym),
                                c_v_name, false, false);
            this->visit_expr(*v.m_symbolic_value);
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
            if (ASRUtils::is_integer(*v_m_type)) {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(v_m_type);
                return format_type_c(dims, "int" + std::to_string(t->m_kind * 8) + "_t *",
                    decl_name, use_ref, dummy);
            } else if (ASRUtils::is_unsigned_integer(*v_m_type)) {
                ASR::UnsignedInteger_t *t = ASR::down_cast<ASR::UnsignedInteger_t>(v_m_type);
                return format_type_c(dims, "uint" + std::to_string(t->m_kind * 8) + "_t *",
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
                return format_type_c(dims, type_name, decl_name, use_ref, dummy);
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
                return format_type_c(dims, type_name, decl_name, use_ref, dummy);
            } else if (ASRUtils::is_logical(*v_m_type)) {
                headers.insert("stdbool.h");
                return format_type_c(dims, "bool *", decl_name, use_ref, dummy);
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
                        c_v_name);
                }
                return get_function_pointer_declaration_from_type(
                    ASR::down_cast<ASR::FunctionType_t>(t2),
                    c_v_name);
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
                    std::string encoded_type_name =
                        CUtils::get_c_type_code(ASRUtils::type_get_past_allocatable_pointer(v_m_type));
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
                                            v.m_intent != ASRUtils::intent_unspecified, is_fixed_size, true, ASR::abiType::Source, false);
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
                    std::string encoded_type_name =
                        CUtils::get_c_type_code(ASRUtils::type_get_past_allocatable_pointer(v_m_type));
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
                                            is_fixed_size, true, ASR::abiType::Source, false);
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
                    std::string encoded_type_name =
                        CUtils::get_c_type_code(ASRUtils::type_get_past_allocatable_pointer(v_m_type));
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
                                            is_fixed_size, true, ASR::abiType::Source, is_simd_array);
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
                    std::string encoded_type_name =
                        CUtils::get_c_type_code(ASRUtils::type_get_past_allocatable_pointer(v_m_type));
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
                                            is_fixed_size, true, ASR::abiType::Source, false);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if (ASRUtils::is_character(*t2)) {
                std::string type_name = "char *";
                if( !is_array ) {
                    type_name.append("*");
                }
                if( is_array ) {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size, true);
                    std::string encoded_type_name =
                        CUtils::get_c_type_code(ASRUtils::type_get_past_allocatable_pointer(v_m_type));
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
                                            is_fixed_size, true, ASR::abiType::Source, false);
                    }
                } else {
                    bool is_fixed_size = true;
                    std::string dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    sub = format_type_c(dims, type_name, decl_name, use_ref, dummy);
                }
            } else if(ASR::is_a<ASR::StructType_t>(*t2)) {
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
                                            is_fixed_size, false, ASR::abiType::Source, false);
                    }
                 } else {
                    std::string ptr_char = "*";
                    if( !use_ptr_for_derived_type && !ASRUtils::is_pointer(v.m_type) ) {
                        ptr_char.clear();
                    }
                    sub = format_type_c("", "struct " + der_type_name + ptr_char,
                                        decl_name, use_ref, dummy);
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
                if (v.m_intent != ASR::intentType::ReturnVar) {
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
                    if(v.m_intent == ASRUtils::intent_local &&
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
                 if( is_array ) {
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
                } else if (ASR::down_cast<ASR::StructType_t>(v_m_type)->m_is_unlimited_polymorphic) {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    std::string poly_type = "void*";
                    sub = format_type_c(dims, poly_type, decl_name, use_ref, dummy);
                    if (v.m_intent == ASRUtils::intent_local && !do_not_initialize) {
                        sub += " = NULL";
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
                        if (v.m_symbolic_value && !do_not_initialize) {
                            this->visit_expr(*v.m_symbolic_value);
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
                        sub = format_type_c(dims, "struct " + der_type_name,
                                            value_var_name, use_ref, dummy);
                        sub += " = { ." + get_runtime_type_tag_member_name()
                            + " = " + std::to_string(get_struct_runtime_type_id(v.m_type_declaration))
                            + " };\n";
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
                    if (v.m_symbolic_value && !do_not_initialize) {
                        this->visit_expr(*v.m_symbolic_value);
                        std::string init = src;
                        sub += "=" + init;
                    }
                    sub += ";\n";
                    sub += indent + value_var_name + "." + get_runtime_type_tag_member_name()
                        + " = " + std::to_string(get_struct_runtime_type_id(v.m_type_declaration))
                        + ";\n";
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
                        ASR::Struct_t* der_type_t = ASR::down_cast<ASR::Struct_t>(
                        ASRUtils::symbol_get_past_external(v.m_type_declaration));
                        allocate_array_members_of_struct(der_type_t, sub, indent, decl_name);
                        sub.pop_back();
                        sub.pop_back();
                    }
                    return sub;
                } else {
                    bool is_fixed_size = true;
                    dims = convert_dims_c(n_dims, m_dims, v_m_type, is_fixed_size);
                    if( v.m_intent == ASRUtils::intent_in ||
                        v.m_intent == ASRUtils::intent_inout ||
                        v.m_intent == ASRUtils::intent_out ) {
                        use_ref = false;
                        dims = "";
                    }
                    std::string ptr_char = "*";
                    if( !use_ptr_for_derived_type && !force_pointer_backed_struct ) {
                        ptr_char.clear();
                    }
                    sub = format_type_c(dims, "struct " + der_type_name + ptr_char,
                                        decl_name, use_ref, dummy);
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
                                            ASR::abiType::Source, false);
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
                ASR::expr_t* init_expr = ASRUtils::expr_value(v.m_symbolic_value);
                if (init_expr && ASR::is_a<ASR::StructConstant_t>(*init_expr)) {
                    ASR::StructConstant_t *sc = ASR::down_cast<ASR::StructConstant_t>(init_expr);
                    std::string der_type_name = CUtils::get_c_symbol_name(
                        ASRUtils::symbol_get_past_external(sc->m_dt_sym));
                    sub = format_type_c("", "struct " + der_type_name, c_v_name, false, false);
                } else {
                    sub = format_type_c("", "void*", c_v_name, false, false);
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
            if (dims.size() == 0 && v.m_symbolic_value && !do_not_initialize) {
                ASR::expr_t* init_expr = ASRUtils::expr_value(v.m_symbolic_value);
                if (!init_expr) {
                    init_expr = v.m_symbolic_value;
                }
                if( v.m_storage != ASR::storage_typeType::Parameter ) {
                    for( size_t i = 0; i < v.n_dependencies; i++ ) {
                        std::string variable_name = v.m_dependencies[i];
                        ASR::symbol_t* dep_sym = current_scope->resolve_symbol(variable_name);
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
        std::vector<std::string> global_func_order = ASRUtils::determine_function_definition_order(x.m_symtab);

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
        size_t i;
        for (i = 0; i < global_func_order.size(); i++) {
            ASR::symbol_t* sym = x.m_symtab->get_symbol(global_func_order[i]);
            // Ignore external symbols because they are already defined by the loop above.
            if( !sym || ASR::is_a<ASR::ExternalSymbol_t>(*sym) ) {
                continue ;
            }
            visit_symbol(*sym);
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
        std::vector<std::string> func_order = ASRUtils::determine_function_definition_order(x.m_symtab);
        for (auto &item : func_order) {
            ASR::symbol_t* sym = x.m_symtab->get_symbol(item);
            ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(sym);
            visit_Function(*s);
            unit_src += src;
        }
        src = unit_src;
        intrinsic_module = false;
    }

    void visit_Program(const ASR::Program_t &x) {
        // Topologically sort all program functions
        // and then define them in the right order
        std::vector<std::string> func_order = ASRUtils::determine_function_definition_order(x.m_symtab);

        // Generate code for nested subroutines and functions first:
        std::string contains;
        for (auto &item : func_order) {
            ASR::symbol_t* sym = x.m_symtab->get_symbol(item);
            ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(sym);
            visit_Function(*s);
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
            body += indent + "int32_t " + get_runtime_type_tag_member_name() + ";\n";
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
        if( x.m_enum_value_type == ASR::enumtypeType::IntegerUnique &&
            x.m_abi == ASR::abiType::BindC ) {
            throw CodeGenError("C-interoperation support for non-consecutive but uniquely "
                               "valued integer enums isn't available yet.");
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

        std::pair<std::string, std::string> file_arg = visit_string_arg(x.m_file);
        std::pair<std::string, std::string> write_arg = visit_string_arg(x.m_write);
        std::pair<std::string, std::string> read_arg = visit_string_arg(x.m_read);
        std::pair<std::string, std::string> readwrite_arg = visit_string_arg(x.m_readwrite);
        std::pair<std::string, std::string> access_arg = visit_string_arg(x.m_access);
        std::pair<std::string, std::string> name_arg = visit_string_arg(x.m_name);
        std::pair<std::string, std::string> blank_arg = visit_string_arg(x.m_blank);
        std::pair<std::string, std::string> sequential_arg = visit_string_arg(x.m_sequential);
        std::pair<std::string, std::string> direct_arg = visit_string_arg(x.m_direct);
        std::pair<std::string, std::string> form_arg = visit_string_arg(x.m_form);
        std::pair<std::string, std::string> formatted_arg = visit_string_arg(x.m_formatted);
        std::pair<std::string, std::string> unformatted_arg = visit_string_arg(x.m_unformatted);
        std::pair<std::string, std::string> decimal_arg = visit_string_arg(x.m_decimal);
        std::pair<std::string, std::string> sign_arg = visit_string_arg(x.m_sign);
        std::pair<std::string, std::string> encoding_arg = visit_string_arg(x.m_encoding);
        std::pair<std::string, std::string> stream_arg = visit_string_arg(x.m_stream);
        std::pair<std::string, std::string> iomsg_arg = visit_string_arg(x.m_iomsg);
        std::pair<std::string, std::string> round_arg = visit_string_arg(x.m_round);
        std::pair<std::string, std::string> pad_arg = visit_string_arg(x.m_pad);
        std::pair<std::string, std::string> asynchronous_arg = visit_string_arg(x.m_asynchronous);

        std::string unit = "-1";
        if (x.m_unit) {
            this->visit_expr(*x.m_unit);
            unit = src;
        }

        src = indent + "_lfortran_inquire("
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
            + asynchronous_arg.first + ", " + asynchronous_arg.second + ");\n";
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
            return {value, "strlen(" + value + ")"};
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
            std::string offset_name = current_scope->get_unique_name("__lfortran_read_offset");
            std::string internal_read_code;
            internal_read_code += indent + "int64_t " + offset_name + " = 0;\n";
            if (x.m_iomsg) {
                internal_read_code += indent + "if (" + iomsg_arg.first + " == NULL) " + iomsg_arg.first
                    + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                    + iomsg_arg.second + ");\n";
            }

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
                    std::string data_ptr = base;
                    if (base_phys == ASR::array_physical_typeType::DescriptorArray ||
                        base_phys == ASR::array_physical_typeType::PointerArray ||
                        base_phys == ASR::array_physical_typeType::UnboundedPointerArray) {
                        data_ptr += "->data";
                    }

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

                    std::string idx_name = current_scope->get_unique_name("__lfortran_read_idx");
                    std::string off_name = current_scope->get_unique_name("__lfortran_read_off");
                    internal_read_code += indent + "for (int32_t " + idx_name + " = " + start_expr + ", "
                        + off_name + " = ((" + start_expr + ") - (" + lower_bound_expr + ")); "
                        + "(((" + step_expr + ") >= 0) && (" + idx_name + " <= " + end_expr + ")) || "
                        + "(((" + step_expr + ") < 0) && (" + idx_name + " >= " + end_expr + ")); "
                        + idx_name + " += (" + step_expr + "), " + off_name + " += (" + step_expr + ")) {\n";

                    std::string elem_ptr = "&(" + data_ptr + "[" + off_name + "])";
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
                        std::string tmp_name = current_scope->get_unique_name("__lfortran_read_logical");
                        internal_read_code += inner_indent + "int32_t " + tmp_name + " = 0;\n";
                        internal_read_code += inner_indent + "_lfortran_string_read_bool(" + unit_arg.first + ", " + unit_arg.second + ", "
                            + format_arg + ", &" + tmp_name + ", " + iostat_ptr + ", &" + offset_name + ");\n";
                        internal_read_code += inner_indent + data_ptr + "[" + off_name + "] = (" + tmp_name + " != 0);\n";
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
                    std::string tmp_name = current_scope->get_unique_name("__lfortran_read_logical");
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
                    + "1, 0, 0, &(" + value + "), " + value_len + ");\n";
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

            auto append_formatted_scalar = [&](ASR::expr_t *value_expr) {
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
                    formatted_setup += indent + "if (" + value + " == NULL) " + value
                        + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                        + value_len + ");\n";
                    formatted_args += ", 0, 0, &(" + value + "), " + value_len;
                    formatted_arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Logical_t>(*value_type_past_allocatable)) {
                    std::string tmp_name = current_scope->get_unique_name("__lfortran_fmt_logical");
                    formatted_setup += indent + "int32_t " + tmp_name + " = 0;\n";
                    formatted_args += ", 0, 1, &" + tmp_name;
                    formatted_post += indent + value + " = (" + tmp_name + " != 0);\n";
                    formatted_arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Integer_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Integer_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        formatted_args += ", 0, 2, &(" + value + ")";
                    } else if (kind == 8) {
                        formatted_args += ", 0, 3, &(" + value + ")";
                    } else {
                        throw CodeGenError("C backend FileRead supports formatted scalar integer reads only for kind=4 or kind=8",
                            value_expr->base.loc);
                    }
                    formatted_arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Real_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Real_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        formatted_args += ", 0, 4, &(" + value + ")";
                    } else if (kind == 8) {
                        formatted_args += ", 0, 5, &(" + value + ")";
                    } else {
                        throw CodeGenError("C backend FileRead supports formatted scalar real reads only for kind=4 or kind=8",
                            value_expr->base.loc);
                    }
                    formatted_arg_count++;
                    return;
                }

                if (ASR::is_a<ASR::Complex_t>(*value_type_past_allocatable)) {
                    int kind = ASR::down_cast<ASR::Complex_t>(value_type_past_allocatable)->m_kind;
                    if (kind == 4) {
                        formatted_args += ", 0, 6, &(" + value + ")";
                    } else if (kind == 8) {
                        formatted_args += ", 0, 7, &(" + value + ")";
                    } else {
                        throw CodeGenError("C backend FileRead supports formatted scalar complex reads only for kind=4 or kind=8",
                            value_expr->base.loc);
                    }
                    formatted_arg_count++;
                    return;
                }

                throw CodeGenError("C backend FileRead currently supports only scalar integer/real/complex/logical/character values for formatted external reads",
                    value_expr->base.loc);
            };

            for (size_t i = 0; i < x.n_values; i++) {
                append_formatted_scalar(x.m_values[i]);
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
            for (int i = n_dims - 1; i >= 0; i--) {
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
        std::string type_src = CUtils::get_c_type_from_ttype_t(ASRUtils::expr_type(x.m_ptr));
        src += indent + dest_src + " = (" + type_src + ") " + source_src + ";\n";
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
            unit_len_name = current_scope->get_unique_name("__lfortran_unit_len");
            unit_setup += indent + "int64_t " + unit_len_name + " = (int64_t)(" + unit_len + ");\n";
            if (ASR::is_a<ASR::Var_t>(*x.m_unit)) {
                unit_setup += indent + "if (" + unit + " == NULL) " + unit
                    + " = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), "
                    + unit_len_name + ");\n";
            }
            ASR::ttype_t *unit_type = ASRUtils::expr_type(x.m_unit);
            is_unit_allocatable = ASRUtils::is_allocatable(unit_type);
            is_unit_deferred = ASRUtils::is_deferredLength_string(unit_type);
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
                    std::string size_expr = "sizeof(" + CUtils::get_c_type_from_ttype_t(past_type) + ")";
                    code += indent + "_lfortran_file_write(" + unit + ", " + iostat_ptr + ", \"\", 0, "
                        + "((int32_t)" + size_expr + "), (void*)&(" + value + "), -1);\n";
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
                if (ASRUtils::is_array(value_type)) {
                    printable_type = ASRUtils::extract_type(value_type);
                }
                if (ASR::is_a<ASR::List_t>(*value_type) || ASR::is_a<ASR::Tuple_t>(*value_type)) {
                    throw CodeGenError("C backend FileWrite does not support list/tuple values in StringFormat yet",
                        str_fmt->m_args[i]->base.loc);
                }
                if (i != 0 || has_hash_prefix) {
                    snprintf_fmt += " ";
                }
                snprintf_fmt += c_ds_api->get_print_type(printable_type,
                    ASR::is_a<ASR::ArrayItem_t>(*(str_fmt->m_args[i])));
                if (ASRUtils::is_array(value_type)) {
                    arg_value += "->data";
                }
                if (ASR::is_a<ASR::Complex_t>(*printable_type)) {
                    fmt_args.push_back("creal(" + arg_value + ")");
                    fmt_args.push_back("cimag(" + arg_value + ")");
                } else {
                    fmt_args.push_back(arg_value);
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
                src += indent + "_lfortran_string_write(&(" + unit + "), "
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
            src += indent + "_lfortran_string_write(&(" + unit + "), "
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
        std::string args = "";
        std::string result_type = CUtils::get_c_type_from_ttype_t(x.m_type);
        if (x.m_dim == nullptr) {
            std::string array_size_func = c_utils_functions->get_array_size();
            ASR::dimension_t* m_dims = nullptr;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(x.m_v), m_dims);
            src = "((" + result_type + ") " + array_size_func + "(" + var_name + "->dims, " + std::to_string(n_dims) + "))";
        } else {
            visit_expr(*x.m_dim);
            std::string idx = src;
            src = "((" + result_type + ")" + var_name + "->dims[" + idx + "-1].length)";
        }
    }

    void visit_ArrayReshape(const ASR::ArrayReshape_t& x) {
        CHECK_FAST_C(compiler_options, x)
        visit_expr(*x.m_array);
        std::string array = src;
        visit_expr(*x.m_shape);
        std::string shape = src;

        ASR::ttype_t* array_type_asr = ASRUtils::expr_type(x.m_array);
        std::string array_type_name = CUtils::get_c_type_from_ttype_t(array_type_asr);
        std::string array_encoded_type_name = CUtils::get_c_type_code(array_type_asr, true, false, false);
        std::string array_type = c_ds_api->get_array_type(array_type_name, array_encoded_type_name, array_types_decls, true);
        std::string return_type = c_ds_api->get_array_type(array_type_name, array_encoded_type_name, array_types_decls, false);

        ASR::ttype_t* shape_type_asr = ASRUtils::expr_type(x.m_shape);
        std::string shape_type_name = CUtils::get_c_type_from_ttype_t(shape_type_asr);
        std::string shape_encoded_type_name = CUtils::get_c_type_code(shape_type_asr, true, false, false);
        std::string shape_type = c_ds_api->get_array_type(shape_type_name, shape_encoded_type_name, array_types_decls, true);

        std::string array_reshape_func = c_utils_functions->get_array_reshape(array_type, shape_type,
            return_type, array_type_name, array_encoded_type_name);
        src = array_reshape_func + "(" + array + ", " + shape + ")";
    }

    void visit_ArrayBound(const ASR::ArrayBound_t& x) {
        CHECK_FAST_C(compiler_options, x)
        visit_expr(*x.m_v);
        std::string var_name = src;
        std::string args = "";
        std::string result_type = CUtils::get_c_type_from_ttype_t(x.m_type);
        visit_expr(*x.m_dim);
        std::string idx = src;
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
    }

    void visit_ArrayConstant(const ASR::ArrayConstant_t& x) {
        // TODO: Support and test for multi-dimensional array constants
        headers.insert("stdarg.h");
        std::string array_const = "";
        for( size_t i = 0; i < (size_t) ASRUtils::get_fixed_size_of_array(x.m_type); i++ ) {
            array_const += ASRUtils::fetch_ArrayConstant_value(x, i) + ", ";
        }
        array_const.pop_back();
        array_const.pop_back();

        ASR::ttype_t* array_type_asr = x.m_type;
        std::string array_type_name = CUtils::get_c_type_from_ttype_t(array_type_asr);
        std::string array_encoded_type_name = CUtils::get_c_type_code(array_type_asr, true, false);
        std::string return_type = c_ds_api->get_array_type(array_type_name, array_encoded_type_name,array_types_decls, false);

        src = c_utils_functions->get_array_constant(return_type, array_type_name, array_encoded_type_name) +
                "(" + std::to_string(ASRUtils::get_fixed_size_of_array(x.m_type)) + ", " + array_const + ")";
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
                    current_index += src;
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
        if( array_t->m_physical_type == ASR::array_physical_typeType::PointerArray ||
                array_t->m_physical_type == ASR::array_physical_typeType::FixedSizeArray ) {
            for( size_t idim = 0; idim < x.n_args; idim++ ) {
                diminfo.push_back(get_dim_start_src(m_dims[idim]));
                diminfo.push_back(get_dim_length_src(m_dims[idim]));
            }
        } else if( array_t->m_physical_type == ASR::array_physical_typeType::UnboundedPointerArray ) {
            for( size_t idim = 0; idim < x.n_args; idim++ ) {
                diminfo.push_back(get_dim_start_src(m_dims[idim]));
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
                                                array_t->m_physical_type == ASR::array_physical_typeType::PointerArray,
                                                array_t->m_physical_type == ASR::array_physical_typeType::FixedSizeArray,
                                                diminfo, false);
        }
        last_expr_precedence = 2;
    }

    void visit_ArraySection(const ASR::ArraySection_t &x) {
        CHECK_FAST_C(compiler_options, x)
        this->visit_expr(*x.m_v);
        last_expr_precedence = 2;
    }

    void visit_StringItem(const ASR::StringItem_t& x) {
        CHECK_FAST_C(compiler_options, x)
        this->visit_expr(*x.m_idx);
        std::string idx = std::move(src);
        this->visit_expr(*x.m_arg);
        std::string str = std::move(src);
        src = "_lfortran_str_item_alloc(_lfortran_get_default_allocator(), "
            + str + ", strlen(" + str + "), " + idx + ")";
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
        src = "strlen(" + src + ")";
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
    pass_unused_functions(al, asr, co.po);
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

} // namespace LCompilers
