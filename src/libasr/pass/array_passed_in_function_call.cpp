#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/asr_builder.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/replace_array_passed_in_function_call.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_subroutine_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>
#include <libasr/pickle.h>

#include <set>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*
This pass collector that the BinOp only Var nodes and nothing else.
*/
class ArrayVarCollector: public ASR::BaseWalkVisitor<ArrayVarCollector> {
    private:

    Allocator& al;
    Vec<ASR::expr_t*>& vars;

    public:

    ArrayVarCollector(Allocator& al_, Vec<ASR::expr_t*>& vars_): al(al_), vars(vars_) {}

    void visit_Var(const ASR::Var_t& x) {
        if( ASRUtils::is_array(ASRUtils::symbol_type(x.m_v)) ) {
            vars.push_back(al, const_cast<ASR::expr_t*>(&(x.base)));
        }
    }

    void visit_StructInstanceMember(const ASR::StructInstanceMember_t& x) {
        if( ASRUtils::is_array(ASRUtils::symbol_type(x.m_m)) ) {
            vars.push_back(al, const_cast<ASR::expr_t*>(&(x.base)));
        }
    }

    void visit_ArrayBroadcast(const ASR::ArrayBroadcast_t& /*x*/) {

    }

    void visit_ArraySize(const ASR::ArraySize_t& /*x*/) {

    }

};

void transform_stmts_impl(Allocator& al, ASR::stmt_t**& m_body, size_t& n_body,
    Vec<ASR::stmt_t*>*& current_body, Vec<ASR::stmt_t*>*& body_after_curr_stmt,
    std::function<void(const ASR::stmt_t&)> visit_stmt) {
    Vec<ASR::stmt_t*>* current_body_copy = current_body;
    Vec<ASR::stmt_t*> current_body_vec; current_body_vec.reserve(al, 1);
    current_body_vec.reserve(al, n_body);
    current_body = &current_body_vec;
    for (size_t i = 0; i < n_body; i++) {
        Vec<ASR::stmt_t*>* body_after_curr_stmt_copy = body_after_curr_stmt;
        Vec<ASR::stmt_t*> body_after_curr_stmt_vec; body_after_curr_stmt_vec.reserve(al, 1);
        body_after_curr_stmt = &body_after_curr_stmt_vec;
        visit_stmt(*m_body[i]);
        current_body->push_back(al, m_body[i]);
        for (size_t j = 0; j < body_after_curr_stmt_vec.size(); j++) { 
            current_body->push_back(al, body_after_curr_stmt_vec[j]);
        }
        body_after_curr_stmt = body_after_curr_stmt_copy;
    }
    m_body = current_body_vec.p; n_body = current_body_vec.size();
    current_body = current_body_copy;
}

/*
    This pass is responsible to convert non-contiguous ( DescriptorArray, arrays with stride != 1  )
    arrays passed to functions by casting to contiguous ( PointerArray ) arrays.

    For example:

    subroutine matprod(y)
        real(8), intent(inout) :: y(:, :)
        call istril(y)
    end subroutine 

    gets converted to:

    subroutine matprod(y)
        real(8), intent(inout) :: y(:, :)
        real(8), pointer :: y_tmp(:, :)
        if (.not. is_contiguous(y))
            allocate(y_tmp(size(y, 1), size(y, 2)))
            y_tmp = y
        else
            y_tmp => y
        end if
        call istril(y_tmp)
        if (.not. is_contiguous(y)) ! only if intent is inout, out
            y = y_tmp
            deallocate(y_tmp)
        end if
    end subroutine
*/
class CallVisitor : public ASR::CallReplacerOnExpressionsVisitor<CallVisitor>
{
public:

    Allocator &al;
    int is_current_body_set = 0;
    Vec<ASR::stmt_t*>* current_body;
    Vec<ASR::stmt_t*>* body_after_curr_stmt;
    const LCompilers::PassOptions& pass_options;

    // Variables that were associated (by pass_array_by_data) with an
    // ArraySection whose source array is UnboundedPointerArray (assumed-size)
    // or FixedSizeArray (sequence association via --legacy-array-sections).
    // Copy-in/copy-out must be skipped for these because:
    // - UnboundedPointerArray: section bounds reference undefined UBound
    // - FixedSizeArray: sequence association requires the callee to index the
    //   original contiguous memory with its own leading dimension
    std::set<ASR::symbol_t*> vars_from_assumed_size_sections;
    std::set<ASR::symbol_t*> vars_from_c_no_copy_section_views;
    std::set<ASR::symbol_t*> one_based_contiguous_allocs;

    CallVisitor(Allocator &al_, const LCompilers::PassOptions& pass_options_) : al(al_), pass_options(pass_options_) {}

    int get_index_kind() const {
        return pass_options.descriptor_index_64 ? 8 : 4;
    }

    ASR::ttype_t* get_index_type(const Location& loc) {
        return ASRUtils::TYPE(ASR::make_Integer_t(al, loc, get_index_kind()));
    }

    ASR::expr_t* get_index_one(const Location& loc) {
        return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, get_index_type(loc)));
    }

    ASR::expr_t* get_index_constant(const Location& loc, int64_t value) {
        return ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, value, get_index_type(loc)));
    }

    bool is_integer_constant_one(ASR::expr_t *expr) {
        if (!expr) {
            return true;
        }
        expr = ASRUtils::expr_value(expr);
        return expr && ASR::is_a<ASR::IntegerConstant_t>(*expr)
            && ASR::down_cast<ASR::IntegerConstant_t>(expr)->m_n == 1;
    }

    ASR::symbol_t* get_whole_array_var_symbol(ASR::expr_t *expr) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (ASR::is_a<ASR::Var_t>(*expr)) {
            return ASRUtils::symbol_get_past_external(
                ASR::down_cast<ASR::Var_t>(expr)->m_v);
        }
        return nullptr;
    }

    bool allocation_has_default_lower_bounds(const ASR::alloc_arg_t &arg) {
        for (size_t i = 0; i < arg.n_dims; i++) {
            if (!is_integer_constant_one(arg.m_dims[i].m_start)) {
                return false;
            }
        }
        return true;
    }

    void track_allocate_arg(const ASR::alloc_arg_t &arg) {
        ASR::symbol_t *sym = get_whole_array_var_symbol(arg.m_a);
        if (!sym) {
            return;
        }
        if (allocation_has_default_lower_bounds(arg)) {
            one_based_contiguous_allocs.insert(sym);
        } else {
            one_based_contiguous_allocs.erase(sym);
        }
    }

    bool is_array_bound_kind(ASR::expr_t *expr, ASR::arrayboundType bound) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        return expr && ASR::is_a<ASR::ArrayBound_t>(*expr)
            && ASR::down_cast<ASR::ArrayBound_t>(expr)->m_bound == bound;
    }

    bool is_full_unit_slice(const ASR::array_index_t &idx) {
        return is_array_bound_kind(idx.m_left, ASR::arrayboundType::LBound)
            && is_array_bound_kind(idx.m_right, ASR::arrayboundType::UBound)
            && is_integer_constant_one(idx.m_step);
    }

    bool is_scalar_section_index(const ASR::array_index_t &idx) {
        return idx.m_left == nullptr && idx.m_right != nullptr && idx.m_step == nullptr;
    }

    bool is_variable_declared_contiguous(ASR::symbol_t *sym) {
        if (!sym) {
            return false;
        }
        sym = ASRUtils::symbol_get_past_external(sym);
        return ASR::is_a<ASR::Variable_t>(*sym)
            && ASR::down_cast<ASR::Variable_t>(sym)->m_contiguous_attr;
    }

    bool array_type_has_default_lower_bounds(ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_allocatable_pointer(type);
        if (!type || !ASRUtils::is_array(type)) {
            return false;
        }
        ASR::dimension_t *dims = nullptr;
        int rank = ASRUtils::extract_dimensions_from_ttype(type, dims);
        for (int i = 0; i < rank; i++) {
            if (!is_integer_constant_one(dims[i].m_start)) {
                return false;
            }
        }
        return true;
    }

    bool is_known_contiguous_section_base(ASR::symbol_t *base_sym) {
        return base_sym && (one_based_contiguous_allocs.count(base_sym) > 0 ||
            is_variable_declared_contiguous(base_sym));
    }

    bool is_known_one_based_contiguous_section(ASR::ArraySection_t *section) {
        ASR::symbol_t *base_sym = get_whole_array_var_symbol(section->m_v);
        if (!is_known_contiguous_section_base(base_sym)) {
            return false;
        }
        bool seen_scalar_index = false;
        for (size_t i = 0; i < section->n_args; i++) {
            if (is_scalar_section_index(section->m_args[i])) {
                seen_scalar_index = true;
                continue;
            }
            if (seen_scalar_index || !is_full_unit_slice(section->m_args[i])) {
                return false;
            }
        }
        return true;
    }

    bool is_known_one_based_contiguous_whole_array(ASR::expr_t *expr) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return is_known_one_based_contiguous_section(ASR::down_cast<ASR::ArraySection_t>(expr));
        }
        ASR::symbol_t *sym = get_whole_array_var_symbol(expr);
        if (!sym) {
            return false;
        }
        if (one_based_contiguous_allocs.count(sym) > 0) {
            return true;
        }
        return is_variable_declared_contiguous(sym)
            && array_type_has_default_lower_bounds(ASRUtils::symbol_type(sym));
    }

    bool is_whole_dummy_array_variable(ASR::expr_t *expr) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (!ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        if (!ASR::is_a<ASR::Variable_t>(*sym)) {
            return false;
        }
        ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(sym);
        return var->m_intent == ASR::intentType::In ||
            var->m_intent == ASR::intentType::Out ||
            var->m_intent == ASR::intentType::InOut ||
            var->m_intent == ASR::intentType::Unspecified;
    }

    bool is_c_plain_scalar_array_element_type(ASR::ttype_t *type) {
        if (type == nullptr) {
            return false;
        }
        return ASRUtils::is_integer(*type)
            || ASRUtils::is_unsigned_integer(*type)
            || ASRUtils::is_real(*type)
            || ASRUtils::is_logical(*type);
    }

    bool is_c_simple_integer_bound_expr(ASR::expr_t *expr) {
        if (expr == nullptr) {
            return true;
        }
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::IntegerConstant:
            case ASR::exprType::Var:
            case ASR::exprType::ArrayBound: {
                return true;
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop =
                    ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return is_c_simple_integer_bound_expr(binop->m_left)
                    && is_c_simple_integer_bound_expr(binop->m_right);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_simple_integer_bound_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_simple_array_section_view(ASR::ArraySection_t *section) {
        if (section == nullptr) {
            return false;
        }
        ASR::expr_t *base =
            ASRUtils::get_past_array_physical_cast(section->m_v);
        ASR::ttype_t *base_type = base != nullptr
            ? ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(base))
            : nullptr;
        int base_rank = ASRUtils::extract_n_dims_from_ttype(base_type);
        if (base_rank <= 0 || section->n_args != static_cast<size_t>(base_rank)) {
            return false;
        }
        bool found_slice = false;
        for (int i = 0; i < base_rank; i++) {
            const ASR::array_index_t &idx = section->m_args[i];
            bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
            if (!is_slice) {
                if (!is_scalar_section_index(idx)
                        || !is_c_simple_integer_bound_expr(idx.m_right)) {
                    return false;
                }
                continue;
            }
            if (!is_c_simple_integer_bound_expr(idx.m_left)
                    || !is_c_simple_integer_bound_expr(idx.m_right)
                    || !is_c_simple_integer_bound_expr(idx.m_step)) {
                return false;
            }
            found_slice = true;
        }
        return found_slice;
    }

    bool should_preserve_c_no_copy_section_actual(
            ASR::expr_t *dummy, ASR::expr_t *actual) {
        ASR::expr_t *actual_unwrapped = actual
            ? ASRUtils::get_past_array_physical_cast(actual) : nullptr;
        if (!pass_options.c_backend || dummy == nullptr || actual == nullptr
                || !ASRUtils::is_array(ASRUtils::expr_type(dummy))
                || actual_unwrapped == nullptr
                || !ASRUtils::is_array(ASRUtils::expr_type(actual_unwrapped))) {
            return false;
        }
        ASR::Variable_t *dummy_var = ASRUtils::expr_to_variable_or_null(dummy);
        if (dummy_var == nullptr
                || dummy_var->m_contiguous_attr
                || ASRUtils::is_allocatable(dummy_var->m_type)
                || ASRUtils::is_pointer(dummy_var->m_type)) {
            return false;
        }
        ASR::array_physical_typeType dummy_physical_type =
            ASRUtils::extract_physical_type(dummy_var->m_type);
        if (dummy_physical_type != ASR::array_physical_typeType::DescriptorArray
                && dummy_physical_type != ASR::array_physical_typeType::PointerArray) {
            return false;
        }
        if (actual_unwrapped == nullptr
                || !ASR::is_a<ASR::ArraySection_t>(*actual_unwrapped)) {
            return false;
        }
        ASR::ArraySection_t *section =
            ASR::down_cast<ASR::ArraySection_t>(actual_unwrapped);
        if (ASRUtils::is_array_indexed_with_array_indices(section)
                || !is_c_simple_array_section_view(section)) {
            return false;
        }
        ASR::expr_t *base =
            ASRUtils::get_past_array_physical_cast(section->m_v);
        if (base == nullptr
                || (!ASR::is_a<ASR::Var_t>(*base)
                    && !ASR::is_a<ASR::StructInstanceMember_t>(*base))) {
            return false;
        }
        ASR::ttype_t *dummy_type =
            ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(dummy));
        ASR::ttype_t *actual_type =
            ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(actual_unwrapped));
        if (dummy_type == nullptr || actual_type == nullptr
                || ASRUtils::extract_n_dims_from_ttype(dummy_type)
                    != ASRUtils::extract_n_dims_from_ttype(actual_type)
                || !is_c_plain_scalar_array_element_type(
                    ASRUtils::type_get_past_array(dummy_type))
                || !is_c_plain_scalar_array_element_type(
                    ASRUtils::type_get_past_array(actual_type))) {
            return false;
        }
        return ASRUtils::types_equal(
            ASRUtils::type_get_past_array(dummy_type),
            ASRUtils::type_get_past_array(actual_type), nullptr, nullptr);
    }

    bool is_c_raw_helper_scalar_type(ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_allocatable_pointer(type);
        return type != nullptr
            && !ASRUtils::is_pointer(type)
            && !ASRUtils::is_allocatable(type)
            && (ASRUtils::is_integer(*type)
                || ASRUtils::is_unsigned_integer(*type)
                || ASRUtils::is_real(*type)
                || ASRUtils::is_logical(*type));
    }

    bool is_c_raw_helper_array_arg(const ASR::Variable_t *arg) {
        if (!pass_options.c_backend || arg == nullptr
                || !ASRUtils::is_arg_dummy(arg->m_intent)
                || arg->m_presence == ASR::presenceType::Optional
                || arg->m_value_attr || arg->m_target_attr
                || ASRUtils::is_allocatable(arg->m_type)
                || ASRUtils::is_pointer(arg->m_type)) {
            return false;
        }
        ASR::ttype_t *arg_type =
            ASRUtils::type_get_past_allocatable_pointer(arg->m_type);
        if (arg_type == nullptr || !ASR::is_a<ASR::Array_t>(*arg_type)) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(arg_type);
        if (element_type == nullptr
                || !(ASRUtils::is_integer(*element_type)
                    || ASRUtils::is_unsigned_integer(*element_type)
                    || ASRUtils::is_real(*element_type)
                    || ASRUtils::is_logical(*element_type))) {
            return false;
        }
        ASR::Array_t *array_type = ASR::down_cast<ASR::Array_t>(arg_type);
        return array_type->m_physical_type
            == ASR::array_physical_typeType::UnboundedPointerArray;
    }

    bool is_c_raw_helper_scalar_arg(const ASR::Variable_t *arg) {
        ASR::ttype_t *arg_type_unwrapped = arg == nullptr ? nullptr
            : ASRUtils::type_get_past_allocatable_pointer(arg->m_type);
        if (!pass_options.c_backend || arg == nullptr
                || arg_type_unwrapped == nullptr
                || !ASRUtils::is_arg_dummy(arg->m_intent)
                || arg->m_presence == ASR::presenceType::Optional
                || arg->m_value_attr || arg->m_target_attr
                || ASRUtils::is_array(arg->m_type)
                || ASRUtils::is_allocatable(arg->m_type)
                || ASRUtils::is_pointer(arg->m_type)
                || ASRUtils::is_character(*arg_type_unwrapped)
                || ASRUtils::is_aggregate_type(arg->m_type)
                || ASRUtils::is_class_type(arg->m_type)) {
            return false;
        }
        return is_c_raw_helper_scalar_type(arg->m_type);
    }

    ASR::Variable_t *get_c_raw_helper_arg_var(ASR::Function_t *func,
            size_t i) {
        if (func == nullptr || i >= func->n_args || func->m_args[i] == nullptr
                || !ASR::is_a<ASR::Var_t>(*func->m_args[i])) {
            return nullptr;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(func->m_args[i])->m_v);
        if (sym == nullptr || !ASR::is_a<ASR::Variable_t>(*sym)) {
            return nullptr;
        }
        return ASR::down_cast<ASR::Variable_t>(sym);
    }

    bool is_c_private_or_internal_source_subroutine(ASR::Function_t *func) {
        if (func == nullptr || std::string(func->m_name) == "main") {
            return false;
        }
        ASR::FunctionType_t *func_type =
            ASR::down_cast<ASR::FunctionType_t>(func->m_function_signature);
        if (func_type->m_abi != ASR::abiType::Source
                || func_type->m_deftype != ASR::deftypeType::Implementation
                || func->m_return_var != nullptr) {
            return false;
        }
        if (func->m_access == ASR::accessType::Private) {
            return true;
        }
        SymbolTable *parent = ASRUtils::symbol_parent_symtab(
            reinterpret_cast<ASR::symbol_t*>(func));
        ASR::asr_t *owner = parent ? parent->asr_owner : nullptr;
        if (owner == nullptr || !ASR::is_a<ASR::symbol_t>(*owner)) {
            return false;
        }
        ASR::symbol_t *owner_sym = ASR::down_cast<ASR::symbol_t>(owner);
        return ASR::is_a<ASR::Function_t>(*owner_sym)
            || ASR::is_a<ASR::Program_t>(*owner_sym);
    }

    ASR::expr_t *get_array_index_expr(const ASR::array_index_t &idx) {
        if (idx.m_right) {
            return idx.m_right;
        }
        if (idx.m_left) {
            return idx.m_left;
        }
        return nullptr;
    }

    bool is_c_raw_helper_raw_var_expr(ASR::expr_t *expr,
            const std::set<ASR::symbol_t*> &raw_arg_symbols) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::Var_t>(*expr)) {
            return false;
        }
        ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(
            ASR::down_cast<ASR::Var_t>(expr)->m_v);
        return raw_arg_symbols.find(sym) != raw_arg_symbols.end();
    }

    bool can_specialize_c_raw_helper_body(ASR::Function_t *func,
            const std::vector<size_t> &raw_positions) {
        if (func == nullptr || raw_positions.empty()) {
            return false;
        }
        std::set<ASR::symbol_t*> raw_arg_symbols;
        for (size_t pos: raw_positions) {
            ASR::Variable_t *arg = get_c_raw_helper_arg_var(func, pos);
            if (arg == nullptr) {
                return false;
            }
            raw_arg_symbols.insert(ASRUtils::symbol_get_past_external(
                reinterpret_cast<ASR::symbol_t*>(arg)));
        }

        struct RawArrayUseVerifier:
                public ASR::BaseWalkVisitor<RawArrayUseVerifier> {
            CallVisitor &parent;
            const std::set<ASR::symbol_t*> &raw_arg_symbols;
            bool ok = true;

            RawArrayUseVerifier(CallVisitor &parent,
                    const std::set<ASR::symbol_t*> &raw_arg_symbols):
                parent(parent), raw_arg_symbols(raw_arg_symbols) {}

            bool is_raw_var(ASR::expr_t *expr) {
                return parent.is_c_raw_helper_raw_var_expr(
                    expr, raw_arg_symbols);
            }

            void visit_Var(const ASR::Var_t &x) {
                ASR::symbol_t *sym =
                    ASRUtils::symbol_get_past_external(x.m_v);
                if (raw_arg_symbols.find(sym) != raw_arg_symbols.end()) {
                    ok = false;
                }
            }

            void visit_ArrayItem(const ASR::ArrayItem_t &x) {
                if (!ok) {
                    return;
                }
                if (!is_raw_var(x.m_v)) {
                    ASR::BaseWalkVisitor<RawArrayUseVerifier>::
                        visit_ArrayItem(x);
                    return;
                }
                if (ASRUtils::is_array(x.m_type)) {
                    ok = false;
                    return;
                }
                for (size_t i = 0; i < x.n_args; i++) {
                    ASR::expr_t *idx_expr =
                        parent.get_array_index_expr(x.m_args[i]);
                    if (idx_expr == nullptr
                            || ASRUtils::is_array(
                                ASRUtils::expr_type(idx_expr))) {
                        ok = false;
                        return;
                    }
                    this->visit_expr(*idx_expr);
                }
            }

            void visit_ArraySection(const ASR::ArraySection_t &x) {
                if (!ok) {
                    return;
                }
                if (!is_raw_var(x.m_v)) {
                    ASR::BaseWalkVisitor<RawArrayUseVerifier>::
                        visit_ArraySection(x);
                    return;
                }
                if (!ASRUtils::is_array(x.m_type)) {
                    ok = false;
                    return;
                }
                for (size_t i = 0; i < x.n_args; i++) {
                    ASR::array_index_t idx = x.m_args[i];
                    if (idx.m_left != nullptr) {
                        if (ASRUtils::is_array(
                                ASRUtils::expr_type(idx.m_left))) {
                            ok = false;
                            return;
                        }
                        this->visit_expr(*idx.m_left);
                    }
                    if (idx.m_right != nullptr) {
                        if (ASRUtils::is_array(
                                ASRUtils::expr_type(idx.m_right))) {
                            ok = false;
                            return;
                        }
                        this->visit_expr(*idx.m_right);
                    }
                    if (idx.m_step != nullptr) {
                        if (ASRUtils::is_array(
                                ASRUtils::expr_type(idx.m_step))) {
                            ok = false;
                            return;
                        }
                        this->visit_expr(*idx.m_step);
                    }
                }
            }

            void visit_ArraySize(const ASR::ArraySize_t &x) {
                if (!ok) {
                    return;
                }
                if (!is_raw_var(x.m_v)) {
                    ASR::BaseWalkVisitor<RawArrayUseVerifier>::
                        visit_ArraySize(x);
                    return;
                }
                if (x.m_dim != nullptr) {
                    this->visit_expr(*x.m_dim);
                }
            }

            void visit_ArrayBound(const ASR::ArrayBound_t &x) {
                if (!ok) {
                    return;
                }
                if (!is_raw_var(x.m_v)) {
                    ASR::BaseWalkVisitor<RawArrayUseVerifier>::
                        visit_ArrayBound(x);
                    return;
                }
                if (x.m_dim != nullptr) {
                    this->visit_expr(*x.m_dim);
                }
            }
        };

        RawArrayUseVerifier verifier(*this, raw_arg_symbols);
        for (size_t i = 0; i < func->n_body && verifier.ok; i++) {
            verifier.visit_stmt(*func->m_body[i]);
        }
        return verifier.ok;
    }

    bool get_c_raw_helper_args(ASR::Function_t *func,
            std::vector<size_t> &raw_positions) {
        raw_positions.clear();
        if (!pass_options.c_backend
                || !is_c_private_or_internal_source_subroutine(func)) {
            return false;
        }
        for (size_t i = 0; i < func->n_args; i++) {
            ASR::Variable_t *arg = get_c_raw_helper_arg_var(func, i);
            if (arg == nullptr) {
                return false;
            }
            if (is_c_raw_helper_array_arg(arg)) {
                raw_positions.push_back(i);
                continue;
            }
            if (!is_c_raw_helper_scalar_arg(arg)) {
                return false;
            }
        }
        return can_specialize_c_raw_helper_body(func, raw_positions);
    }

    bool is_c_raw_helper_array_actual(ASR::Variable_t *formal,
            ASR::expr_t *actual) {
        ASR::expr_t *actual_no_cast =
            actual == nullptr ? nullptr
            : ASRUtils::get_past_array_physical_cast(actual);
        if (!is_c_raw_helper_array_arg(formal) || actual_no_cast == nullptr
                || ASR::is_a<ASR::FunctionParam_t>(*actual_no_cast)) {
            return false;
        }
        ASR::expr_t *unwrapped = actual_no_cast;
        ASR::ArrayItem_t *item = ASR::is_a<ASR::ArrayItem_t>(*unwrapped)
            ? ASR::down_cast<ASR::ArrayItem_t>(unwrapped) : nullptr;
        ASR::ArraySection_t *section =
            ASR::is_a<ASR::ArraySection_t>(*unwrapped)
            ? ASR::down_cast<ASR::ArraySection_t>(unwrapped) : nullptr;
        if (item == nullptr && section == nullptr) {
            return false;
        }
        ASR::expr_t *base = item != nullptr ? item->m_v : section->m_v;
        ASR::ttype_t *base_type =
            ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(base));
        ASR::ttype_t *formal_type =
            ASRUtils::type_get_past_allocatable_pointer(formal->m_type);
        if (base_type == nullptr || formal_type == nullptr) {
            return false;
        }
        int base_rank = ASRUtils::extract_n_dims_from_ttype(base_type);
        int formal_rank = ASRUtils::extract_n_dims_from_ttype(formal_type);
        if (base_rank <= 0 || formal_rank <= 0 || base_rank != formal_rank
                || (item != nullptr
                    && item->n_args != static_cast<size_t>(base_rank))
                || (section != nullptr
                    && section->n_args != static_cast<size_t>(base_rank))) {
            return false;
        }
        const ASR::array_index_t *indices =
            item != nullptr ? item->m_args : section->m_args;
        for (int i = 0; i < base_rank; i++) {
            if (!is_integer_constant_one(indices[i].m_step)) {
                return false;
            }
            ASR::expr_t *left = indices[i].m_left;
            ASR::expr_t *right = indices[i].m_right;
            if ((left != nullptr && ASRUtils::is_array(ASRUtils::expr_type(left)))
                    || (right != nullptr
                        && ASRUtils::is_array(ASRUtils::expr_type(right)))) {
                return false;
            }
        }
        return true;
    }

    std::vector<bool> get_c_raw_helper_call_temp_bypass(ASR::Function_t *func,
            ASR::call_arg_t *args, size_t n_args) {
        std::vector<bool> bypass(n_args, false);
        std::vector<size_t> raw_positions;
        if (func == nullptr || n_args != func->n_args
                || !get_c_raw_helper_args(func, raw_positions)) {
            return bypass;
        }
        for (size_t pos: raw_positions) {
            ASR::Variable_t *formal = get_c_raw_helper_arg_var(func, pos);
            if (args[pos].m_value == nullptr
                    || !is_c_raw_helper_array_actual(
                        formal, args[pos].m_value)) {
                return std::vector<bool>(n_args, false);
            }
            bypass[pos] = true;
        }
        return bypass;
    }

    bool is_c_inlineable_dot_product_array_arg(ASR::Function_t *func,
            size_t pos) {
        if (!pass_options.c_backend || func == nullptr || pos >= func->n_args
                || func->m_args[pos] == nullptr
                || !ASR::is_a<ASR::Var_t>(*func->m_args[pos])) {
            return false;
        }
        ASR::Variable_t *formal = ASRUtils::EXPR2VAR(func->m_args[pos]);
        if (formal->m_intent != ASR::intentType::In) {
            return false;
        }
        ASR::ttype_t *formal_type =
            ASRUtils::type_get_past_allocatable_pointer(formal->m_type);
        if (formal_type == nullptr || !ASRUtils::is_array(formal_type)
                || ASRUtils::extract_n_dims_from_ttype(formal_type) != 1) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::type_get_past_array(formal_type));
        return element_type != nullptr
            && (ASRUtils::is_real(*element_type)
                || ASRUtils::is_integer(*element_type));
    }

    bool is_c_inlineable_dot_product_helper(ASR::Function_t *func,
            std::vector<size_t> &array_positions) {
        array_positions.clear();
        if (!pass_options.c_backend || func == nullptr
                || (func->n_args != 2 && func->n_args != 4)
                || std::string(func->m_name).find("lcompilers_dot_product")
                    == std::string::npos
                || func->m_return_var == nullptr) {
            return false;
        }
        ASR::ttype_t *return_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(func->m_return_var));
        if (return_type == nullptr
                || !(ASRUtils::is_real(*return_type)
                    || ASRUtils::is_integer(*return_type))) {
            return false;
        }
        size_t rhs_pos = func->n_args == 4 ? 2 : 1;
        if (!is_c_inlineable_dot_product_array_arg(func, 0)
                || !is_c_inlineable_dot_product_array_arg(func, rhs_pos)) {
            return false;
        }
        array_positions.push_back(0);
        array_positions.push_back(rhs_pos);
        return true;
    }

    bool is_c_inlineable_dot_product_array_actual(ASR::expr_t *actual) {
        if (actual == nullptr
                || !is_descriptor_array_casted_to_pointer_to_data(actual)) {
            return false;
        }
        ASR::expr_t *actual_no_cast =
            ASRUtils::get_past_array_physical_cast(actual);
        return actual_no_cast != nullptr
            && ASR::is_a<ASR::ArraySection_t>(*actual_no_cast);
    }

    std::vector<bool> get_c_inlineable_dot_product_call_temp_bypass(
            ASR::Function_t *func, ASR::call_arg_t *args, size_t n_args) {
        std::vector<bool> bypass(n_args, false);
        std::vector<size_t> array_positions;
        if (!is_c_inlineable_dot_product_helper(func, array_positions)
                || n_args != func->n_args) {
            return bypass;
        }
        for (size_t pos: array_positions) {
            if (pos < n_args
                    && is_c_inlineable_dot_product_array_actual(
                        args[pos].m_value)) {
                bypass[pos] = true;
            }
        }
        return bypass;
    }

    bool is_c_rank2_scalarizable_sum_actual_expr(ASR::expr_t *expr) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *array_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        ASR::ttype_t *element_type = array_type != nullptr
            ? ASRUtils::type_get_past_array(array_type) : nullptr;
        if (array_type == nullptr || element_type == nullptr
                || !ASRUtils::is_array(array_type)
                || ASRUtils::extract_n_dims_from_ttype(array_type) != 2
                || !(ASRUtils::is_real(*element_type)
                    || ASRUtils::is_integer(*element_type)
                    || ASRUtils::is_unsigned_integer(*element_type))) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember: {
                return true;
            }
            case ASR::exprType::ArraySection: {
                return is_c_simple_array_section_view(
                    ASR::down_cast<ASR::ArraySection_t>(expr));
            }
            case ASR::exprType::ArrayBroadcast: {
                return is_c_rank2_scalarizable_sum_actual_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_scalarizable_sum_actual_expr(binop->m_left)
                    && is_c_rank2_scalarizable_sum_actual_expr(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop =
                    ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_scalarizable_sum_actual_expr(binop->m_left)
                    && is_c_rank2_scalarizable_sum_actual_expr(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank2_scalarizable_sum_actual_expr(binop->m_left)
                    && is_c_rank2_scalarizable_sum_actual_expr(binop->m_right);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_rank2_scalarizable_sum_actual_expr(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_rank2_scalarizable_sum_actual_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_inlineable_sum_helper(ASR::Function_t *func) {
        if (!pass_options.c_backend || func == nullptr
                || func->n_args == 0 || func->m_return_var == nullptr) {
            return false;
        }
        std::string func_name = std::string(func->m_name);
        if (func_name.find("lcompilers_Sum") == std::string::npos
                && func_name.find("lcompilers_sum") == std::string::npos) {
            return false;
        }
        ASR::ttype_t *return_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(func->m_return_var));
        return return_type != nullptr
            && !ASRUtils::is_array(return_type)
            && (ASRUtils::is_real(*return_type)
                || ASRUtils::is_integer(*return_type)
                || ASRUtils::is_unsigned_integer(*return_type));
    }

    bool is_c_rank1_scalarizable_reduction_expr(ASR::expr_t *expr) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || !ASRUtils::is_array(type)) {
            return true;
        }
        if (ASRUtils::extract_n_dims_from_ttype(type) != 1) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection:
            case ASR::exprType::ArrayItem:
            case ASR::exprType::ArrayConstant:
            case ASR::exprType::ArrayConstructor: {
                return true;
            }
            case ASR::exprType::ArrayBroadcast: {
                return is_c_rank1_scalarizable_reduction_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank1_scalarizable_reduction_expr(binop->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop =
                    ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank1_scalarizable_reduction_expr(binop->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_rank1_scalarizable_reduction_expr(binop->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(binop->m_right);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_rank1_scalarizable_reduction_expr(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_rank1_scalarizable_reduction_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerCompare: {
                ASR::IntegerCompare_t *cmp = ASR::down_cast<ASR::IntegerCompare_t>(expr);
                return is_c_rank1_scalarizable_reduction_expr(cmp->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(cmp->m_right);
            }
            case ASR::exprType::UnsignedIntegerCompare: {
                ASR::UnsignedIntegerCompare_t *cmp =
                    ASR::down_cast<ASR::UnsignedIntegerCompare_t>(expr);
                return is_c_rank1_scalarizable_reduction_expr(cmp->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(cmp->m_right);
            }
            case ASR::exprType::RealCompare: {
                ASR::RealCompare_t *cmp = ASR::down_cast<ASR::RealCompare_t>(expr);
                return is_c_rank1_scalarizable_reduction_expr(cmp->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(cmp->m_right);
            }
            case ASR::exprType::LogicalCompare: {
                ASR::LogicalCompare_t *cmp = ASR::down_cast<ASR::LogicalCompare_t>(expr);
                return is_c_rank1_scalarizable_reduction_expr(cmp->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(cmp->m_right);
            }
            case ASR::exprType::LogicalBinOp: {
                ASR::LogicalBinOp_t *binop = ASR::down_cast<ASR::LogicalBinOp_t>(expr);
                return is_c_rank1_scalarizable_reduction_expr(binop->m_left)
                    && is_c_rank1_scalarizable_reduction_expr(binop->m_right);
            }
            case ASR::exprType::LogicalNot: {
                return is_c_rank1_scalarizable_reduction_expr(
                    ASR::down_cast<ASR::LogicalNot_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_scalarizable_logical_reduction_operand_expr(ASR::expr_t *expr) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || !ASRUtils::is_array(type)) {
            return true;
        }
        int rank = ASRUtils::extract_n_dims_from_ttype(type);
        if (rank == 1) {
            return is_c_rank1_scalarizable_reduction_expr(expr);
        }
        if (rank != 2) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(type);
        if (element_type != nullptr) {
            ASR::ttype_t *element_type_past_alloc =
                ASRUtils::type_get_past_allocatable_pointer(element_type);
            if (ASRUtils::is_real(*element_type_past_alloc)
                    || ASRUtils::is_integer(*element_type_past_alloc)
                    || ASRUtils::is_unsigned_integer(*element_type_past_alloc)) {
                return is_c_rank2_scalarizable_sum_actual_expr(expr);
            }
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember: {
                return true;
            }
            case ASR::exprType::ArraySection: {
                return is_c_simple_array_section_view(
                    ASR::down_cast<ASR::ArraySection_t>(expr));
            }
            case ASR::exprType::ArrayBroadcast: {
                return is_c_scalarizable_logical_reduction_operand_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array);
            }
            case ASR::exprType::LogicalCompare: {
                ASR::LogicalCompare_t *cmp = ASR::down_cast<ASR::LogicalCompare_t>(expr);
                return is_c_scalarizable_logical_reduction_operand_expr(cmp->m_left)
                    && is_c_scalarizable_logical_reduction_operand_expr(cmp->m_right);
            }
            case ASR::exprType::LogicalBinOp: {
                ASR::LogicalBinOp_t *binop = ASR::down_cast<ASR::LogicalBinOp_t>(expr);
                return is_c_scalarizable_logical_reduction_operand_expr(binop->m_left)
                    && is_c_scalarizable_logical_reduction_operand_expr(binop->m_right);
            }
            case ASR::exprType::LogicalNot: {
                return is_c_scalarizable_logical_reduction_operand_expr(
                    ASR::down_cast<ASR::LogicalNot_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_scalarizable_logical_reduction_mask_expr(ASR::expr_t *expr) {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || !ASRUtils::is_array(type)) {
            return true;
        }
        int rank = ASRUtils::extract_n_dims_from_ttype(type);
        if (rank != 1 && rank != 2) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::type_get_past_array(type);
        if (element_type == nullptr) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection:
            case ASR::exprType::ArrayItem:
            case ASR::exprType::ArrayConstant:
            case ASR::exprType::ArrayConstructor: {
                return ASRUtils::is_logical(
                    *ASRUtils::type_get_past_allocatable_pointer(element_type));
            }
            case ASR::exprType::IntegerCompare: {
                ASR::IntegerCompare_t *cmp = ASR::down_cast<ASR::IntegerCompare_t>(expr);
                return is_c_scalarizable_logical_reduction_operand_expr(cmp->m_left)
                    && is_c_scalarizable_logical_reduction_operand_expr(cmp->m_right);
            }
            case ASR::exprType::UnsignedIntegerCompare: {
                ASR::UnsignedIntegerCompare_t *cmp =
                    ASR::down_cast<ASR::UnsignedIntegerCompare_t>(expr);
                return is_c_scalarizable_logical_reduction_operand_expr(cmp->m_left)
                    && is_c_scalarizable_logical_reduction_operand_expr(cmp->m_right);
            }
            case ASR::exprType::RealCompare: {
                ASR::RealCompare_t *cmp = ASR::down_cast<ASR::RealCompare_t>(expr);
                return is_c_scalarizable_logical_reduction_operand_expr(cmp->m_left)
                    && is_c_scalarizable_logical_reduction_operand_expr(cmp->m_right);
            }
            case ASR::exprType::LogicalCompare: {
                ASR::LogicalCompare_t *cmp = ASR::down_cast<ASR::LogicalCompare_t>(expr);
                return is_c_scalarizable_logical_reduction_operand_expr(cmp->m_left)
                    && is_c_scalarizable_logical_reduction_operand_expr(cmp->m_right);
            }
            case ASR::exprType::LogicalBinOp: {
                ASR::LogicalBinOp_t *binop = ASR::down_cast<ASR::LogicalBinOp_t>(expr);
                return is_c_scalarizable_logical_reduction_mask_expr(binop->m_left)
                    && is_c_scalarizable_logical_reduction_mask_expr(binop->m_right);
            }
            case ASR::exprType::LogicalNot: {
                return is_c_scalarizable_logical_reduction_mask_expr(
                    ASR::down_cast<ASR::LogicalNot_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_inlineable_logical_reduction_helper(ASR::Function_t *func) {
        if (!pass_options.c_backend || func == nullptr
                || func->n_args == 0 || func->m_return_var == nullptr) {
            return false;
        }
        std::string func_name = std::string(func->m_name);
        bool is_any_or_all = func_name.find("lcompilers_Any") != std::string::npos
            || func_name.find("lcompilers_any") != std::string::npos
            || func_name.find("lcompilers_All") != std::string::npos
            || func_name.find("lcompilers_all") != std::string::npos;
        bool is_count = func_name.find("lcompilers_Count") != std::string::npos
            || func_name.find("lcompilers_count") != std::string::npos;
        if (!is_any_or_all && !is_count) {
            return false;
        }
        ASR::ttype_t *return_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(func->m_return_var));
        return return_type != nullptr
            && !ASRUtils::is_array(return_type)
            && ((is_any_or_all && ASRUtils::is_logical(*return_type))
                || (is_count && ASRUtils::is_integer(*return_type)));
    }

    std::vector<bool> get_c_inlineable_reduction_call_temp_bypass(
            ASR::Function_t *func, ASR::call_arg_t *args, size_t n_args) {
        std::vector<bool> bypass(n_args, false);
        if (n_args == 0
                || args == nullptr || args[0].m_value == nullptr) {
            return bypass;
        }
        if (!ASRUtils::is_array(ASRUtils::expr_type(args[0].m_value))) {
            return bypass;
        }
        if (is_c_inlineable_sum_helper(func)
                && is_c_rank2_scalarizable_sum_actual_expr(args[0].m_value)) {
            bypass[0] = true;
        } else if (is_c_inlineable_logical_reduction_helper(func)
                && is_c_scalarizable_logical_reduction_mask_expr(args[0].m_value)) {
            bypass[0] = true;
        }
        return bypass;
    }

    bool is_descriptor_array_casted_to_pointer_to_data( ASR::expr_t* expr ) {
        if ( ASRUtils::is_array(ASRUtils::expr_type(expr) ) &&
             ASR::is_a<ASR::ArrayPhysicalCast_t>(*expr) ) {
            ASR::ArrayPhysicalCast_t* cast = ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr);
            if ( !((cast->m_new == ASR::array_physical_typeType::PointerArray ||
                     cast->m_new == ASR::array_physical_typeType::UnboundedPointerArray) &&
                    (cast->m_old == ASR::array_physical_typeType::DescriptorArray ||
                     cast->m_old == ASR::array_physical_typeType::PointerArray)) ) {
                return false;
            }
            // Skip copy-in/copy-out when the inner expression must use Fortran
            // sequence association into the original storage.
            ASR::expr_t* inner = ASRUtils::get_past_array_physical_cast(cast->m_arg);
            if ( ASR::is_a<ASR::ArraySection_t>(*inner) ) {
                ASR::ArraySection_t* section = ASR::down_cast<ASR::ArraySection_t>(inner);
                if ( is_sequence_association_array_section(section) ) {
                    return false;
                }
            }
            // Also skip when the inner Var was created by an earlier pass
            // (pass_array_by_data) from an ArraySection of an assumed-size array.
            if ( ASR::is_a<ASR::Var_t>(*inner) ) {
                ASR::symbol_t* sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(inner)->m_v);
                if ( vars_from_assumed_size_sections.count(sym)
                        || vars_from_c_no_copy_section_views.count(sym) ) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    bool is_explicit_shape_array_type(ASR::ttype_t *type) {
        type = ASRUtils::type_get_past_allocatable_pointer(type);
        if (!type || !ASRUtils::is_array(type)) {
            return false;
        }
        ASR::dimension_t *dims = nullptr;
        int rank = ASRUtils::extract_dimensions_from_ttype(type, dims);
        if (rank <= 0) {
            return false;
        }
        for (int i = 0; i < rank; i++) {
            if (dims[i].m_start == nullptr || dims[i].m_length == nullptr) {
                return false;
            }
        }
        return true;
    }

    bool is_sequence_association_array_section(ASR::ArraySection_t *section) {
        ASR::ttype_t* source_type = ASRUtils::expr_type(section->m_v);
        if (!ASRUtils::is_array(source_type)) {
            return false;
        }
        ASR::array_physical_typeType phys =
            ASRUtils::extract_physical_type(source_type);
        if (phys == ASR::array_physical_typeType::UnboundedPointerArray
                && has_undefined_assumed_size_bound(section)) {
            return true;
        }
        if (phys == ASR::array_physical_typeType::FixedSizeArray) {
            return true;
        }
        return (phys == ASR::array_physical_typeType::PointerArray
                || phys == ASR::array_physical_typeType::DescriptorArray)
            && is_explicit_shape_array_type(source_type);
    }

    void transform_stmts(ASR::stmt_t**& m_body, size_t& n_body) {
        is_current_body_set++;
        transform_stmts_impl(al, m_body, n_body, current_body, body_after_curr_stmt,
            [this](const ASR::stmt_t& stmt) { visit_stmt(stmt); });
        is_current_body_set--;
    }

    template <typename T>
    ASR::expr_t* get_first_array_function_args(T* func) {
        int64_t first_array_arg_idx = -1;
        ASR::expr_t* first_array_arg = nullptr;
        for (int64_t i = 0; i < (int64_t)func->n_args; i++) {
            ASR::ttype_t* func_arg_type;
            if constexpr (std::is_same_v<T, ASR::FunctionCall_t>) {
                func_arg_type = ASRUtils::expr_type(func->m_args[i].m_value);
            } else {
                func_arg_type = ASRUtils::expr_type(func->m_args[i]);
            }
            if (ASRUtils::is_array(func_arg_type)) {
                first_array_arg_idx = i;
                break;
            }
        }
        LCOMPILERS_ASSERT(first_array_arg_idx != -1)
        if constexpr (std::is_same_v<T, ASR::FunctionCall_t>) {
            first_array_arg = func->m_args[first_array_arg_idx].m_value;
        } else {
            first_array_arg = func->m_args[first_array_arg_idx];
        }
        return first_array_arg;
    }


    /*
        sets allocation size of an elemental function, which can be
        either an intrinsic elemental function or a user-defined
    */
    template <typename T>
    void set_allocation_size_elemental_function(
        Allocator& al, const Location& loc,
        T* elemental_function,
        Vec<ASR::dimension_t>& allocate_dims
    ) {
        ASR::ttype_t* index_type = get_index_type(loc);
        ASR::expr_t* index_one = get_index_one(loc);
        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(elemental_function->m_type);
        allocate_dims.reserve(al, n_dims);
        ASR::expr_t* first_array_arg = get_first_array_function_args(elemental_function);
        for( size_t i = 0; i < n_dims; i++ ) {
            ASR::dimension_t allocate_dim;
            allocate_dim.loc = loc;
            allocate_dim.m_start = index_one;
            ASR::expr_t* size_i_1 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                al, loc, ASRUtils::get_past_array_physical_cast(first_array_arg),
                get_index_constant(loc, i + 1),
                index_type, nullptr));
            allocate_dim.m_length = size_i_1;
            allocate_dims.push_back(al, allocate_dim);
        }
    }

    ASR::expr_t* create_temporary_variable_for_array(Allocator& al,
        ASR::expr_t* value, SymbolTable* scope, std::string name_hint,
        bool is_pointer_required=false) {
        ASR::ttype_t* value_type = ASRUtils::expr_type(value);
        LCOMPILERS_ASSERT(ASRUtils::is_array(value_type));
    
        /* Figure out the type of the temporary array variable */
        ASR::dimension_t* value_m_dims = nullptr;
        size_t value_n_dims = ASRUtils::extract_dimensions_from_ttype(value_type, value_m_dims);
    
        if (ASR::is_a<ASR::IntegerCompare_t>(*value)) {
            ASR::IntegerCompare_t* integer_compare = ASR::down_cast<ASR::IntegerCompare_t>(value);
            ASR::ttype_t* logical_type = ASRUtils::TYPE(ASR::make_Logical_t(al, value->base.loc, 4));
    
            ASR::ttype_t* left_type = ASRUtils::expr_type(integer_compare->m_left);
            ASR::ttype_t* right_type = ASRUtils::expr_type(integer_compare->m_right);
    
            if (ASR::is_a<ASR::Array_t>(*left_type)) {
                ASR::Array_t* left_array_type = ASR::down_cast<ASR::Array_t>(left_type);
                ASR::dimension_t* left_m_dims = nullptr;
                size_t left_n_dims = ASRUtils::extract_dimensions_from_ttype(left_type, left_m_dims);
                value_m_dims = left_m_dims;
                value_n_dims = left_n_dims;
    
                if (left_array_type->m_physical_type == ASR::array_physical_typeType::FixedSizeArray) {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, left_m_dims, left_n_dims, ASR::array_physical_typeType::FixedSizeArray));
                    value_type = logical_array_type;
                } else {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, left_m_dims, left_n_dims, ASR::array_physical_typeType::PointerArray));
                    value_type = logical_array_type;
                }
            } else if (ASR::is_a<ASR::Array_t>(*right_type)) {
                ASR::Array_t* right_array_type = ASR::down_cast<ASR::Array_t>(right_type);
                ASR::dimension_t* right_m_dims = nullptr;
                size_t right_n_dims = ASRUtils::extract_dimensions_from_ttype(right_type, right_m_dims);
                value_m_dims = right_m_dims;
                value_n_dims = right_n_dims;
    
                if (right_array_type->m_physical_type == ASR::array_physical_typeType::FixedSizeArray) {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, right_m_dims, right_n_dims, ASR::array_physical_typeType::FixedSizeArray));
                    value_type = logical_array_type;
                } else {
                    ASR::ttype_t* logical_array_type = ASRUtils::TYPE(ASR::make_Array_t(al, value->base.loc, logical_type, right_m_dims, right_n_dims, ASR::array_physical_typeType::PointerArray));
                    value_type = logical_array_type;
                }
            }
        }
        // dimensions can be different for an ArrayConstructor e.g. [1, a], where `a` is an
        // ArrayConstructor like [5, 2, 1]
        if (ASR::is_a<ASR::ArrayConstructor_t>(*value) &&
               !PassUtils::is_args_contains_allocatable(value)) {
            ASR::ArrayConstructor_t* arr_constructor = ASR::down_cast<ASR::ArrayConstructor_t>(value);
            value_m_dims->m_length = ASRUtils::get_ArrayConstructor_size(al, arr_constructor);
        }

        /*
            Handle character type with assumed length OR functioncall (to avoid duplicate calls).
            `character(*), intent(in) :: inp_char_arr(:)` --TempType--> `character(len(inp_char_arr)), intent(in) :: tmp(:)`
        */
        if(ASRUtils::is_character(*value_type)){
            if(ASRUtils::get_string_type(value_type)->m_len_kind == ASR::AssumedLength || 
                (ASRUtils::get_string_type(value_type)->m_len &&
                    ASR::is_a<ASR::FunctionCall_t>(*ASRUtils::get_string_type(value_type)->m_len)
                )
            ){
                ASRUtils::ASRBuilder b(al, value->base.loc);
                value_type = b.String(b.StringLen(value), ASR::ExpressionLength);             
            }
        }
        bool is_fixed_sized_array = ASRUtils::is_fixed_size_array(value_type);
        bool is_size_only_dependent_on_arguments = ASRUtils::is_dimension_dependent_only_on_arguments(
            value_m_dims, value_n_dims);
        bool is_allocatable = ASRUtils::is_allocatable(value_type);
        ASR::ttype_t* temporary_value_type = value_type;
        if (is_allocatable && (is_fixed_sized_array || is_size_only_dependent_on_arguments)) {
            temporary_value_type = ASRUtils::type_get_past_allocatable(value_type);
        }
        ASR::ttype_t* var_type = nullptr;
        if( (is_fixed_sized_array || is_size_only_dependent_on_arguments || is_allocatable) &&
            !is_pointer_required ) {
            var_type = temporary_value_type;
        } else {
            var_type = ASRUtils::create_array_type_with_empty_dims(al, value_n_dims, value_type);
            var_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, var_type->base.loc, var_type));
        }
    
        std::string var_name = scope->get_unique_name("__libasr_created_" + name_hint);
        ASR::symbol_t* temporary_variable = ASR::down_cast<ASR::symbol_t>(ASRUtils::make_Variable_t_util(
            al, value->base.loc, scope, s2c(al, var_name), nullptr, 0, ASR::intentType::Local,
            nullptr, nullptr, ASR::storage_typeType::Default, var_type, ASRUtils::get_struct_sym_from_struct_expr(value), ASR::abiType::Source,
            ASR::accessType::Public, ASR::presenceType::Required, false));
        scope->add_symbol(var_name, temporary_variable);
    
        return ASRUtils::EXPR(ASR::make_Var_t(al, temporary_variable->base.loc, temporary_variable));
    }

    bool set_allocation_size(
        Allocator& al, ASR::expr_t* value,
        Vec<ASR::dimension_t>& allocate_dims,
        size_t target_n_dims,
        ASR::expr_t* &len_expr /*allocate(character(len(x)) :: y(size(x)))*/
    ) {
        if ( !ASRUtils::is_array(ASRUtils::expr_type(value)) ) {
            return false;
        }

        const Location& loc = value->base.loc;
        ASR::ttype_t* index_type = get_index_type(loc);
        ASR::expr_t* index_one = get_index_one(loc);
        
        if( ASRUtils::is_fixed_size_array(ASRUtils::expr_type(value)) ) {
            ASR::dimension_t* m_dims = nullptr;
            size_t n_dims = ASRUtils::extract_dimensions_from_ttype(
                ASRUtils::expr_type(value), m_dims);
            allocate_dims.reserve(al, n_dims);
            for( size_t i = 0; i < n_dims; i++ ) {
                ASR::dimension_t allocate_dim;
                allocate_dim.loc = value->base.loc;
                allocate_dim.m_start = index_one;
                allocate_dim.m_length = m_dims[i].m_length;
                allocate_dims.push_back(al, allocate_dim);
            }
            return true;
        }
        if(ASRUtils::is_character(*ASRUtils::expr_type(value))){
            bool is_const_len = ASRUtils::is_value_constant(ASRUtils::get_string_type(value)->m_len);
            len_expr = ASRUtils::EXPR(ASR::make_StringLen_t(al,
                value->base.loc, value,
                index_type,
                is_const_len ? ASRUtils::get_string_type(value)->m_len : nullptr));
        }
        switch( value->type ) {
            case ASR::exprType::FunctionCall: {
                ASR::FunctionCall_t* function_call = ASR::down_cast<ASR::FunctionCall_t>(value);
                ASR::ttype_t* type = function_call->m_type;
                if( ASRUtils::is_allocatable(type) ) {
                    return false;
                }
                if (ASRUtils::is_elemental(function_call->m_name)) {
                    set_allocation_size_elemental_function(al, loc, function_call, allocate_dims);
                    break;
                }
                ASRUtils::ExprStmtDuplicator duplicator(al);
                ASR::dimension_t* dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(type, dims);
                allocate_dims.reserve(al, n_dims);
                for( size_t i = 0; i < n_dims; i++ ) {
                    ASR::dimension_t dim = dims[i];
                    ASR::dimension_t dim_copy;
                    dim_copy.loc = dim.loc;
                    dim_copy.m_start = !dim.m_start ? nullptr : duplicator.duplicate_expr(dim.m_start);
                    dim_copy.m_length = !dim.m_length ? nullptr : duplicator.duplicate_expr(dim.m_length);
                    LCOMPILERS_ASSERT(dim_copy.m_start);
                    LCOMPILERS_ASSERT(dim_copy.m_length);
                    allocate_dims.push_back(al, dim_copy);
                }
                break ;
            }
            case ASR::exprType::IntegerBinOp:
            case ASR::exprType::RealBinOp:
            case ASR::exprType::ComplexBinOp:
            case ASR::exprType::LogicalBinOp:
            case ASR::exprType::UnsignedIntegerBinOp:
            case ASR::exprType::IntegerCompare:
            case ASR::exprType::RealCompare:
            case ASR::exprType::ComplexCompare:
            case ASR::exprType::LogicalCompare:
            case ASR::exprType::UnsignedIntegerCompare:
            case ASR::exprType::StringCompare:
            case ASR::exprType::IntegerUnaryMinus:
            case ASR::exprType::RealUnaryMinus:
            case ASR::exprType::ComplexUnaryMinus: {
                /*
                    Collect all the variables from these expressions,
                    then take the size of one of the arrays having
                    maximum dimensions for now. For now LFortran will
                    assume that broadcasting is doable for arrays with lesser
                    dimensions and the array having maximum dimensions
                    has compatible size of each dimension with other arrays.
                */
    
                Vec<ASR::expr_t*> array_vars; array_vars.reserve(al, 1);
                ArrayVarCollector array_var_collector(al, array_vars);
                array_var_collector.visit_expr(*value);
                Vec<ASR::expr_t*> arrays_with_maximum_rank;
                arrays_with_maximum_rank.reserve(al, 1);
                LCOMPILERS_ASSERT(target_n_dims > 0);
                for( size_t i = 0; i < array_vars.size(); i++ ) {
                    if( (size_t) ASRUtils::extract_n_dims_from_ttype(
                            ASRUtils::expr_type(array_vars[i])) == target_n_dims ) {
                        arrays_with_maximum_rank.push_back(al, array_vars[i]);
                    }
                }
    
                if( arrays_with_maximum_rank.size() == 0 ) {
                    return false;
                }
                ASR::expr_t* selected_array = arrays_with_maximum_rank[0];
                allocate_dims.reserve(al, target_n_dims);
                for( size_t i = 0; i < target_n_dims; i++ ) {
                    ASR::dimension_t allocate_dim;
                    Location loc_inner; loc_inner.first = 1, loc_inner.last = 1;
                    allocate_dim.loc = loc_inner;
                    // Assume 1 for Fortran.
                    allocate_dim.m_start = get_index_one(loc_inner);
                    ASR::expr_t* dim = get_index_constant(loc_inner, i + 1);
                    allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                        al, loc_inner, ASRUtils::get_past_array_physical_cast(selected_array),
                        dim, index_type, nullptr));
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::LogicalNot: {
                ASR::LogicalNot_t* logical_not = ASR::down_cast<ASR::LogicalNot_t>(value);
                if ( ASRUtils::is_array(ASRUtils::expr_type(logical_not->m_arg)) ) {
                    size_t rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::expr_type(logical_not->m_arg));
                    ASR::expr_t* selected_array = logical_not->m_arg;
                    allocate_dims.reserve(al, rank);
                    for( size_t i = 0; i < rank; i++ ) {
                        ASR::dimension_t allocate_dim;
                        allocate_dim.loc = loc;
                        // Assume 1 for Fortran.
                        allocate_dim.m_start = index_one;
                        ASR::expr_t* dim = get_index_constant(loc, i + 1);
                        allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                            al, loc, ASRUtils::get_past_array_physical_cast(selected_array),
                            dim, index_type, nullptr));
                        allocate_dims.push_back(al, allocate_dim);
                    }
                }
                break;
            }
            case ASR::exprType::Cast: {
                ASR::Cast_t* cast = ASR::down_cast<ASR::Cast_t>(value);
                if ( ASRUtils::is_array(ASRUtils::expr_type(cast->m_arg)) ) {
                    size_t rank = ASRUtils::extract_n_dims_from_ttype(
                        ASRUtils::expr_type(cast->m_arg));
                    ASR::expr_t* selected_array = cast->m_arg;
                    allocate_dims.reserve(al, rank);
                    for( size_t i = 0; i < rank; i++ ) {
                        ASR::dimension_t allocate_dim;
                        allocate_dim.loc = loc;
                        // Assume 1 for Fortran.
                        allocate_dim.m_start = index_one;
                        ASR::expr_t* dim = get_index_constant(loc, i + 1);
                        allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                            al, loc, ASRUtils::get_past_array_physical_cast(selected_array),
                            dim, index_type, nullptr));
                        allocate_dims.push_back(al, allocate_dim);
                    }
                }
                break;
            }
            case ASR::exprType::ArraySection: {
                ASR::ArraySection_t* array_section_t = ASR::down_cast<ASR::ArraySection_t>(value);
                allocate_dims.reserve(al, array_section_t->n_args);
                for( size_t i = 0; i < array_section_t->n_args; i++ ) {
                    ASR::expr_t* start = array_section_t->m_args[i].m_left;
                    ASR::expr_t* end = array_section_t->m_args[i].m_right;
                    ASR::expr_t* step = array_section_t->m_args[i].m_step;
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = index_one;
                    if( start == nullptr && step == nullptr && end != nullptr ) {
                        if( ASRUtils::is_array(ASRUtils::expr_type(end)) ) {
                            allocate_dim.m_length = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(
                                al, loc, end, nullptr, index_type, nullptr, false));
                            allocate_dims.push_back(al, allocate_dim);
                        }
                    } else {
                        bool is_any_kind_8 = false;
                        ASR::expr_t * int_one = index_one;
                        if( ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(end)) == 8 ||
                            ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(start)) == 8 ||
                            ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(step)) == 8 ) {
                                is_any_kind_8 = true;
                        }
                        if( is_any_kind_8 || pass_options.descriptor_index_64 ) {
                            int_one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                al, loc, 1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8))));
                        }
                        ASR::expr_t* end_minus_start = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                            end, ASR::binopType::Sub, start, ASRUtils::expr_type(end), nullptr));
                        ASR::expr_t* by_step = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                            end_minus_start, ASR::binopType::Div, step, ASRUtils::expr_type(end_minus_start),
                            nullptr));
                        ASR::expr_t* length = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                            by_step, ASR::binopType::Add, int_one, ASRUtils::expr_type(by_step), nullptr));
                        allocate_dim.m_length = length;
                        allocate_dims.push_back(al, allocate_dim);
                    }
                }
                break;
            }
            case ASR::exprType::ArrayItem: {
                ASR::ArrayItem_t* array_item_t = ASR::down_cast<ASR::ArrayItem_t>(value);
                allocate_dims.reserve(al, array_item_t->n_args);
                for( size_t i = 0; i < array_item_t->n_args; i++ ) {
                    ASR::expr_t* start = array_item_t->m_args[i].m_left;
                    ASR::expr_t* end = array_item_t->m_args[i].m_right;
                    ASR::expr_t* step = array_item_t->m_args[i].m_step;
                    if( !(start == nullptr && step == nullptr && end != nullptr) ) {
                        continue ;
                    }
                    if( !ASRUtils::is_array(ASRUtils::expr_type(end)) ) {
                        continue ;
                    }
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = index_one;
                    allocate_dim.m_length = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(
                        al, loc, end, nullptr, index_type, nullptr, false));
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::IntrinsicElementalFunction: {
                ASR::IntrinsicElementalFunction_t* intrinsic_elemental_function =
                    ASR::down_cast<ASR::IntrinsicElementalFunction_t>(value);
                set_allocation_size_elemental_function(al, loc, intrinsic_elemental_function,
                            allocate_dims);
                break;
            }
            case ASR::exprType::IntrinsicArrayFunction: {
                ASR::IntrinsicArrayFunction_t* intrinsic_array_function =
                    ASR::down_cast<ASR::IntrinsicArrayFunction_t>(value);
                switch (intrinsic_array_function->m_arr_intrinsic_id) {
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::All):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Any):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Count):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Parity):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Sum):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::MaxVal):
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::MinVal): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(
                            intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for( size_t i = 0; i < n_dims; i++ ) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = index_one;
                            ASR::expr_t* size_i_1 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, ASRUtils::get_past_array_physical_cast(intrinsic_array_function->m_args[0]),
                                get_index_constant(loc, i + 1),
                                index_type, nullptr));
                            ASR::expr_t* size_i_2 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, ASRUtils::get_past_array_physical_cast(intrinsic_array_function->m_args[0]),
                                get_index_constant(loc, i + 2),
                                index_type, nullptr));
                            Vec<ASR::expr_t*> merge_i_args; merge_i_args.reserve(al, 3);
                            merge_i_args.push_back(al, size_i_1); merge_i_args.push_back(al, size_i_2);
                            merge_i_args.push_back(al, ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, loc,
                                get_index_constant(loc, i + 1), ASR::cmpopType::Lt,
                                    intrinsic_array_function->m_args[1],
                                    ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr)));
                            ASR::expr_t* merge_i = ASRUtils::EXPR(ASRUtils::make_IntrinsicElementalFunction_t_util(
                                al, loc, static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::Merge),
                                merge_i_args.p, merge_i_args.size(), 0, index_type, nullptr));
                            allocate_dim.m_length = merge_i;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Pack): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(
                            intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for ( size_t i = 0; i < n_dims; i++ ) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = index_one;
                            ASR::expr_t* size_i_1 = nullptr;
                            if (intrinsic_array_function->n_args == 3) {
                                size_i_1 = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                    al, loc, ASRUtils::get_past_array_physical_cast(intrinsic_array_function->m_args[2]),
                                    get_index_constant(loc, i + 1),
                                    index_type, nullptr));
                            } else {
                                Vec<ASR::expr_t*> count_i_args; count_i_args.reserve(al, 1);
                                count_i_args.push_back(al, intrinsic_array_function->m_args[1]);
                                size_i_1 = ASRUtils::EXPR(ASRUtils::make_IntrinsicArrayFunction_t_util(
                                    al, loc, static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Count),
                                    count_i_args.p, count_i_args.size(), 0, index_type, nullptr));
                            }
                            allocate_dim.m_length = size_i_1;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Shape): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(
                            intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for( size_t i = 0; i < n_dims; i++ ) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = index_one;
                            allocate_dim.m_length = index_one;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Transpose): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(intrinsic_array_function->m_type);
                        LCOMPILERS_ASSERT(n_dims == 2);
                        allocate_dims.reserve(al, n_dims);
                        // Transpose swaps the dimensions
                        for (size_t i = 0; i < n_dims; i++) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = index_one;
                            ASR::expr_t* size_i = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, intrinsic_array_function->m_args[0],
                                get_index_constant(loc, n_dims - i),
                                index_type, nullptr));

                            allocate_dim.m_length = size_i;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Cshift): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(intrinsic_array_function->m_type);
                        allocate_dims.reserve(al, n_dims);
                        for (size_t i = 0; i < n_dims; i++) {
                            ASR::dimension_t allocate_dim;
                            allocate_dim.loc = loc;
                            allocate_dim.m_start = index_one;

                            ASR::expr_t* size_i = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                al, loc, intrinsic_array_function->m_args[0],
                                get_index_constant(loc, i + 1),
                                index_type, nullptr));

                            allocate_dim.m_length = size_i;
                            allocate_dims.push_back(al, allocate_dim);
                        }
                        break;
                    }
                    case static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Spread): {
                        size_t n_dims = ASRUtils::extract_n_dims_from_ttype(intrinsic_array_function->m_type);
                        ASR::expr_t* dim_arg = intrinsic_array_function->m_args[1];
                        allocate_dims.reserve(al, n_dims);
                        if (dim_arg && (ASRUtils::expr_value(dim_arg) != nullptr)) {
                            // Compile time value of `dim`
                            ASRUtils::ASRBuilder b(al, intrinsic_array_function->base.base.loc);
                            ASR::IntegerConstant_t* dim_val = ASR::down_cast<ASR::IntegerConstant_t>(dim_arg);
                            size_t dim_index = dim_val->m_n;
                            ASR::expr_t* ncopies = intrinsic_array_function->m_args[2];
                            int ncopies_inserted = 0;
                            for (size_t i = 0; i < n_dims; i++) {
                                ASR::dimension_t allocate_dim;
                                allocate_dim.loc = loc;
                                allocate_dim.m_start = index_one;
                                if ( i == dim_index - 1 ) {
                                    allocate_dim.m_length = ncopies;
                                    ncopies_inserted = 1;
                                } else {
                                    allocate_dim.m_length = b.ArraySize(intrinsic_array_function->m_args[0],
                                            get_index_constant(loc, i + 1 - ncopies_inserted), index_type);
                                }
                                allocate_dims.push_back(al, allocate_dim);
                            }
                        } else {
                            // Here `dim` is runtime so can't decide where to insert ncopies
                            // Just copy original dimensions
                            ASR::dimension_t* dims;
                            ASRUtils::extract_dimensions_from_ttype(intrinsic_array_function->m_type, dims);
                            for (size_t i = 0; i < n_dims; i++) {
                                allocate_dims.push_back(al, dims[i]);
                            }
                        }
                        break;
                    }
    
                    default: {
                        LCOMPILERS_ASSERT_MSG(false, "ASR::IntrinsicArrayFunctions::" +
                            ASRUtils::get_array_intrinsic_name(intrinsic_array_function->m_arr_intrinsic_id)
                            + " not handled yet in set_allocation_size");
                    }
                }
                break;
            }
            case ASR::exprType::StructInstanceMember: {
                ASR::StructInstanceMember_t* struct_instance_member_t =
                    ASR::down_cast<ASR::StructInstanceMember_t>(value);
                size_t n_dims = ASRUtils::extract_n_dims_from_ttype(struct_instance_member_t->m_type);
                allocate_dims.reserve(al, n_dims);
                if( ASRUtils::is_array(ASRUtils::expr_type(struct_instance_member_t->m_v)) ) {
                    value = struct_instance_member_t->m_v;
                }
                ASRUtils::ExprStmtDuplicator expr_duplicator(al);
                for( size_t i = 0; i < n_dims; i++ ) {
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = index_one;
                    allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                        al, loc, expr_duplicator.duplicate_expr(
                            ASRUtils::get_past_array_physical_cast(value)),
                            get_index_constant(loc, i + 1),
                        index_type, nullptr));
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::ArrayReshape: {
                ASR::ArrayReshape_t* array_reshape_t = ASR::down_cast<ASR::ArrayReshape_t>(value);
                size_t n_dims = ASRUtils::get_fixed_size_of_array(
                    ASRUtils::expr_type(array_reshape_t->m_shape));
                allocate_dims.reserve(al, n_dims);
                ASRUtils::ASRBuilder b(al, array_reshape_t->base.base.loc);
                for( size_t i = 0; i < n_dims; i++ ) {
                    ASR::dimension_t allocate_dim;
                    allocate_dim.loc = loc;
                    allocate_dim.m_start = index_one;
                    allocate_dim.m_length = b.ArrayItem_01(array_reshape_t->m_shape, {get_index_constant(loc, i + 1)});
                    allocate_dims.push_back(al, allocate_dim);
                }
                break;
            }
            case ASR::exprType::ArrayConstructor: {
                allocate_dims.reserve(al, 1);
                ASR::dimension_t allocate_dim;
                allocate_dim.loc = loc;
                allocate_dim.m_start = index_one;
                allocate_dim.m_length = ASRUtils::get_ArrayConstructor_size(al,
                    ASR::down_cast<ASR::ArrayConstructor_t>(value));
                allocate_dims.push_back(al, allocate_dim);
                break;
            }
            case ASR::exprType::ArrayConstant: {
                allocate_dims.reserve(al, 1);
                ASR::dimension_t allocate_dim;
                allocate_dim.loc = loc;
                allocate_dim.m_start = index_one;
                allocate_dim.m_length = ASRUtils::get_ArrayConstant_size(al,
                    ASR::down_cast<ASR::ArrayConstant_t>(value));
                allocate_dims.push_back(al, allocate_dim);
                break;
            }
            case ASR::exprType::Var: {
                ASRUtils::ASRBuilder b(al, value->base.loc);
                if ( ASRUtils::is_array(ASRUtils::expr_type(value))) {
                    ASR::dimension_t* m_dims;
                    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(
                        ASRUtils::expr_type(value), m_dims);
                    allocate_dims.reserve(al, n_dims);
                    for( size_t i = 0; i < n_dims; i++ ) {
                        ASR::dimension_t allocate_dim;
                        allocate_dim.loc = loc;
                        allocate_dim.m_start = index_one;
                        allocate_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                            al, loc, value, get_index_constant(loc, i + 1),
                            index_type, nullptr));
                        allocate_dims.push_back(al, allocate_dim);
                    }
                } else {
                    return false;
                }
                break;
            }
            default: {
                LCOMPILERS_ASSERT_MSG(false, "ASR::exprType::" + std::to_string(value->type)
                    + " not handled yet in set_allocation_size");
            }
        }
        return true;
    }


    std::vector<ASR::expr_t*> remap_indices_to_array_bounds(
            Allocator &al, const Location &loc,
            const std::vector<ASR::expr_t*> &do_loop_variables,
            ASR::expr_t* loop_arr, ASR::expr_t* indexed_arr,
            int integer_kind) {
        ASRUtils::ASRBuilder b(al, loc);
        std::vector<ASR::expr_t*> mapped_vars;
        mapped_vars.reserve(do_loop_variables.size());
        for (size_t i = 0; i < do_loop_variables.size(); i++) {
            ASR::expr_t *loop_lbound = PassUtils::get_bound(
                loop_arr, i + 1, "lbound", al, integer_kind);
            ASR::expr_t *indexed_lbound = PassUtils::get_bound(
                indexed_arr, i + 1, "lbound", al, integer_kind);
            mapped_vars.push_back(b.Add(indexed_lbound,
                b.Sub(do_loop_variables[i], loop_lbound)));
        }
        return mapped_vars;
    }

    ASR::stmt_t* create_do_loop(Allocator &al, const Location &loc, std::vector<ASR::expr_t*> do_loop_variables, ASR::expr_t* left_arr, ASR::expr_t* right_arr, int curr_idx, int integer_kind) {
        ASRUtils::ASRBuilder b(al, loc);

        if (curr_idx == 1) {
            std::vector<ASR::expr_t*> vars;
            for (size_t i = 0; i < do_loop_variables.size(); i++) {
                vars.push_back(do_loop_variables[i]);
            }
            std::vector<ASR::expr_t*> right_vars = remap_indices_to_array_bounds(
                al, loc, vars, left_arr, right_arr, integer_kind);
            return b.DoLoop(do_loop_variables[curr_idx - 1],
                PassUtils::get_bound(left_arr, curr_idx, "lbound", al, integer_kind),
                PassUtils::get_bound(left_arr, curr_idx, "ubound", al, integer_kind), {
                b.Assignment(b.ArrayItem_01(left_arr, vars), b.ArrayItem_01(right_arr, right_vars))
            }, nullptr);
        }
        return b.DoLoop(do_loop_variables[curr_idx - 1],
            PassUtils::get_bound(left_arr, curr_idx, "lbound", al, integer_kind),
            PassUtils::get_bound(left_arr, curr_idx, "ubound", al, integer_kind), {
            create_do_loop(al, loc, do_loop_variables, left_arr, right_arr, curr_idx - 1, integer_kind)
        }, nullptr);
    }

    void traverse_call_args(Vec<ASR::call_arg_t>& x_m_args_vec, ASR::call_arg_t* x_m_args,
        size_t x_n_args, const std::string& name_hint,
        std::vector<bool> is_arg_intent_out = {},
        std::vector<bool> is_arg_intent_in = {},
        std::vector<bool> is_arg_intent_out_only = {},
        bool is_func_bind_c = false,
        std::vector<bool> bypass_raw_helper_array_temps = {},
        ASR::Function_t *func = nullptr) {
        /* For other frontends, we might need to traverse the arguments
           in reverse order. */
        for( size_t i = 0; i < x_n_args; i++ ) {
            ASR::expr_t* arg_expr = x_m_args[i].m_value;
            if (bypass_raw_helper_array_temps.size() > i
                    && bypass_raw_helper_array_temps[i]) {
                x_m_args_vec.push_back(al, x_m_args[i]);
                continue;
            }
            if (func != nullptr && i < func->n_args
                    && should_preserve_c_no_copy_section_actual(
                        func->m_args[i], x_m_args[i].m_value)) {
                x_m_args_vec.push_back(al, x_m_args[i]);
                continue;
            }
            if ( x_m_args[i].m_value && is_descriptor_array_casted_to_pointer_to_data(x_m_args[i].m_value) &&
                 !is_func_bind_c &&
                 !ASR::is_a<ASR::FunctionParam_t>(*ASRUtils::get_past_array_physical_cast(x_m_args[i].m_value)) 
                 && !ASRUtils::is_stringToArray_cast(ASR::down_cast<ASR::ArrayPhysicalCast_t>(arg_expr)->m_arg)) {
                ASR::ArrayPhysicalCast_t* array_physical_cast = ASR::down_cast<ASR::ArrayPhysicalCast_t>(arg_expr);
                ASR::expr_t* arg_expr_past_cast = ASRUtils::get_past_array_physical_cast(arg_expr);
                const Location& loc = arg_expr->base.loc;
                ASR::expr_t* array_var_temporary = create_temporary_variable_for_array(
                    al, arg_expr_past_cast, current_scope, name_hint, true);
                ASR::call_arg_t array_var_temporary_arg;
                array_var_temporary_arg.loc = loc;
                const bool unhandled_case = ASRUtils::is_unlimited_polymorphic_type(ASRUtils::expr_type(arg_expr)) || 
                        (ASR::is_a<ASR::ArrayPhysicalCast_t>(*arg_expr) 
                        && ASRUtils::is_unlimited_polymorphic_type(ASRUtils::expr_type(
                            ASR::down_cast<ASR::ArrayPhysicalCast_t>(arg_expr)->m_arg))); // TODO : remove -- Look `class_95.f90`  
                if( ASRUtils::is_pointer(ASRUtils::expr_type(array_var_temporary)) && !unhandled_case ) {
                    ASR::expr_t* casted_array_var_temporary_arg = ASRUtils::EXPR(ASR::make_ArrayPhysicalCast_t(al, loc,
                        array_var_temporary, ASR::array_physical_typeType::DescriptorArray, array_physical_cast->m_new,
                        array_physical_cast->m_type, nullptr));
                    array_var_temporary_arg.m_value = casted_array_var_temporary_arg;
                    x_m_args_vec.push_back(al, array_var_temporary_arg);
                    // This should be always true as we pass `true` to `create_temporary_variable_for_array`
                    ASRUtils::ASRBuilder b(al, arg_expr_past_cast->base.loc);
                    Vec<ASR::dimension_t> allocate_dims; allocate_dims.reserve(al, 1);
                    ASR::expr_t* len_expr{}; // Character length to allocate with
                    size_t target_n_dims = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(array_var_temporary));
                    bool allocation_size_set = false;
                    bool is_spread_result_arg = name_hint.find("lcompilers_spread") != std::string::npos
                        && is_arg_intent_out.size() > i && is_arg_intent_out[i];
                    if (is_spread_result_arg && i >= 2) {
                        size_t source_index = 0;
                        size_t dim_index = i - 2;
                        size_t ncopies_index = i - 1;
                        ASR::expr_t *source = x_m_args[source_index].m_value;
                        ASR::expr_t *dim = x_m_args[dim_index].m_value;
                        ASR::expr_t *ncopies = x_m_args[ncopies_index].m_value;
                        ASR::expr_t *dim_value = dim ? ASRUtils::expr_value(dim) : nullptr;
                        if (source && dim_value && ASR::is_a<ASR::IntegerConstant_t>(*dim_value)) {
                            int64_t spread_dim =
                                ASR::down_cast<ASR::IntegerConstant_t>(dim_value)->m_n;
                            allocate_dims.reserve(al, target_n_dims);
                            int ncopies_inserted = 0;
                            ASR::ttype_t *index_type = get_index_type(loc);
                            auto make_index_length = [&](ASR::expr_t *expr) -> ASR::expr_t* {
                                ASR::ttype_t *expr_type = ASRUtils::expr_type(expr);
                                if (!expr_type) {
                                    return expr;
                                }
                                if (ASRUtils::is_integer(*expr_type)) {
                                    if (ASRUtils::extract_kind_from_ttype_t(expr_type)
                                            == get_index_kind()) {
                                        return expr;
                                    }
                                    return ASRUtils::EXPR(ASR::make_Cast_t(
                                        al, expr->base.loc, expr,
                                        ASR::cast_kindType::IntegerToInteger,
                                        index_type, nullptr, nullptr));
                                }
                                if (ASRUtils::is_unsigned_integer(*expr_type)) {
                                    return ASRUtils::EXPR(ASR::make_Cast_t(
                                        al, expr->base.loc, expr,
                                        ASR::cast_kindType::UnsignedIntegerToInteger,
                                        index_type, nullptr, nullptr));
                                }
                                return expr;
                            };
                            ASR::expr_t *ncopies_length = make_index_length(ncopies);
                            for (size_t idim = 0; idim < target_n_dims; idim++) {
                                ASR::dimension_t allocate_dim;
                                allocate_dim.loc = loc;
                                allocate_dim.m_start = get_index_one(loc);
                                if (static_cast<int64_t>(idim) == spread_dim - 1) {
                                    allocate_dim.m_length = ncopies_length;
                                    ncopies_inserted = 1;
                                } else {
                                    ASR::expr_t *source_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                                        al, loc, ASRUtils::get_past_array_physical_cast(source),
                                        get_index_constant(loc, idim + 1 - ncopies_inserted),
                                        index_type, nullptr));
                                    allocate_dim.m_length = make_index_length(source_length);
                                }
                                allocate_dims.push_back(al, allocate_dim);
                            }
                            allocation_size_set = true;
                        }
                    }
                    if( (!allocation_size_set
                            && !set_allocation_size(al, arg_expr_past_cast, allocate_dims,
                                target_n_dims, len_expr))
                            || target_n_dims != allocate_dims.size() ) {
                        current_body->push_back(al, ASRUtils::STMT(ASR::make_Associate_t(
                            al, loc, array_var_temporary, arg_expr_past_cast)));
                        continue;
                    }
                    Vec<ASR::alloc_arg_t> alloc_args; alloc_args.reserve(al, 1);
                    ASR::alloc_arg_t alloc_arg;
                    alloc_arg.loc = arg_expr_past_cast->base.loc;
                    alloc_arg.m_a = array_var_temporary;
                    alloc_arg.m_dims = allocate_dims.p;
                    alloc_arg.n_dims = allocate_dims.size();
                    alloc_arg.m_len_expr = len_expr;
                    alloc_arg.m_type = nullptr;
                    alloc_arg.m_sym_subclass = nullptr;
                    alloc_args.push_back(al, alloc_arg);
                    bool arg_is_intent_out =
                        is_arg_intent_out.size() > i && is_arg_intent_out[i];
                    bool arg_is_intent_out_only =
                        is_arg_intent_out_only.size() > i && is_arg_intent_out_only[i];
                    if (arg_is_intent_out && is_spread_result_arg
                            && ASRUtils::is_allocatable(ASRUtils::expr_type(arg_expr_past_cast))) {
                        ASR::alloc_arg_t actual_alloc_arg;
                        actual_alloc_arg.loc = arg_expr_past_cast->base.loc;
                        actual_alloc_arg.m_a = arg_expr_past_cast;
                        actual_alloc_arg.m_dims = allocate_dims.p;
                        actual_alloc_arg.n_dims = allocate_dims.size();
                        actual_alloc_arg.m_len_expr = len_expr;
                        actual_alloc_arg.m_type = nullptr;
                        actual_alloc_arg.m_sym_subclass = nullptr;
                        Vec<ASR::alloc_arg_t> actual_alloc_args;
                        actual_alloc_args.reserve(al, 1);
                        actual_alloc_args.push_back(al, actual_alloc_arg);
                        current_body->push_back(al, ASRUtils::STMT(ASR::make_ReAlloc_t(
                            al, arg_expr_past_cast->base.loc,
                            actual_alloc_args.p, actual_alloc_args.size())));
                    }
                
                    Vec<ASR::expr_t*> dealloc_args; dealloc_args.reserve(al, 1);
                    dealloc_args.push_back(al, array_var_temporary);
                    bool is_read_only_arg = is_arg_intent_in.size() > i && is_arg_intent_in[i];
                    if ((is_read_only_arg || pass_options.c_backend) &&
                            is_whole_dummy_array_variable(arg_expr_past_cast) &&
                            (!pass_options.c_backend ||
                                is_known_one_based_contiguous_whole_array(arg_expr_past_cast))) {
                        current_body->push_back(al, ASRUtils::STMT(ASR::make_Associate_t(
                            al, loc, array_var_temporary, arg_expr_past_cast)));
                        body_after_curr_stmt->push_back(al, ASRUtils::STMT(ASR::make_Nullify_t(
                            al, loc, dealloc_args.p, dealloc_args.size())));
                        continue;
                    }
                    ASR::dimension_t* array_dims = nullptr;
                    int array_rank = ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(array_var_temporary), array_dims);
                    int integer_kind = pass_options.descriptor_index_64 ? 8 : 4;
                    ASR::expr_t* can_alias_actual = nullptr;
                    if (is_known_one_based_contiguous_whole_array(arg_expr_past_cast)) {
                        can_alias_actual = ASRUtils::EXPR(ASR::make_LogicalConstant_t(
                            al, loc, true, ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4))));
                    } else {
                        ASR::expr_t* is_contiguous = ASRUtils::EXPR(ASR::make_ArrayIsContiguous_t(al, loc,
                            arg_expr_past_cast, ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)), nullptr));
                        can_alias_actual = is_contiguous;
                        for (int dim = 0; dim < array_rank; dim++) {
                            ASR::expr_t* lbound_is_one = b.Eq(
                                PassUtils::get_bound(arg_expr_past_cast, dim + 1,
                                    "lbound", al, integer_kind),
                                get_index_one(loc));
                            can_alias_actual = b.And(can_alias_actual, lbound_is_one);
                        }
                    }
                    ASR::expr_t* must_copy_actual = ASRUtils::EXPR(ASR::make_LogicalNot_t(al, loc, can_alias_actual,
                        ASRUtils::expr_type(can_alias_actual), nullptr));
                    std::vector<ASR::expr_t*> do_loop_variables;
                    #define declare(var_name, type, intent)                                     \
                    b.Variable(fn_symtab, var_name, type, ASR::intentType::intent)
                    for (int i = 0; i < array_rank; i++) {
                        std::string var_name = current_scope->get_unique_name("__lcompilers_i_" + std::to_string(i));
                        ASR::ttype_t* loop_var_type = pass_options.descriptor_index_64
                            ? ASRUtils::expr_type(b.i64(0))
                            : ASRUtils::expr_type(b.i32(0));
                        do_loop_variables.push_back(b.Variable(current_scope, var_name, loop_var_type, ASR::intentType::Local));
                    }
                    std::vector<ASR::stmt_t*> copy_in_body = {
                        ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(al,
                            array_var_temporary->base.loc, dealloc_args.p, dealloc_args.size())),
                        ASRUtils::STMT(ASR::make_Allocate_t(al,
                            array_var_temporary->base.loc, alloc_args.p, alloc_args.size(),
                            nullptr, nullptr, nullptr))
                    };
                    if (!arg_is_intent_out_only) {
                        copy_in_body.push_back(create_do_loop(al, loc, do_loop_variables,
                            array_var_temporary, arg_expr_past_cast, array_rank, integer_kind));
                    }
                    current_body->push_back(al,
                        b.If(must_copy_actual, copy_in_body, {
                            ASRUtils::STMT(ASR::make_Associate_t(
                                al, loc, array_var_temporary, arg_expr_past_cast))
                        })
                    );
                    if ( arg_is_intent_out ) {
                        std::vector<ASR::stmt_t*> copy_out_body;
                        copy_out_body.reserve(2);
                        if (is_spread_result_arg
                                && ASRUtils::is_allocatable(ASRUtils::expr_type(arg_expr_past_cast))) {
                            ASR::alloc_arg_t actual_alloc_arg;
                            actual_alloc_arg.loc = arg_expr_past_cast->base.loc;
                            actual_alloc_arg.m_a = arg_expr_past_cast;
                            actual_alloc_arg.m_dims = allocate_dims.p;
                            actual_alloc_arg.n_dims = allocate_dims.size();
                            actual_alloc_arg.m_len_expr = len_expr;
                            actual_alloc_arg.m_type = nullptr;
                            actual_alloc_arg.m_sym_subclass = nullptr;
                            Vec<ASR::alloc_arg_t> actual_alloc_args;
                            actual_alloc_args.reserve(al, 1);
                            actual_alloc_args.push_back(al, actual_alloc_arg);
                            copy_out_body.push_back(ASRUtils::STMT(ASR::make_ReAlloc_t(
                                al, arg_expr_past_cast->base.loc,
                                actual_alloc_args.p, actual_alloc_args.size())));
                        }
                        copy_out_body.push_back(
                            create_do_loop(al, loc, do_loop_variables, arg_expr_past_cast,
                                array_var_temporary, array_rank, integer_kind));
                        body_after_curr_stmt->push_back(al, b.If(must_copy_actual,
                            copy_out_body, {}));
                    }
                    // Nullify the pointer after the call if it was associated (contiguous case).
                    // This prevents double-free on the next loop iteration: without nullification,
                    // the ExplicitDeallocate at the start of the loop would try to free memory
                    // that belongs to the source array (was aliased via Associate).
                    // In the non-contiguous case, deallocate the heap-allocated temporary
                    // that was used for copy-in (and copy-out).
                    body_after_curr_stmt->push_back(al, b.If(can_alias_actual, {
                        ASRUtils::STMT(ASR::make_Nullify_t(al, loc, dealloc_args.p, dealloc_args.size()))
                    }, {
                        ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(al, loc, dealloc_args.p, dealloc_args.size()))
                    }));
                } else {
                    x_m_args_vec.push_back(al, x_m_args[i]);
                }
            } else {
                x_m_args_vec.push_back(al, x_m_args[i]);
            }
        }
    }

    template <typename T>
    void add_class_to_struct_casts(Vec<ASR::call_arg_t>& call_args, const T& x) {
        ASR::symbol_t* call_sym = ASRUtils::symbol_get_past_external(x.m_name);
        ASR::Function_t* func = nullptr;
        if (ASR::is_a<ASR::Function_t>(*call_sym)) {
            func = ASR::down_cast<ASR::Function_t>(call_sym);
        } else if (ASR::is_a<ASR::StructMethodDeclaration_t>(*call_sym)) {
            ASR::StructMethodDeclaration_t* method = ASR::down_cast<ASR::StructMethodDeclaration_t>(call_sym);
            func = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(method->m_proc));
        } else {
            return;
        }

        for (size_t i = 0; i < call_args.size(); i++) {
            if (call_args.p[i].m_value == nullptr || i >= func->n_args) {
                continue;
            }
            ASR::expr_t* actual_arg = call_args.p[i].m_value;
            ASR::expr_t* formal_arg = func->m_args[i];
            ASR::ttype_t* formal_arg_full_type = ASRUtils::expr_type(formal_arg);
            if (ASRUtils::is_allocatable(formal_arg_full_type) ||
                ASRUtils::is_pointer(formal_arg_full_type)) {
                continue;
            }
            if (!ASR::is_a<ASR::ArrayItem_t>(*actual_arg)) {
                continue;
            }
            ASR::ttype_t* actual_type = ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(actual_arg));
            ASR::ttype_t* formal_type = ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(formal_arg));
            if (!actual_type || !formal_type ||
                !ASR::is_a<ASR::StructType_t>(*actual_type) ||
                !ASR::is_a<ASR::StructType_t>(*formal_type) ||
                !ASRUtils::is_class_type(actual_type) ||
                ASRUtils::is_class_type(formal_type)) {
                continue;
            }

            ASR::symbol_t* actual_struct_sym = ASRUtils::symbol_get_past_external(
                ASRUtils::get_struct_sym_from_struct_expr(actual_arg));
            ASR::symbol_t* formal_struct_sym = ASRUtils::symbol_get_past_external(
                ASRUtils::get_struct_sym_from_struct_expr(formal_arg));
            if (!actual_struct_sym || !formal_struct_sym ||
                !ASR::is_a<ASR::Struct_t>(*actual_struct_sym) ||
                !ASR::is_a<ASR::Struct_t>(*formal_struct_sym)) {
                continue;
            }

            if (actual_struct_sym != formal_struct_sym) {
                continue;
            }

            call_args.p[i].m_value = ASRUtils::EXPR(ASR::make_Cast_t(al, actual_arg->base.loc,
                actual_arg, ASR::cast_kindType::ClassToStruct,
                ASRUtils::duplicate_type(al, ASRUtils::expr_type(formal_arg)), nullptr, nullptr));
        }
    }

    template <typename T>
    void visit_Call(const T& x, const std::string& name_hint) {
        Vec<ASR::call_arg_t> x_m_args; x_m_args.reserve(al, x.n_args);
        std::vector<bool> is_arg_intent_out;
        std::vector<bool> is_arg_intent_in;
        std::vector<bool> is_arg_intent_out_only;
        if ( ASR::is_a<ASR::Function_t>(*ASRUtils::symbol_get_past_external(x.m_name)) ) {
            ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(x.m_name));
            ASR::FunctionType_t* func_type = ASR::down_cast<ASR::FunctionType_t>(func->m_function_signature);
            bool is_func_bind_c = func_type->m_abi == ASR::abiType::BindC;
            std::vector<bool> bypass_raw_helper_array_temps =
                get_c_raw_helper_call_temp_bypass(func, x.m_args, x.n_args);
            std::vector<bool> bypass_inline_dot_product_array_temps =
                get_c_inlineable_dot_product_call_temp_bypass(
                    func, x.m_args, x.n_args);
            std::vector<bool> bypass_inline_reduction_array_temps =
                get_c_inlineable_reduction_call_temp_bypass(
                    func, x.m_args, x.n_args);
            for (size_t i = 0; i < bypass_raw_helper_array_temps.size()
                    && i < bypass_inline_dot_product_array_temps.size()
                    && i < bypass_inline_reduction_array_temps.size(); i++) {
                bypass_raw_helper_array_temps[i] =
                    bypass_raw_helper_array_temps[i]
                    || bypass_inline_dot_product_array_temps[i]
                    || bypass_inline_reduction_array_temps[i];
            }
            for (size_t i = 0; i < func->n_args; i++ ) {
                if ( ASR::is_a<ASR::Var_t>(*func->m_args[i]) ) {
                    ASR::Var_t* var_ = ASR::down_cast<ASR::Var_t>(func->m_args[i]);
                    if ( ASR::is_a<ASR::Variable_t>(*var_->m_v) ) {
                        ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(var_->m_v);
                        is_arg_intent_out.push_back(
                            var->m_intent == ASR::intentType::Out ||
                            var->m_intent == ASR::intentType::InOut ||
                            var->m_intent == ASR::intentType::Unspecified
                        );
                        is_arg_intent_in.push_back(
                            var->m_intent == ASR::intentType::In
                        );
                        is_arg_intent_out_only.push_back(
                            var->m_intent == ASR::intentType::Out
                        );
                    } else {
                        is_arg_intent_out.push_back(false);
                        is_arg_intent_in.push_back(false);
                        is_arg_intent_out_only.push_back(false);
                    }
                } else {
                    is_arg_intent_out.push_back(false);
                    is_arg_intent_in.push_back(false);
                    is_arg_intent_out_only.push_back(false);
                }
            }
            traverse_call_args(x_m_args, x.m_args, x.n_args,
                name_hint + ASRUtils::symbol_name(x.m_name), is_arg_intent_out,
                is_arg_intent_in, is_arg_intent_out_only, is_func_bind_c,
                bypass_raw_helper_array_temps, func);
        } else {
            traverse_call_args(x_m_args, x.m_args, x.n_args,
                name_hint + ASRUtils::symbol_name(x.m_name));
        }
        add_class_to_struct_casts(x_m_args, x);

        T& xx = const_cast<T&>(x);
        xx.m_args = x_m_args.p;
        xx.n_args = x_m_args.size();
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        visit_Call(x, "_subroutine_call_");
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_SubroutineCall(x);
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        if (is_current_body_set != 0) {
            visit_Call(x, "_function_call_");
        }
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_FunctionCall(x);
    }

    void visit_Function(const ASR::Function_t& x) {
        vars_from_assumed_size_sections.clear();
        vars_from_c_no_copy_section_views.clear();
        one_based_contiguous_allocs.clear();
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_Function(x);
    }

    void visit_Allocate(const ASR::Allocate_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_Allocate(x);
        for (size_t i = 0; i < x.n_args; i++) {
            track_allocate_arg(x.m_args[i]);
        }
    }

    void visit_ReAlloc(const ASR::ReAlloc_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_ReAlloc(x);
        for (size_t i = 0; i < x.n_args; i++) {
            track_allocate_arg(x.m_args[i]);
        }
    }

    void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_ExplicitDeallocate(x);
        for (size_t i = 0; i < x.n_vars; i++) {
            ASR::symbol_t *sym = get_whole_array_var_symbol(x.m_vars[i]);
            if (sym) {
                one_based_contiguous_allocs.erase(sym);
            }
        }
    }

    // Track Associate statements created by pass_array_by_data where the
    // source is an ArraySection that must use sequence association rather than
    // copy-in/copy-out lowering.
    // For UnboundedPointerArray: allocation size cannot be determined (undefined UBound).
    // For explicit-shape/fixed-size sources: the callee must index original
    // contiguous storage with its own leading dimension; copy-in would create
    // a packed temporary with wrong memory layout and very large generated C.
    void visit_Associate(const ASR::Associate_t& x) {
        if ( ASR::is_a<ASR::ArraySection_t>(*x.m_value) &&
             ASR::is_a<ASR::Var_t>(*x.m_target) ) {
            ASR::ArraySection_t* section = ASR::down_cast<ASR::ArraySection_t>(x.m_value);
            ASR::ttype_t* source_type = ASRUtils::expr_type(section->m_v);
            ASR::Variable_t *target_var =
                ASRUtils::expr_to_variable_or_null(x.m_target);
            if ( ASRUtils::is_array(source_type) ) {
                if ( is_sequence_association_array_section(section) ) {
                    ASR::symbol_t* sym = ASR::down_cast<ASR::Var_t>(x.m_target)->m_v;
                    vars_from_assumed_size_sections.insert(sym);
                }
                ASR::ttype_t *target_type = target_var
                    ? ASRUtils::type_get_past_pointer(target_var->m_type)
                    : nullptr;
                if (pass_options.c_backend
                        && target_type != nullptr
                        && ASRUtils::is_array(target_type)
                        && ASRUtils::extract_physical_type(target_type)
                            == ASR::array_physical_typeType::DescriptorArray
                        && is_c_simple_array_section_view(section)
                        && is_c_plain_scalar_array_element_type(
                            ASRUtils::type_get_past_array(
                                ASRUtils::type_get_past_allocatable_pointer(
                                    target_type)))) {
                    ASR::symbol_t* sym = ASRUtils::symbol_get_past_external(
                        ASR::down_cast<ASR::Var_t>(x.m_target)->m_v);
                    vars_from_c_no_copy_section_views.insert(sym);
                }
            }
        }
        ASR::CallReplacerOnExpressionsVisitor<CallVisitor>::visit_Associate(x);
    }

    // Check if an ArraySection has a right bound that is an ArrayBound UBound
    // on an assumed-size dimension (a dimension with no declared upper bound).
    static bool has_undefined_assumed_size_bound(ASR::ArraySection_t* section) {
        ASR::ttype_t* source_type = ASRUtils::expr_type(section->m_v);
        ASR::dimension_t* dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(source_type, dims);
        for ( size_t i = 0; i < section->n_args && i < n_dims; i++ ) {
            // Check if this dimension is assumed-size (no declared upper bound)
            if ( dims[i].m_length != nullptr ) continue;
            // Check if the section's right bound is ArrayBound UBound on this dim
            ASR::expr_t* right = section->m_args[i].m_right;
            if ( right && ASR::is_a<ASR::ArrayBound_t>(*right) ) {
                ASR::ArrayBound_t* bound = ASR::down_cast<ASR::ArrayBound_t>(right);
                if ( bound->m_bound == ASR::arrayboundType::UBound ) {
                    return true;
                }
            }
        }
        return false;
    }

    // Don't visit DebugCheckArrayBounds, m_dt in FunctionCall might be an array
    void visit_DebugCheckArrayBounds(const ASR::DebugCheckArrayBounds_t& x) {
        (void)x;
    }
};

void pass_replace_array_passed_in_function_call(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& pass_options) {
    CallVisitor v(al, pass_options);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LCompilers
