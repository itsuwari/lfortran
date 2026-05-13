#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_array_op.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>

#include <libasr/asr_builder.h>

#include <vector>

namespace LCompilers {

static inline bool is_unlimited_polymorphic_array_type(ASR::ttype_t *type) {
    type = ASRUtils::type_get_past_allocatable_pointer(type);
    if (type == nullptr || !ASRUtils::is_array(type)) {
        return false;
    }
    ASR::ttype_t *element_type = ASRUtils::extract_type(type);
    return element_type != nullptr
        && ASR::is_a<ASR::StructType_t>(*element_type)
        && ASR::down_cast<ASR::StructType_t>(element_type)->m_is_unlimited_polymorphic;
}

static inline bool is_unlimited_polymorphic_array_expr(ASR::expr_t *expr) {
    return expr != nullptr
        && is_unlimited_polymorphic_array_type(ASRUtils::expr_type(expr));
}

class ArrayVarAddressReplacer: public ASR::BaseExprReplacer<ArrayVarAddressReplacer> {

    public:

    Allocator& al;
    Vec<ASR::expr_t**>& vars;

    ArrayVarAddressReplacer(Allocator& al_, Vec<ASR::expr_t**>& vars_):
        al(al_), vars(vars_) {
        call_replacer_on_value = false;
    }

    void replace_ArraySize(ASR::ArraySize_t* /*x*/) {

    }

    void replace_ArrayBound(ASR::ArrayBound_t* /*x*/) {

    }

    void replace_ArrayRank(ASR::ArrayRank_t* /*x*/) {

    }

    void replace_Var(ASR::Var_t* x) {
        if( ASRUtils::is_array(ASRUtils::symbol_type(x->m_v)) ) {
            vars.push_back(al, current_expr);
        }
    }

    void replace_StructInstanceMember(ASR::StructInstanceMember_t* x) {
        if( !ASRUtils::is_array(x->m_type) ) {
            return ;
        }
        if( ASRUtils::is_array(ASRUtils::symbol_type(x->m_m)) ) {
            vars.push_back(al, current_expr);
        } else {
            ASR::BaseExprReplacer<ArrayVarAddressReplacer>::replace_StructInstanceMember(x);
        }
    }

    void replace_ArrayItem(ASR::ArrayItem_t* /*x*/) {
    }

    void replace_ArraySection(ASR::ArraySection_t* x) {
        if( ASRUtils::is_array(ASRUtils::expr_type((ASR::expr_t*) x)) ) {
            vars.push_back(al, current_expr);
        }
    }

    template <typename T>
    void replace_BinOp_args(T* x) {
        ASR::expr_t** current_expr_copy = current_expr;
        current_expr = &(x->m_left);
        replace_expr(x->m_left);
        current_expr = &(x->m_right);
        replace_expr(x->m_right);
        current_expr = current_expr_copy;
    }

    void replace_IntegerBinOp(ASR::IntegerBinOp_t* x) {
        replace_BinOp_args(x);
    }

    void replace_UnsignedIntegerBinOp(ASR::UnsignedIntegerBinOp_t* x) {
        replace_BinOp_args(x);
    }

    void replace_RealBinOp(ASR::RealBinOp_t* x) {
        replace_BinOp_args(x);
    }

    void replace_ComplexBinOp(ASR::ComplexBinOp_t* x) {
        replace_BinOp_args(x);
    }

    void replace_LogicalBinOp(ASR::LogicalBinOp_t* x) {
        replace_BinOp_args(x);
    }

    template <typename T>
    void replace_Unary_arg(T* x) {
        ASR::expr_t** current_expr_copy = current_expr;
        current_expr = &(x->m_arg);
        replace_expr(x->m_arg);
        current_expr = current_expr_copy;
    }

    void replace_IntegerUnaryMinus(ASR::IntegerUnaryMinus_t* x) {
        replace_Unary_arg(x);
    }

    void replace_RealUnaryMinus(ASR::RealUnaryMinus_t* x) {
        replace_Unary_arg(x);
    }

    void replace_IntrinsicElementalFunction(ASR::IntrinsicElementalFunction_t* x) {
        ASR::expr_t** current_expr_copy = current_expr;
        for (size_t i = 0; i < x->n_args; i++) {
            current_expr = &(x->m_args[i]);
            replace_expr(x->m_args[i]);
        }
        current_expr = current_expr_copy;
    }

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        if( !ASRUtils::is_elemental(x->m_name) ) {
            return ;
        }

        ASR::BaseExprReplacer<ArrayVarAddressReplacer>::replace_FunctionCall(x);
    }

    void replace_IntrinsicArrayFunction(ASR::IntrinsicArrayFunction_t* /*x*/) {
        // Do not descend into intrinsic array functions (reductions like
        // MaxVal, MinVal, Sum, etc.) because they operate on whole arrays
        // and their array arguments must not be replaced with element accesses.
    }

};

class ArrayVarAddressCollector: public ASR::CallReplacerOnExpressionsVisitor<ArrayVarAddressCollector> {

    private:

    ArrayVarAddressReplacer replacer;

    public:

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.replace_expr(*current_expr);
    }

    ArrayVarAddressCollector(Allocator& al_, Vec<ASR::expr_t**>& vars_):
        replacer(al_, vars_) {
        visit_expr_after_replacement = false;
    }

    void visit_Allocate(const ASR::Allocate_t& /*x*/) {
    }

    void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t& /*x*/) {
    }

    void visit_ImplicitDeallocate(const ASR::ImplicitDeallocate_t& /*x*/) {
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        if( !ASRUtils::is_elemental(x.m_name) ) {
            return ;
        }
    }

    void visit_Associate(const ASR::Associate_t& /*x*/) {
    }

};

class FixTypeVisitor: public ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor> {
    public:

    FixTypeVisitor(Allocator& al_) {
        (void)al_;      // Explicitly mark the parameter as unused
    }

    void visit_Cast(const ASR::Cast_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_Cast(x);
        ASR::Cast_t& xx = const_cast<ASR::Cast_t&>(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) &&
             ASRUtils::is_array(x.m_type) ) {
            xx.m_type = ASRUtils::type_get_past_array(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_IntrinsicElementalFunction(const ASR::IntrinsicElementalFunction_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_IntrinsicElementalFunction(x);
        ASR::IntrinsicElementalFunction_t& xx = const_cast<ASR::IntrinsicElementalFunction_t&>(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_args[0])) ) {
            xx.m_type = ASRUtils::extract_type(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_FunctionCall(x);
        if( !ASRUtils::is_elemental(x.m_name) ) {
            return ;
        }
        ASR::FunctionCall_t& xx = const_cast<ASR::FunctionCall_t&>(x);
        if( (x.m_dt && !ASRUtils::is_array(ASRUtils::expr_type(x.m_dt))) ||
            !ASRUtils::is_array(ASRUtils::expr_type(x.m_args[0].m_value)) ) {
            xx.m_type = ASRUtils::extract_type(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    template <typename T>
    void visit_ArrayOp(const T& x) {
        T& xx = const_cast<T&>(x);
        if( !ASRUtils::is_array(ASRUtils::expr_type(xx.m_left)) &&
            !ASRUtils::is_array(ASRUtils::expr_type(xx.m_right)) &&
            ASRUtils::is_array(xx.m_type) ) {
            xx.m_type = ASRUtils::extract_type(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_IntegerBinOp(x);
        visit_ArrayOp(x);
    }

    void visit_UnsignedIntegerBinOp(const ASR::UnsignedIntegerBinOp_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_UnsignedIntegerBinOp(x);
        visit_ArrayOp(x);
    }

    void visit_RealBinOp(const ASR::RealBinOp_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_RealBinOp(x);
        visit_ArrayOp(x);
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_ComplexBinOp(x);
        visit_ArrayOp(x);
    }

    void visit_RealCompare(const ASR::RealCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_RealCompare(x);
        visit_ArrayOp(x);
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_IntegerCompare(x);
        visit_ArrayOp(x);
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_ComplexCompare(x);
        visit_ArrayOp(x);
    }

    void visit_StringCompare(const ASR::StringCompare_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_StringCompare(x);
        visit_ArrayOp(x);
    }

    void visit_StringConcat(const ASR::StringConcat_t& x){
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_StringConcat(x);
        visit_ArrayOp(x);
    }

    void visit_LogicalBinOp(const ASR::LogicalBinOp_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_LogicalBinOp(x);
        visit_ArrayOp(x);
    }

    void visit_BitCast(const ASR::BitCast_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_BitCast(x);
        ASR::BitCast_t& xx = const_cast<ASR::BitCast_t&>(x);
        if (!ASRUtils::is_array(ASRUtils::expr_type(x.m_mold)) &&
             ASRUtils::is_array(x.m_type)) {
            xx.m_type = ASRUtils::type_get_past_array(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_ComplexRe(const ASR::ComplexRe_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_ComplexRe(x);
        ASR::ComplexRe_t& xx = const_cast<ASR::ComplexRe_t&>(x);
        if (!ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) &&
             ASRUtils::is_array(x.m_type)) {
            xx.m_type = ASRUtils::type_get_past_array(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_ComplexIm(const ASR::ComplexIm_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_ComplexIm(x);
        ASR::ComplexIm_t& xx = const_cast<ASR::ComplexIm_t&>(x);
        if (!ASRUtils::is_array(ASRUtils::expr_type(x.m_arg)) &&
             ASRUtils::is_array(x.m_type)) {
            xx.m_type = ASRUtils::type_get_past_array(xx.m_type);
            xx.m_value = nullptr;
        }
    }

    void visit_StructInstanceMember(const ASR::StructInstanceMember_t& x) {
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_StructInstanceMember(x);
        if( !ASRUtils::is_array(x.m_type) ) {
            return ;
        }
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_v)) &&
            !ASRUtils::is_array(ASRUtils::symbol_type(x.m_m)) ) {
            ASR::StructInstanceMember_t& xx = const_cast<ASR::StructInstanceMember_t&>(x);
            xx.m_type = ASRUtils::extract_type(x.m_type);
        }
    }

    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t& x){
        if( !ASRUtils::is_array(x.m_type) ) {
            return ;
        }
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_RealUnaryMinus(x);
        ASR::RealUnaryMinus_t& xx = const_cast<ASR::RealUnaryMinus_t&>(x);
        xx.m_type = ASRUtils::extract_type(x.m_type);
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t& x){
        if( !ASRUtils::is_array(x.m_type) ) {
            return ;
        }
        ASR::CallReplacerOnExpressionsVisitor<FixTypeVisitor>::visit_IntegerUnaryMinus(x);
        ASR::IntegerUnaryMinus_t& xx = const_cast<ASR::IntegerUnaryMinus_t&>(x);
        xx.m_type = ASRUtils::extract_type(x.m_type);
    }
};

class CleanupDegenerateArraySection: public ASR::BaseExprReplacer<CleanupDegenerateArraySection> {
    public:
    Allocator& al;
    CleanupDegenerateArraySection(Allocator& al_): al(al_) {}

    void replace_ArraySection(ASR::ArraySection_t* x) {
        ASR::BaseExprReplacer<CleanupDegenerateArraySection>::replace_ArraySection(x);
        if (!ASRUtils::is_array(ASRUtils::expr_type(x->m_v))) {
            *current_expr = x->m_v;
        }
    }

    void replace_StructInstanceMember(ASR::StructInstanceMember_t* x) {
        ASR::BaseExprReplacer<CleanupDegenerateArraySection>::replace_StructInstanceMember(x);
        if (ASRUtils::is_array(x->m_type) &&
            !ASRUtils::is_array(ASRUtils::expr_type(x->m_v)) &&
            !ASRUtils::is_array(ASRUtils::symbol_type(x->m_m))) {
            x->m_type = ASRUtils::extract_type(x->m_type);
        }
    }
};

class ReplaceArrayOp: public ASR::BaseExprReplacer<ReplaceArrayOp> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;

    public:

    SymbolTable* current_scope;
    ASR::expr_t* result_expr;
    bool& remove_original_stmt;

    ReplaceArrayOp(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
                   bool& remove_original_stmt_):
        al(al_), pass_result(pass_result_),
        current_scope(nullptr), result_expr(nullptr),
        remove_original_stmt(remove_original_stmt_) {}

    #define remove_original_stmt_if_size_0(type) if( ASRUtils::get_fixed_size_of_array(type) == 0 ) { \
            remove_original_stmt = true; \
            return ; \
        } \

    void replace_ArrayConstant(ASR::ArrayConstant_t* x) {
        remove_original_stmt_if_size_0(x->m_type)
        if (result_expr == nullptr || is_unlimited_polymorphic_array_expr(result_expr)) {
            return;
        }
        pass_result.reserve(al, x->m_n_data);
        const Location& loc = x->base.base.loc;
        ASR::ttype_t* result_type = ASRUtils::expr_type(result_expr);
        ASR::ttype_t* result_element_type = ASRUtils::extract_type(result_type);

        ASR::dimension_t* m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(x->m_type, m_dims);

        int64_t size = ASRUtils::get_fixed_size_of_array(x->m_type);
        int repeat = 1;
        std::vector<std::vector<int64_t>> rank_indexes(n_dims, std::vector<int64_t>(size));
        for (size_t i = 0; i < n_dims; i++) {
            int64_t length = 0, start = 1, ubound;
            ASRUtils::extract_value(m_dims[i].m_length, length);
            ASRUtils::extract_value(m_dims[i].m_start, start);
            ubound = length + start - 1;
            int64_t c = 0;
            while (c != size) {
                for (int64_t j = start; j <= ubound; j++) {
                    for (int64_t k = 1; k <= repeat; k++) {
                        rank_indexes[i][c++] = j;
                    }
                }
            }
            repeat *= length;
        }

        for (int64_t i = 0; i < size; i++) {
            ASR::expr_t* x_i = ASRUtils::fetch_ArrayConstant_value(al, x, i);
            Vec<ASR::array_index_t> array_index_args;
            array_index_args.reserve(al, n_dims);
            for (size_t j = 0; j < n_dims; j++) {
                ASR::array_index_t array_index_arg;
                array_index_arg.loc = loc;
                array_index_arg.m_left = nullptr;
                array_index_arg.m_right = make_ConstantWithKind(
                    make_IntegerConstant_t, make_Integer_t, rank_indexes[j][i], 4, loc);
                array_index_arg.m_step = nullptr;
                array_index_args.push_back(al, array_index_arg);
            }
            ASR::expr_t* y_i = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc,
                result_expr, array_index_args.p, array_index_args.size(),
                result_element_type, ASR::arraystorageType::ColMajor, nullptr));
            pass_result.push_back(al, ASRUtils::STMT(ASRUtils::make_Assignment_t_util(al, loc, y_i, x_i, nullptr, false, false)));
        }
    }

    bool are_all_elements_scalars(ASR::expr_t** args, size_t n) {
        for( size_t i = 0; i < n; i++ ) {
            if (ASR::is_a<ASR::ImpliedDoLoop_t>(*args[i])) {
                return false;
            }
            if( ASRUtils::is_array(ASRUtils::expr_type(args[i])) ) {
                return false;
            }
        }
        return true;
    }

    void replace_ArrayConstructor(ASR::ArrayConstructor_t* x) {
        if (result_expr != nullptr && is_unlimited_polymorphic_array_expr(result_expr)) {
            return;
        }
        if (result_expr == nullptr) {
            return;
        }
        // TODO: Remove this because the ArrayConstructor node should
        // be replaced with its value already (if present) in array_struct_temporary pass.
        if( x->m_value == nullptr ) {
            if( !are_all_elements_scalars(x->m_args, x->n_args) ) {
                PassUtils::ReplacerUtils::replace_ArrayConstructor_(
                    al, x, result_expr, &pass_result, current_scope);
                return ;
            }

            if( !ASRUtils::is_fixed_size_array(x->m_type) ) {
                PassUtils::ReplacerUtils::replace_ArrayConstructor_(
                    al, x, result_expr, &pass_result, current_scope);
                return ;
            }
        }

        ASR::ttype_t* arr_type = nullptr;
        ASR::ArrayConstant_t* arr_value = nullptr;
        if( x->m_value ) {
            arr_value = ASR::down_cast<ASR::ArrayConstant_t>(x->m_value);
            arr_type = arr_value->m_type;
        } else {
            arr_type = x->m_type;
        }

        remove_original_stmt_if_size_0(arr_type)
        pass_result.reserve(al, x->n_args);
        const Location& loc = x->base.base.loc;

        ASR::ttype_t* result_type = ASRUtils::expr_type(result_expr);
        ASRUtils::ExprStmtDuplicator duplicator(al);
        ASR::ttype_t* result_element_type = ASRUtils::extract_type(result_type);
        result_element_type = duplicator.duplicate_ttype(result_element_type);

        FixTypeVisitor fix_type_visitor(al);
        fix_type_visitor.current_scope = current_scope;
        fix_type_visitor.visit_ttype(*result_element_type);

        ASRUtils::ASRBuilder builder(al, loc);
        ASR::dimension_t* m_dims = nullptr;
        ASRUtils::extract_dimensions_from_ttype(arr_type, m_dims);

        for( int64_t i = 0; i < ASRUtils::get_fixed_size_of_array(arr_type); i++ ) {
            ASR::expr_t* x_i = nullptr;
            if( x->m_value ) {
                x_i = ASRUtils::fetch_ArrayConstant_value(al, arr_value, i);
            } else {
                x_i = x->m_args[i];
            }
            LCOMPILERS_ASSERT(!ASRUtils::is_array(ASRUtils::expr_type(x_i)));
            Vec<ASR::array_index_t> array_index_args;
            array_index_args.reserve(al, 1);
            ASR::array_index_t array_index_arg;
            array_index_arg.loc = loc;
            array_index_arg.m_left = nullptr;
            array_index_arg.m_right = builder.Add(m_dims[0].m_start, make_ConstantWithKind(
                make_IntegerConstant_t, make_Integer_t, i, 4, loc));
            array_index_arg.m_step = nullptr;
            array_index_args.push_back(al, array_index_arg);
            ASR::expr_t* y_i = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc,
                result_expr, array_index_args.p, array_index_args.size(),
                result_element_type, ASR::arraystorageType::ColMajor, nullptr));
            pass_result.push_back(al, ASRUtils::STMT(ASRUtils::make_Assignment_t_util(al, loc, y_i, x_i, nullptr, false, false)));
        }
    }

};

ASR::expr_t* at(Vec<ASR::expr_t*>& vec, int64_t index) {
    index = index + vec.size();
    if( index < 0 ) {
        return nullptr;
    }
    return vec[index];
}

// Collect the array expressions which should be of the same size
class CollectComponentsFromElementalExpr: public ASR::BaseWalkVisitor<CollectComponentsFromElementalExpr> {
private:
        Allocator& al;
        Vec<ASR::expr_t*>& vars;
public:

        CollectComponentsFromElementalExpr(Allocator& al_, Vec<ASR::expr_t*>& vars_) :
        al(al_), vars(vars_) {}

        void push_expr_if_array(ASR::expr_t *v) {
            if (ASRUtils::is_array(ASRUtils::expr_type(v))) {
                vars.push_back(al, v);
            }
        }

        // Don't go inside these
        void visit_ttype(const ASR::ttype_t &) {}
        void visit_ArraySection(const ASR::ArraySection_t&) {}
        void visit_ArrayItem(const ASR::ArrayItem_t&) {}
        void visit_ArraySize(const ASR::ArraySize_t&) {}
        void visit_ArrayReshape(const ASR::ArrayReshape_t&) {}
        void visit_ArrayBound(const ASR::ArrayBound_t&) {}

        void visit_Var(const ASR::Var_t& x) {
            ASR::Var_t *xx = const_cast<ASR::Var_t*>(&x);
            push_expr_if_array((ASR::expr_t *)xx);
        }

        void visit_ArrayPhysicalCast(const ASR::ArrayPhysicalCast_t& x) {
            ASR::ArrayPhysicalCast_t *xx = const_cast<ASR::ArrayPhysicalCast_t*>(&x);
            push_expr_if_array((ASR::expr_t *)xx);
        }

        void visit_StructInstanceMember(const ASR::StructInstanceMember_t& x) {
            ASR::StructInstanceMember_t *xx = const_cast<ASR::StructInstanceMember_t*>(&x);
            push_expr_if_array((ASR::expr_t *)xx);
        }

        void visit_BitCast(const ASR::BitCast_t& x) {
            ASR::BitCast_t *xx = const_cast<ASR::BitCast_t*>(&x);
            push_expr_if_array((ASR::expr_t *)xx);
        }

        void visit_ArrayConstant(const ASR::ArrayConstant_t& x) {
            ASR::ArrayConstant_t *xx = const_cast<ASR::ArrayConstant_t*>(&x);
            push_expr_if_array((ASR::expr_t *)xx);
        }

        // Only go inside the FunctionCall if the Function is elemental
        void visit_FunctionCall(const ASR::FunctionCall_t& x) {
            if (ASRUtils::is_elemental(x.m_name)) {
                ASR::BaseWalkVisitor<CollectComponentsFromElementalExpr>::visit_FunctionCall(x);
            }
        }
};

// Replaces BitCast (transfer) nodes that return arrays with temporary variables
class BitCastArrayReplacer: public ASR::BaseExprReplacer<BitCastArrayReplacer> {
    public:

    Allocator& al;
    SymbolTable* current_scope;
    Vec<ASR::stmt_t*> bc_stmts;

    BitCastArrayReplacer(Allocator& al_):
        al(al_), current_scope(nullptr) {
        bc_stmts.n = 0;
    }

    void replace_BitCast(ASR::BitCast_t* x) {
        if( !ASRUtils::is_array(x->m_type) ) {
            return ;
        }
        // Create a temporary variable with the BitCast's result type
        const Location& loc = x->base.base.loc;
        std::string tmp_name = current_scope->get_unique_name(
            "__libasr_bitcast_tmp_");
        ASR::symbol_t* tmp_sym = ASR::down_cast<ASR::symbol_t>(
            ASRUtils::make_Variable_t_util(
                al, loc, current_scope, s2c(al, tmp_name), nullptr, 0,
                ASR::intentType::Local, nullptr, nullptr,
                ASR::storage_typeType::Default, x->m_type, nullptr,
                ASR::abiType::Source, ASR::accessType::Public,
                ASR::presenceType::Required, false));
        current_scope->add_symbol(tmp_name, tmp_sym);
        ASR::expr_t* tmp_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, tmp_sym));

        // Create assignment: tmp = BitCast(...)
        ASR::expr_t* bitcast_expr = ASRUtils::EXPR((ASR::asr_t*)x);
        bc_stmts.push_back(al, ASRUtils::STMT(
            ASRUtils::make_Assignment_t_util(
                al, loc, tmp_var, bitcast_expr, nullptr, false, false)));

        // Replace the BitCast in the expression with the temporary
        *current_expr = tmp_var;
    }
};

class BitCastArrayVisitor: public ASR::CallReplacerOnExpressionsVisitor<BitCastArrayVisitor> {
    private:

    BitCastArrayReplacer replacer;

    public:

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.replace_expr(*current_expr);
    }

    BitCastArrayVisitor(Allocator& al_):
        replacer(al_) {}

    void set_scope(SymbolTable* scope) {
        replacer.current_scope = scope;
    }

    Vec<ASR::stmt_t*>& get_bc_stmts() {
        return replacer.bc_stmts;
    }

    void init_bc_stmts() {
        replacer.bc_stmts.n = 0;
        replacer.bc_stmts.reserve(replacer.al, 1);
    }
};

class ArrayOpVisitor: public ASR::CallReplacerOnExpressionsVisitor<ArrayOpVisitor> {
    private:

    Allocator& al;
    ReplaceArrayOp replacer;
    Vec<ASR::stmt_t*> pass_result;
    Vec<ASR::stmt_t*>* parent_body;
    bool realloc_lhs;
    bool bounds_checking;
    bool remove_original_stmt;
    bool skip_allocate_source_expansion;
    const LCompilers::PassOptions& pass_options;
    inline static std::set<const ASR::Assignment_t*> debug_inserted;
    size_t allocate_source_copy_assignments_to_skip_realloc;

    public:

    int get_index_kind() const {
        return pass_options.descriptor_index_64 ? 8 : 4;
    }

    ASR::ttype_t* get_index_type(const Location& loc) {
        return ASRUtils::TYPE(ASR::make_Integer_t(al, loc, get_index_kind()));
    }

    bool allocate_source_assignment_follows(const ASR::Allocate_t& x,
            ASR::stmt_t **body, size_t n_body, size_t stmt_index) {
        if (x.m_source == nullptr) {
            return false;
        }
        bool needs_existing_assignment = false;
        size_t next_stmt_index = stmt_index + 1;
        for (size_t i = 0; i < x.n_args; i++) {
            if (!ASRUtils::is_array(ASRUtils::expr_type(x.m_args[i].m_a))) {
                continue;
            }
            if (x.m_args[i].m_type != nullptr) {
                continue;
            }
            needs_existing_assignment = true;
            if (next_stmt_index >= n_body
                    || !ASR::is_a<ASR::Assignment_t>(*body[next_stmt_index])) {
                return false;
            }
            ASR::Assignment_t *assign =
                ASR::down_cast<ASR::Assignment_t>(body[next_stmt_index]);
            if (!ASRUtils::expr_equal(assign->m_target, x.m_args[i].m_a)) {
                return false;
            }
            // The allocation already established the target shape from
            // source=/explicit bounds. Reallocating the immediately following
            // semantic copy only repeats shape work and, for source= arrays,
            // used to duplicate the full copy loop.
            assign->m_realloc_lhs = false;
            allocate_source_copy_assignments_to_skip_realloc++;
            next_stmt_index++;
        }
        return needs_existing_assignment;
    }

    bool allocation_stmt_targets_expr(ASR::stmt_t *stmt, ASR::expr_t *target) const {
        ASR::alloc_arg_t *args = nullptr;
        size_t n_args = 0;
        if (ASR::is_a<ASR::Allocate_t>(*stmt)) {
            ASR::Allocate_t *allocate = ASR::down_cast<ASR::Allocate_t>(stmt);
            args = allocate->m_args;
            n_args = allocate->n_args;
        } else if (ASR::is_a<ASR::ReAlloc_t>(*stmt)) {
            ASR::ReAlloc_t *realloc = ASR::down_cast<ASR::ReAlloc_t>(stmt);
            args = realloc->m_args;
            n_args = realloc->n_args;
        } else {
            return false;
        }
        for (size_t i = 0; i < n_args; i++) {
            if (ASRUtils::expr_equal(args[i].m_a, target)) {
                return true;
            }
        }
        return false;
    }

    bool previous_stmt_allocates_target(ASR::expr_t *target) const {
        return parent_body != nullptr
            && parent_body->size() > 0
            && allocation_stmt_targets_expr(
                parent_body->p[parent_body->size() - 1], target);
    }

    ASR::expr_t* unwrap_array_op_lvalue(ASR::expr_t *expr) const {
        while (expr != nullptr) {
            if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArrayPhysicalCast_t>(expr)->m_arg;
            } else if (ASR::is_a<ASR::Cast_t>(*expr)) {
                expr = ASR::down_cast<ASR::Cast_t>(expr)->m_arg;
            } else if (ASR::is_a<ASR::GetPointer_t>(*expr)) {
                expr = ASR::down_cast<ASR::GetPointer_t>(expr)->m_arg;
            } else {
                break;
            }
        }
        return expr;
    }

    ASR::ArrayConstant_t* get_array_constant_value(ASR::expr_t *expr) const {
        while (expr != nullptr) {
            ASR::expr_t *value = ASRUtils::expr_value(expr);
            if (value != nullptr && value != expr) {
                expr = value;
                continue;
            }
            expr = unwrap_array_op_lvalue(expr);
            break;
        }
        if (expr != nullptr && ASR::is_a<ASR::ArrayConstant_t>(*expr)) {
            return ASR::down_cast<ASR::ArrayConstant_t>(expr);
        }
        return nullptr;
    }

    bool is_unit_step_expr(ASR::expr_t *expr) const {
        if (expr == nullptr) {
            return true;
        }
        ASR::expr_t *value = ASRUtils::expr_value(expr);
        if (value == nullptr) {
            value = expr;
        }
        return ASR::is_a<ASR::IntegerConstant_t>(*value)
            && ASR::down_cast<ASR::IntegerConstant_t>(value)->m_n == 1;
    }

    bool is_generated_extent_upper_bound_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::IntegerBinOp_t>(*expr)) {
            return false;
        }
        ASR::IntegerBinOp_t *sub = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
        if (sub->m_op != ASR::binopType::Sub || !is_unit_step_expr(sub->m_right)) {
            return false;
        }
        ASR::expr_t *left = ASRUtils::get_past_array_physical_cast(sub->m_left);
        if (left == nullptr || !ASR::is_a<ASR::IntegerBinOp_t>(*left)) {
            return false;
        }
        ASR::IntegerBinOp_t *add = ASR::down_cast<ASR::IntegerBinOp_t>(left);
        return add->m_op == ASR::binopType::Add
            && (is_unit_step_expr(add->m_left)
                || is_unit_step_expr(add->m_right));
    }

    bool is_default_unit_array_slice(const ASR::array_index_t &idx) const {
        bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
        if (!is_slice || !is_unit_step_expr(idx.m_step)) {
            return false;
        }
        bool left_is_default = idx.m_left == nullptr
            || ASR::is_a<ASR::ArrayBound_t>(*idx.m_left)
            || is_unit_step_expr(idx.m_left);
        bool right_is_default = idx.m_right == nullptr
            || ASR::is_a<ASR::ArrayBound_t>(*idx.m_right)
            || is_generated_extent_upper_bound_expr(idx.m_right);
        return left_is_default && right_is_default;
    }

    bool is_scalar_array_index(const ASR::array_index_t &idx) const {
        return idx.m_left == nullptr && idx.m_step == nullptr
            && idx.m_right != nullptr
            && !ASRUtils::is_array(ASRUtils::expr_type(idx.m_right));
    }

    bool is_simple_integer_section_bound_expr(ASR::expr_t *expr) const {
        if (expr == nullptr) {
            return true;
        }
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::ArrayBound:
            case ASR::exprType::ArraySize:
            case ASR::exprType::IntegerConstant:
            case ASR::exprType::Var: {
                return true;
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_simple_integer_section_bound_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return is_simple_integer_section_bound_expr(binop->m_left)
                    && is_simple_integer_section_bound_expr(binop->m_right);
            }
            default: {
                return false;
            }
        }
    }

    bool is_simple_scalar_array_index(const ASR::array_index_t &idx) const {
        return is_scalar_array_index(idx)
            && is_simple_integer_section_bound_expr(idx.m_right);
    }

    bool is_c_rank1_unit_section(ASR::expr_t *expr) const {
        if (!pass_options.c_backend || expr == nullptr
                || !ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return false;
        }
        ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
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
            if (!is_unit_step_expr(idx.m_step)) {
                return false;
            }
        }
        return slice_dims == 1;
    }

    bool is_c_scalarizable_element_type(ASR::ttype_t *type) const {
        type = ASRUtils::type_get_past_array(
            ASRUtils::type_get_past_allocatable_pointer(type));
        return type != nullptr
            && (ASRUtils::is_integer(*type)
                || ASRUtils::is_unsigned_integer(*type)
                || ASRUtils::is_real(*type)
                || ASRUtils::is_logical(*type));
    }

    bool c_plain_array_copy_element_types_match(
            ASR::expr_t *target_expr, ASR::expr_t *value_expr) const {
        ASR::ttype_t *target_type = target_expr
            ? ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(target_expr)) : nullptr;
        ASR::ttype_t *value_type = value_expr
            ? ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(value_expr)) : nullptr;
        ASR::ttype_t *target_element_type = target_type
            ? ASRUtils::type_get_past_array(target_type) : nullptr;
        ASR::ttype_t *value_element_type = value_type
            ? ASRUtils::type_get_past_array(value_type) : nullptr;
        return target_element_type != nullptr
            && value_element_type != nullptr
            && is_c_scalarizable_element_type(target_element_type)
            && is_c_scalarizable_element_type(value_element_type)
            && ASRUtils::types_equal(
                target_element_type, value_element_type, nullptr, nullptr);
    }

    bool is_fixed_size_struct_member_array_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
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

    bool is_c_vector_subscript_expr(ASR::expr_t *expr) const {
        if (expr == nullptr) {
            return false;
        }
        expr = ASRUtils::get_past_array_physical_cast(expr);
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

    bool is_c_struct_member_vector_subscript_array_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::StructInstanceMember_t>(*expr)) {
            return false;
        }
        ASR::ttype_t *expr_type = ASRUtils::expr_type(expr);
        if (!ASRUtils::is_array(expr_type)
                || ASRUtils::extract_n_dims_from_ttype(expr_type) != 1
                || !is_c_scalarizable_element_type(expr_type)) {
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
        if (member_type == nullptr || ASRUtils::is_array(member_type)) {
            return false;
        }
        ASR::expr_t *base = ASRUtils::get_past_array_physical_cast(member->m_v);
        if (base == nullptr || !ASR::is_a<ASR::ArrayItem_t>(*base)) {
            return false;
        }
        ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(base);
        ASR::ttype_t *item_type = ASRUtils::expr_type(ASRUtils::EXPR(
            (ASR::asr_t*) item));
        if (!ASRUtils::is_array(item_type)
                || ASRUtils::extract_n_dims_from_ttype(item_type) != 1) {
            return false;
        }
        ASR::ttype_t *base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(item->m_v));
        if (base_type == nullptr || !ASRUtils::is_array(base_type)
                || ASRUtils::is_fixed_size_array(base_type)
                || is_fixed_size_struct_member_array_expr(item->m_v)) {
            return false;
        }
        size_t vector_dims = 0;
        for (size_t i = 0; i < item->n_args; i++) {
            ASR::expr_t *idx_expr = nullptr;
            if (item->m_args[i].m_right) {
                idx_expr = item->m_args[i].m_right;
            } else if (item->m_args[i].m_left) {
                idx_expr = item->m_args[i].m_left;
            }
            if (idx_expr == nullptr) {
                return false;
            }
            if (is_c_vector_subscript_expr(idx_expr)) {
                vector_dims++;
            }
        }
        return vector_dims == 1;
    }

    bool is_compiler_created_array_temp_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr != nullptr && ASR::is_a<ASR::ArraySection_t>(*expr)) {
            expr = ASR::down_cast<ASR::ArraySection_t>(expr)->m_v;
            expr = ASRUtils::get_past_array_physical_cast(expr);
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

    bool is_whole_allocatable_or_pointer_array_target(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        return type != nullptr
            && ASRUtils::is_array(type)
            && ASRUtils::is_allocatable_or_pointer(type);
    }

    bool is_c_rank1_unit_array_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASRUtils::is_array(ASRUtils::expr_type(expr))) {
            return false;
        }
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
            if (is_fixed_size_struct_member_array_expr(section->m_v)) {
                return false;
            }
            return is_c_rank1_unit_section(expr);
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || ASRUtils::extract_n_dims_from_ttype(type) != 1) {
            return false;
        }
        if (is_c_struct_member_vector_subscript_array_expr(expr)) {
            return false;
        }
        return ASR::is_a<ASR::Var_t>(*expr)
            || (ASR::is_a<ASR::StructInstanceMember_t>(*expr)
                && !is_fixed_size_struct_member_array_expr(expr));
    }

    bool is_c_rank2_full_array_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *expr_type = ASRUtils::expr_type(expr);
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            expr_type);
        if (type == nullptr
                || (!ASRUtils::is_array(type) && !ASRUtils::is_array_t(expr))
                || ASRUtils::extract_n_dims_from_ttype(type) != 2) {
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
        ASR::expr_t *base_expr = ASRUtils::get_past_array_physical_cast(section->m_v);
        ASR::ttype_t *base_type = base_expr != nullptr
            ? ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(base_expr)) : nullptr;
        int base_rank = base_type != nullptr
            ? ASRUtils::extract_n_dims_from_ttype(base_type) : 0;
        if (base_rank < 2 || section->n_args != static_cast<size_t>(base_rank)) {
            return false;
        }
        int slice_dims = 0;
        for (size_t i = 0; i < section->n_args; i++) {
            const ASR::array_index_t &idx = section->m_args[i];
            bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
            if (is_slice) {
                slice_dims++;
                if (!is_default_unit_array_slice(idx)) {
                    return false;
                }
            } else if (!is_simple_scalar_array_index(idx)) {
                return false;
            }
        }
        return slice_dims == 2;
    }

    bool is_c_rank2_unit_slice_array_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *expr_type = ASRUtils::expr_type(expr);
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            expr_type);
        if (type == nullptr
                || (!ASRUtils::is_array(type) && !ASRUtils::is_array_t(expr))
                || ASRUtils::extract_n_dims_from_ttype(type) != 2
                || !is_c_scalarizable_element_type(type)) {
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
        ASR::expr_t *base_expr = ASRUtils::get_past_array_physical_cast(section->m_v);
        ASR::ttype_t *base_type = base_expr != nullptr
            ? ASRUtils::type_get_past_allocatable_pointer(
                ASRUtils::expr_type(base_expr)) : nullptr;
        int base_rank = base_type != nullptr
            ? ASRUtils::extract_n_dims_from_ttype(base_type) : 0;
        if (base_rank < 2 || section->n_args != static_cast<size_t>(base_rank)) {
            return false;
        }
        int slice_dims = 0;
        for (size_t i = 0; i < section->n_args; i++) {
            const ASR::array_index_t &idx = section->m_args[i];
            bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
            if (is_slice) {
                slice_dims++;
                if (!is_unit_step_expr(idx.m_step)
                        || !is_simple_integer_section_bound_expr(idx.m_left)
                        || !is_simple_integer_section_bound_expr(idx.m_right)
                        || !is_simple_integer_section_bound_expr(idx.m_step)) {
                    return false;
                }
            } else if (!is_simple_scalar_array_index(idx)) {
                return false;
            }
        }
        return slice_dims == 2;
    }

    bool is_c_rank2_plain_data_copy_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection: {
                return is_c_rank2_unit_slice_array_expr(expr);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_rank1_plain_data_copy_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection: {
                return is_c_rank1_scalarizable_array_expr(expr);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_pointer_array_base_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return is_c_pointer_array_base_expr(
                ASR::down_cast<ASR::ArraySection_t>(expr)->m_v);
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        return type != nullptr
            && ASRUtils::is_array(type)
            && ASRUtils::is_pointer(type);
    }

    bool is_c_rank1_vector_subscript_array_item_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::ArrayItem_t>(*expr)) {
            return false;
        }
        ASR::ttype_t *expr_type = ASRUtils::expr_type(expr);
        if (!ASRUtils::is_array(expr_type)
                || ASRUtils::extract_n_dims_from_ttype(expr_type) != 1
                || !is_c_scalarizable_element_type(expr_type)) {
            return false;
        }
        ASR::ArrayItem_t *item = ASR::down_cast<ASR::ArrayItem_t>(expr);
        if (is_fixed_size_struct_member_array_expr(item->m_v)) {
            return false;
        }
        size_t vector_dims = 0;
        for (size_t i = 0; i < item->n_args; i++) {
            ASR::expr_t *idx_expr = nullptr;
            if (item->m_args[i].m_right) {
                idx_expr = item->m_args[i].m_right;
            } else if (item->m_args[i].m_left) {
                idx_expr = item->m_args[i].m_left;
            }
            if (idx_expr == nullptr) {
                return false;
            }
            idx_expr = ASRUtils::get_past_array_physical_cast(idx_expr);
            ASR::expr_t *idx_value = ASRUtils::expr_value(idx_expr);
            if (idx_value == nullptr) {
                idx_value = idx_expr;
            }
            if (ASRUtils::is_array(ASRUtils::expr_type(idx_expr))
                    || ASR::is_a<ASR::ArrayConstant_t>(*idx_value)
                    || ASR::is_a<ASR::ArrayConstructor_t>(*idx_value)
                    || ASR::is_a<ASR::ArrayReshape_t>(*idx_value)
                    || ASR::is_a<ASR::ArrayBroadcast_t>(*idx_value)) {
                vector_dims++;
            }
        }
        return vector_dims == 1;
    }

    bool is_c_rank1_scalarizable_array_expr(ASR::expr_t *expr) const {
        return is_c_rank1_unit_array_expr(expr)
            || is_fixed_size_struct_member_array_expr(expr)
            || is_c_struct_member_vector_subscript_array_expr(expr)
            || is_c_rank1_vector_subscript_array_item_expr(expr);
    }

    bool is_c_rank1_matmul_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::IntrinsicArrayFunction_t>(*expr)) {
            return false;
        }
        ASR::IntrinsicArrayFunction_t *matmul =
            ASR::down_cast<ASR::IntrinsicArrayFunction_t>(expr);
        if (static_cast<ASRUtils::IntrinsicArrayFunctions>(
                matmul->m_arr_intrinsic_id) != ASRUtils::IntrinsicArrayFunctions::MatMul
                || matmul->n_args != 2
                || matmul->m_args[0] == nullptr
                || matmul->m_args[1] == nullptr) {
            return false;
        }
        ASR::ttype_t *result_type = ASRUtils::type_get_past_allocatable_pointer(
            matmul->m_type);
        if (result_type == nullptr
                || !ASRUtils::is_array(result_type)
                || ASRUtils::extract_n_dims_from_ttype(result_type) != 1
                || !is_c_scalarizable_element_type(result_type)) {
            return false;
        }
        ASR::ttype_t *left_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(matmul->m_args[0]));
        ASR::ttype_t *right_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(matmul->m_args[1]));
        if (left_type == nullptr || right_type == nullptr
                || !ASRUtils::is_array(left_type)
                || !ASRUtils::is_array(right_type)
                || !is_c_scalarizable_element_type(left_type)
                || !is_c_scalarizable_element_type(right_type)) {
            return false;
        }
        int left_rank = ASRUtils::extract_n_dims_from_ttype(left_type);
        int right_rank = ASRUtils::extract_n_dims_from_ttype(right_type);
        return (left_rank == 2 && right_rank == 1)
            || (left_rank == 1 && right_rank == 2);
    }

    bool c_expr_contains_rank1_matmul_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::IntrinsicArrayFunction: {
                if (is_c_rank1_matmul_expr(expr)) {
                    return true;
                }
                ASR::IntrinsicArrayFunction_t *iaf =
                    ASR::down_cast<ASR::IntrinsicArrayFunction_t>(expr);
                for (size_t i = 0; i < iaf->n_args; i++) {
                    if (c_expr_contains_rank1_matmul_expr(iaf->m_args[i])) {
                        return true;
                    }
                }
                return false;
            }
            case ASR::exprType::ArrayBroadcast: {
                return c_expr_contains_rank1_matmul_expr(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return c_expr_contains_rank1_matmul_expr(binop->m_left)
                    || c_expr_contains_rank1_matmul_expr(binop->m_right);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop =
                    ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return c_expr_contains_rank1_matmul_expr(binop->m_left)
                    || c_expr_contains_rank1_matmul_expr(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return c_expr_contains_rank1_matmul_expr(binop->m_left)
                    || c_expr_contains_rank1_matmul_expr(binop->m_right);
            }
            case ASR::exprType::RealUnaryMinus: {
                return c_expr_contains_rank1_matmul_expr(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return c_expr_contains_rank1_matmul_expr(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_scalarizable_array_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
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
                return is_c_rank1_scalarizable_array_expr(expr);
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
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
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
            case ASR::exprType::IntrinsicArrayFunction: {
                return is_c_rank1_matmul_expr(expr);
            }
            case ASR::exprType::IntrinsicElementalFunction: {
                ASR::IntrinsicElementalFunction_t *ief =
                    ASR::down_cast<ASR::IntrinsicElementalFunction_t>(expr);
                using IEF = ASRUtils::IntrinsicElementalFunctions;
                switch (static_cast<IEF>(ief->m_intrinsic_id)) {
                    case IEF::Abs:
                    case IEF::Sin:
                    case IEF::Cos:
                    case IEF::Tan:
                    case IEF::Exp:
                    case IEF::Sqrt:
                    case IEF::Max:
                    case IEF::Min:
                    case IEF::FMA: {
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

    bool is_c_plain_scalar_expr_for_section_fill(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || ASRUtils::is_array(type)
                || ASRUtils::is_character(*type)) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::IntegerConstant:
            case ASR::exprType::UnsignedIntegerConstant:
            case ASR::exprType::RealConstant:
            case ASR::exprType::LogicalConstant:
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember: {
                return true;
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_plain_scalar_expr_for_section_fill(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_plain_scalar_expr_for_section_fill(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_plain_scalar_expr_for_section_fill(binop->m_left)
                    && is_c_plain_scalar_expr_for_section_fill(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_plain_scalar_expr_for_section_fill(binop->m_left)
                    && is_c_plain_scalar_expr_for_section_fill(binop->m_right);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && is_c_plain_scalar_expr_for_section_fill(binop->m_left)
                    && is_c_plain_scalar_expr_for_section_fill(binop->m_right);
            }
            case ASR::exprType::Cast: {
                return is_c_plain_scalar_expr_for_section_fill(
                    ASR::down_cast<ASR::Cast_t>(expr)->m_arg);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_scalarizable_array_constructor_arg(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(expr));
        if (type == nullptr || ASRUtils::is_array(type)
                || ASRUtils::is_character(*type)) {
            return false;
        }
        switch (expr->type) {
            case ASR::exprType::ArrayItem: {
                return ASRUtils::is_integer(*type)
                    || ASRUtils::is_unsigned_integer(*type)
                    || ASRUtils::is_real(*type)
                    || ASRUtils::is_logical(*type);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return is_c_scalarizable_array_constructor_arg(
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::RealUnaryMinus: {
                return is_c_scalarizable_array_constructor_arg(
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop = ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return is_c_scalarizable_array_constructor_arg(binop->m_left)
                    && is_c_scalarizable_array_constructor_arg(binop->m_right);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return is_c_scalarizable_array_constructor_arg(binop->m_left)
                    && is_c_scalarizable_array_constructor_arg(binop->m_right);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return is_c_scalarizable_array_constructor_arg(binop->m_left)
                    && is_c_scalarizable_array_constructor_arg(binop->m_right);
            }
            case ASR::exprType::Cast: {
                return is_c_scalarizable_array_constructor_arg(
                    ASR::down_cast<ASR::Cast_t>(expr)->m_arg);
            }
            default: {
                return is_c_plain_scalar_expr_for_section_fill(expr);
            }
        }
    }

    bool is_c_plain_scalar_array_constructor(const ASR::ArrayConstructor_t &x) const {
        ASR::ttype_t *type = ASRUtils::type_get_past_allocatable_pointer(x.m_type);
        if (type == nullptr
                || ASRUtils::extract_n_dims_from_ttype(type) != 1
                || !is_c_scalarizable_element_type(type)) {
            return false;
        }
        for (size_t i = 0; i < x.n_args; i++) {
            if (ASR::is_a<ASR::ImpliedDoLoop_t>(*x.m_args[i])
                    || !is_c_scalarizable_array_constructor_arg(x.m_args[i])) {
                return false;
            }
        }
        return true;
    }

    bool is_c_rank2_plain_constructor_reshape(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::ArrayReshape_t>(*expr)) {
            return false;
        }
        ASR::ArrayReshape_t *reshape = ASR::down_cast<ASR::ArrayReshape_t>(expr);
        ASR::ttype_t *reshape_type = ASRUtils::type_get_past_allocatable_pointer(
            reshape->m_type);
        ASR::ttype_t *source_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(reshape->m_array));
        ASR::ttype_t *reshape_element_type = reshape_type != nullptr
            ? ASRUtils::type_get_past_array(reshape_type) : nullptr;
        ASR::ttype_t *source_element_type = source_type != nullptr
            ? ASRUtils::type_get_past_array(source_type) : nullptr;
        ASR::expr_t *source = ASRUtils::get_past_array_physical_cast(
            reshape->m_array);
        return reshape->m_pad == nullptr
            && reshape->m_order == nullptr
            && get_array_constant_value(reshape->m_shape) != nullptr
            && reshape_type != nullptr
            && source_type != nullptr
            && ASRUtils::extract_n_dims_from_ttype(reshape_type) == 2
            && ASRUtils::extract_n_dims_from_ttype(source_type) == 1
            && ASRUtils::is_fixed_size_array(reshape_type)
            && reshape_element_type != nullptr
            && source_element_type != nullptr
            && is_c_scalarizable_element_type(reshape_element_type)
            && is_c_scalarizable_element_type(source_element_type)
            && ASRUtils::types_equal(reshape_element_type, source_element_type,
                nullptr, nullptr)
            && source != nullptr
            && ASR::is_a<ASR::ArrayConstructor_t>(*source)
            && is_c_plain_scalar_array_constructor(
                *ASR::down_cast<ASR::ArrayConstructor_t>(source));
    }

    bool is_c_rank2_scalarizable_reshape(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::ArrayReshape_t>(*expr)) {
            return false;
        }
        ASR::ArrayReshape_t *reshape = ASR::down_cast<ASR::ArrayReshape_t>(expr);
        ASR::ttype_t *reshape_type = ASRUtils::type_get_past_allocatable_pointer(
            reshape->m_type);
        ASR::ttype_t *source_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(reshape->m_array));
        ASR::ttype_t *reshape_element_type = reshape_type != nullptr
            ? ASRUtils::type_get_past_array(reshape_type) : nullptr;
        ASR::ttype_t *source_element_type = source_type != nullptr
            ? ASRUtils::type_get_past_array(source_type) : nullptr;
        ASR::expr_t *source = ASRUtils::get_past_array_physical_cast(
            reshape->m_array);
        bool source_scalarizable = source != nullptr
            && (is_c_rank1_scalarizable_array_expr(source)
                || (ASR::is_a<ASR::ArrayConstructor_t>(*source)
                    && is_c_plain_scalar_array_constructor(
                        *ASR::down_cast<ASR::ArrayConstructor_t>(source))));
        return reshape->m_pad == nullptr
            && reshape->m_order == nullptr
            && get_array_constant_value(reshape->m_shape) != nullptr
            && reshape_type != nullptr
            && source_type != nullptr
            && ASRUtils::extract_n_dims_from_ttype(reshape_type) == 2
            && ASRUtils::extract_n_dims_from_ttype(source_type) == 1
            && ASRUtils::is_fixed_size_array(reshape_type)
            && reshape_element_type != nullptr
            && source_element_type != nullptr
            && is_c_scalarizable_element_type(reshape_element_type)
            && is_c_scalarizable_element_type(source_element_type)
            && ASRUtils::types_equal(reshape_element_type, source_element_type,
                nullptr, nullptr)
            && source_scalarizable;
    }

    bool get_c_rank2_spread_dim(const ASR::IntrinsicArrayFunction_t &x,
            int64_t &dim) const {
        if (x.m_arr_intrinsic_id != static_cast<int64_t>(
                    ASRUtils::IntrinsicArrayFunctions::Spread)
                || x.n_args != 3
                || x.m_args[0] == nullptr
                || x.m_args[1] == nullptr
                || x.m_args[2] == nullptr
                || !ASRUtils::extract_value(x.m_args[1], dim)) {
            return false;
        }
        return dim == 1 || dim == 2;
    }

    bool is_c_rank2_spread_scalarizable_expr(
            const ASR::IntrinsicArrayFunction_t &x) const {
        int64_t dim = -1;
        if (!get_c_rank2_spread_dim(x, dim)) {
            return false;
        }
        ASR::ttype_t *source_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(x.m_args[0]));
        ASR::ttype_t *result_type = ASRUtils::type_get_past_allocatable_pointer(
            x.m_type);
        ASR::ttype_t *source_element_type = source_type != nullptr
            ? ASRUtils::type_get_past_array(source_type) : nullptr;
        ASR::ttype_t *result_element_type = result_type != nullptr
            ? ASRUtils::type_get_past_array(result_type) : nullptr;
        return source_type != nullptr
            && result_type != nullptr
            && source_element_type != nullptr
            && result_element_type != nullptr
            && ASRUtils::extract_n_dims_from_ttype(source_type) == 1
            && ASRUtils::extract_n_dims_from_ttype(result_type) == 2
            && is_c_scalarizable_element_type(source_type)
            && is_c_scalarizable_element_type(result_type)
            && ASRUtils::types_equal(source_element_type, result_element_type,
                nullptr, nullptr)
            && is_c_scalarizable_array_expr(x.m_args[0]);
    }

    bool is_c_rank2_transpose_scalarizable_expr(
            const ASR::IntrinsicArrayFunction_t &x) const {
        if (x.m_arr_intrinsic_id != static_cast<int64_t>(
                    ASRUtils::IntrinsicArrayFunctions::Transpose)
                || x.n_args != 1
                || x.m_args[0] == nullptr) {
            return false;
        }
        ASR::ttype_t *source_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(x.m_args[0]));
        ASR::ttype_t *result_type = ASRUtils::type_get_past_allocatable_pointer(
            x.m_type);
        ASR::ttype_t *source_element_type = source_type != nullptr
            ? ASRUtils::type_get_past_array(source_type) : nullptr;
        ASR::ttype_t *result_element_type = result_type != nullptr
            ? ASRUtils::type_get_past_array(result_type) : nullptr;
        return source_type != nullptr
            && result_type != nullptr
            && source_element_type != nullptr
            && result_element_type != nullptr
            && ASRUtils::extract_n_dims_from_ttype(source_type) == 2
            && ASRUtils::extract_n_dims_from_ttype(result_type) == 2
            && is_c_scalarizable_element_type(source_type)
            && is_c_scalarizable_element_type(result_type)
            && ASRUtils::types_equal(source_element_type, result_element_type,
                nullptr, nullptr)
            && is_c_rank2_scalarizable_array_expr(x.m_args[0]);
    }

    bool is_c_rank2_scalarizable_array_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        ASR::ttype_t *array_type = ASRUtils::type_get_past_allocatable_pointer(type);
        if ((array_type == nullptr || !ASRUtils::is_array(array_type))
                && !ASRUtils::is_array_t(expr)) {
            return true;
        }
        if (!is_c_scalarizable_element_type(array_type)) {
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
                return is_c_rank2_scalarizable_reshape(expr);
            }
            case ASR::exprType::IntrinsicArrayFunction: {
                ASR::IntrinsicArrayFunction_t *iaf =
                    ASR::down_cast<ASR::IntrinsicArrayFunction_t>(expr);
                return is_c_rank2_spread_scalarizable_expr(*iaf)
                    || is_c_rank2_transpose_scalarizable_expr(*iaf);
            }
            default: {
                return false;
            }
        }
    }

    bool is_c_rank2_matmul_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !ASR::is_a<ASR::IntrinsicArrayFunction_t>(*expr)) {
            return false;
        }
        ASR::IntrinsicArrayFunction_t *matmul =
            ASR::down_cast<ASR::IntrinsicArrayFunction_t>(expr);
        if (static_cast<ASRUtils::IntrinsicArrayFunctions>(
                matmul->m_arr_intrinsic_id) != ASRUtils::IntrinsicArrayFunctions::MatMul
                || matmul->n_args != 2
                || matmul->m_args[0] == nullptr
                || matmul->m_args[1] == nullptr) {
            return false;
        }
        ASR::ttype_t *result_type = ASRUtils::type_get_past_allocatable_pointer(
            matmul->m_type);
        ASR::ttype_t *left_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(matmul->m_args[0]));
        ASR::ttype_t *right_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(matmul->m_args[1]));
        return result_type != nullptr
            && left_type != nullptr
            && right_type != nullptr
            && ASRUtils::extract_n_dims_from_ttype(result_type) == 2
            && ASRUtils::extract_n_dims_from_ttype(left_type) == 2
            && ASRUtils::extract_n_dims_from_ttype(right_type) == 2
            && is_c_scalarizable_element_type(result_type)
            && is_c_rank2_scalarizable_array_expr(matmul->m_args[0])
            && is_c_rank2_scalarizable_array_expr(matmul->m_args[1]);
    }

    ASR::symbol_t *get_c_array_assignment_root_symbol(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        while (expr != nullptr) {
            if (ASR::is_a<ASR::ArrayItem_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArrayItem_t>(expr)->m_v;
                expr = ASRUtils::get_past_array_physical_cast(expr);
                continue;
            }
            if (ASR::is_a<ASR::ArraySection_t>(*expr)) {
                expr = ASR::down_cast<ASR::ArraySection_t>(expr)->m_v;
                expr = ASRUtils::get_past_array_physical_cast(expr);
                continue;
            }
            break;
        }
        if (expr != nullptr && ASR::is_a<ASR::Var_t>(*expr)) {
            return ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Var_t>(expr)->m_v);
        }
        return nullptr;
    }

    bool c_same_simple_index_expr(ASR::expr_t *lhs, ASR::expr_t *rhs) const {
        if (lhs == nullptr || rhs == nullptr) {
            return lhs == rhs;
        }
        lhs = ASRUtils::get_past_array_physical_cast(lhs);
        rhs = ASRUtils::get_past_array_physical_cast(rhs);
        if (lhs == nullptr || rhs == nullptr || lhs->type != rhs->type) {
            return false;
        }
        switch (lhs->type) {
            case ASR::exprType::Var: {
                return ASR::down_cast<ASR::Var_t>(lhs)->m_v
                    == ASR::down_cast<ASR::Var_t>(rhs)->m_v;
            }
            case ASR::exprType::IntegerConstant: {
                return ASR::down_cast<ASR::IntegerConstant_t>(lhs)->m_n
                    == ASR::down_cast<ASR::IntegerConstant_t>(rhs)->m_n;
            }
            case ASR::exprType::ArrayBound: {
                ASR::ArrayBound_t *l = ASR::down_cast<ASR::ArrayBound_t>(lhs);
                ASR::ArrayBound_t *r = ASR::down_cast<ASR::ArrayBound_t>(rhs);
                return l->m_bound == r->m_bound
                    && c_same_simple_index_expr(l->m_dim, r->m_dim)
                    && c_same_array_reference(l->m_v, r->m_v);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *l = ASR::down_cast<ASR::IntegerBinOp_t>(lhs);
                ASR::IntegerBinOp_t *r = ASR::down_cast<ASR::IntegerBinOp_t>(rhs);
                return l->m_op == r->m_op
                    && c_same_simple_index_expr(l->m_left, r->m_left)
                    && c_same_simple_index_expr(l->m_right, r->m_right);
            }
            default: {
                return lhs == rhs;
            }
        }
    }

    bool c_same_array_index(const ASR::array_index_t &lhs,
            const ASR::array_index_t &rhs) const {
        return c_same_simple_index_expr(lhs.m_left, rhs.m_left)
            && c_same_simple_index_expr(lhs.m_right, rhs.m_right)
            && c_same_simple_index_expr(lhs.m_step, rhs.m_step);
    }

    bool c_same_array_reference(ASR::expr_t *lhs, ASR::expr_t *rhs) const {
        lhs = ASRUtils::get_past_array_physical_cast(lhs);
        rhs = ASRUtils::get_past_array_physical_cast(rhs);
        if (lhs == nullptr || rhs == nullptr || lhs->type != rhs->type) {
            return false;
        }
        switch (lhs->type) {
            case ASR::exprType::Var: {
                return ASR::down_cast<ASR::Var_t>(lhs)->m_v
                    == ASR::down_cast<ASR::Var_t>(rhs)->m_v;
            }
            case ASR::exprType::StructInstanceMember: {
                ASR::StructInstanceMember_t *l =
                    ASR::down_cast<ASR::StructInstanceMember_t>(lhs);
                ASR::StructInstanceMember_t *r =
                    ASR::down_cast<ASR::StructInstanceMember_t>(rhs);
                return l->m_m == r->m_m
                    && c_same_array_reference(l->m_v, r->m_v);
            }
            case ASR::exprType::ArraySection: {
                ASR::ArraySection_t *l = ASR::down_cast<ASR::ArraySection_t>(lhs);
                ASR::ArraySection_t *r = ASR::down_cast<ASR::ArraySection_t>(rhs);
                if (l->n_args != r->n_args
                        || !c_same_array_reference(l->m_v, r->m_v)) {
                    return false;
                }
                for (size_t i = 0; i < l->n_args; i++) {
                    if (!c_same_array_index(l->m_args[i], r->m_args[i])) {
                        return false;
                    }
                }
                return true;
            }
            case ASR::exprType::ArrayItem: {
                ASR::ArrayItem_t *l = ASR::down_cast<ASR::ArrayItem_t>(lhs);
                ASR::ArrayItem_t *r = ASR::down_cast<ASR::ArrayItem_t>(rhs);
                if (l->n_args != r->n_args
                        || !c_same_array_reference(l->m_v, r->m_v)) {
                    return false;
                }
                for (size_t i = 0; i < l->n_args; i++) {
                    if (!c_same_array_index(l->m_args[i], r->m_args[i])) {
                        return false;
                    }
                }
                return true;
            }
            default: {
                return lhs == rhs;
            }
        }
    }

    bool is_c_whole_rank1_array_reference(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || !is_c_rank1_scalarizable_array_expr(expr)) {
            return false;
        }
        if (!ASR::is_a<ASR::ArraySection_t>(*expr)) {
            return true;
        }
        ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(expr);
        ASR::expr_t *base = ASRUtils::get_past_array_physical_cast(section->m_v);
        ASR::ttype_t *base_type = base != nullptr
            ? ASRUtils::type_get_past_allocatable_pointer(ASRUtils::expr_type(base))
            : nullptr;
        if (base_type == nullptr || !ASRUtils::is_array(base_type)
                || ASRUtils::extract_n_dims_from_ttype(base_type) != 1
                || section->n_args != 1) {
            return false;
        }
        return is_default_unit_array_slice(section->m_args[0]);
    }

    bool c_expr_references_root_symbol(ASR::expr_t *expr, ASR::symbol_t *root_sym) const {
        if (expr == nullptr || root_sym == nullptr) {
            return false;
        }
        expr = ASRUtils::get_past_array_physical_cast(expr);
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
                    if (c_expr_references_root_symbol(item->m_args[i].m_left, root_sym)
                            || c_expr_references_root_symbol(item->m_args[i].m_right, root_sym)
                            || c_expr_references_root_symbol(item->m_args[i].m_step, root_sym)) {
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
                    if (c_expr_references_root_symbol(section->m_args[i].m_left, root_sym)
                            || c_expr_references_root_symbol(section->m_args[i].m_right, root_sym)
                            || c_expr_references_root_symbol(section->m_args[i].m_step, root_sym)) {
                        return true;
                    }
                }
                return false;
            }
            case ASR::exprType::StructInstanceMember: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::StructInstanceMember_t>(expr)->m_v, root_sym);
            }
            case ASR::exprType::ArrayBroadcast: {
                return c_expr_references_root_symbol(
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array, root_sym);
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
            case ASR::exprType::IntrinsicArrayFunction: {
                ASR::IntrinsicArrayFunction_t *iaf =
                    ASR::down_cast<ASR::IntrinsicArrayFunction_t>(expr);
                for (size_t i = 0; i < iaf->n_args; i++) {
                    if (c_expr_references_root_symbol(iaf->m_args[i], root_sym)) {
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

    bool c_rank1_ref_same_target(ASR::expr_t *target, ASR::expr_t *expr) const {
        target = ASRUtils::get_past_array_physical_cast(target);
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (!is_c_rank1_scalarizable_array_expr(target)
                || !is_c_rank1_scalarizable_array_expr(expr)) {
            return false;
        }
        ASR::symbol_t *target_root = get_c_array_assignment_root_symbol(target);
        ASR::symbol_t *expr_root = get_c_array_assignment_root_symbol(expr);
        if (target_root == nullptr || target_root != expr_root) {
            return false;
        }
        return c_same_array_reference(target, expr)
            || (is_c_whole_rank1_array_reference(target)
                && is_c_whole_rank1_array_reference(expr));
    }

    bool is_c_rank1_matmul_nonself_expr(ASR::expr_t *expr,
            ASR::symbol_t *target_root) const {
        return is_c_scalarizable_array_expr(expr)
            && c_expr_contains_rank1_matmul_expr(expr)
            && !c_expr_references_root_symbol(expr, target_root);
    }

    template <typename BinOp>
    bool c_rank1_matmul_binop_has_one_self_ref(
            const BinOp *binop, ASR::expr_t *target,
            ASR::symbol_t *target_root) const {
        bool left_is_self = c_rank1_ref_same_target(target, binop->m_left);
        bool right_is_self = c_rank1_ref_same_target(target, binop->m_right);
        if (left_is_self == right_is_self) {
            return false;
        }
        ASR::expr_t *other = left_is_self ? binop->m_right : binop->m_left;
        return is_c_rank1_matmul_nonself_expr(other, target_root);
    }

    bool c_rank1_matmul_self_update_expr_has_single_target_ref(
            ASR::expr_t *target, ASR::symbol_t *target_root,
            ASR::expr_t *expr, int &self_refs) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr || target_root == nullptr) {
            return false;
        }
        if (c_rank1_ref_same_target(target, expr)) {
            self_refs++;
            return self_refs <= 1;
        }
        switch (expr->type) {
            case ASR::exprType::Var:
            case ASR::exprType::StructInstanceMember:
            case ASR::exprType::ArraySection:
            case ASR::exprType::ArrayItem:
            case ASR::exprType::IntrinsicArrayFunction: {
                return !c_expr_references_root_symbol(expr, target_root)
                    && is_c_scalarizable_array_expr(expr);
            }
            case ASR::exprType::ArrayBroadcast: {
                return c_rank1_matmul_self_update_expr_has_single_target_ref(
                    target, target_root,
                    ASR::down_cast<ASR::ArrayBroadcast_t>(expr)->m_array,
                    self_refs);
            }
            case ASR::exprType::RealBinOp: {
                ASR::RealBinOp_t *binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && c_rank1_matmul_self_update_expr_has_single_target_ref(
                        target, target_root, binop->m_left, self_refs)
                    && c_rank1_matmul_self_update_expr_has_single_target_ref(
                        target, target_root, binop->m_right, self_refs);
            }
            case ASR::exprType::IntegerBinOp: {
                ASR::IntegerBinOp_t *binop =
                    ASR::down_cast<ASR::IntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && c_rank1_matmul_self_update_expr_has_single_target_ref(
                        target, target_root, binop->m_left, self_refs)
                    && c_rank1_matmul_self_update_expr_has_single_target_ref(
                        target, target_root, binop->m_right, self_refs);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                ASR::UnsignedIntegerBinOp_t *binop =
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(expr);
                return binop->m_op != ASR::binopType::Pow
                    && c_rank1_matmul_self_update_expr_has_single_target_ref(
                        target, target_root, binop->m_left, self_refs)
                    && c_rank1_matmul_self_update_expr_has_single_target_ref(
                        target, target_root, binop->m_right, self_refs);
            }
            case ASR::exprType::RealUnaryMinus: {
                return c_rank1_matmul_self_update_expr_has_single_target_ref(
                    target, target_root,
                    ASR::down_cast<ASR::RealUnaryMinus_t>(expr)->m_arg,
                    self_refs);
            }
            case ASR::exprType::IntegerUnaryMinus: {
                return c_rank1_matmul_self_update_expr_has_single_target_ref(
                    target, target_root,
                    ASR::down_cast<ASR::IntegerUnaryMinus_t>(expr)->m_arg,
                    self_refs);
            }
            default: {
                return !c_expr_references_root_symbol(expr, target_root)
                    && is_c_scalarizable_array_expr(expr);
            }
        }
    }

    bool should_leave_rank1_matmul_self_update_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        if (!pass_options.c_backend || target == nullptr || value == nullptr
                || !is_c_rank1_scalarizable_array_expr(target)
                || !is_c_scalarizable_array_expr(value)
                || !c_expr_contains_rank1_matmul_expr(value)) {
            return false;
        }
        ASR::symbol_t *target_root = get_c_array_assignment_root_symbol(target);
        if (target_root == nullptr) {
            return false;
        }
        int self_refs = 0;
        return c_rank1_matmul_self_update_expr_has_single_target_ref(
                target, target_root, value, self_refs)
            && self_refs == 1;
    }

    bool should_leave_scalarizable_array_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        return pass_options.c_backend
            && target != nullptr
            && value != nullptr
            && !is_compiler_created_array_temp_expr(target)
            && !is_whole_allocatable_or_pointer_array_target(target)
            && is_c_rank1_scalarizable_array_expr(target)
            && is_c_scalarizable_array_expr(value)
            && !c_expr_references_root_symbol(value,
                get_c_array_assignment_root_symbol(target));
    }

    bool should_leave_rank2_plain_data_copy_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        return pass_options.c_backend
            && target != nullptr
            && value != nullptr
            && !is_compiler_created_array_temp_expr(target)
            && !is_whole_allocatable_or_pointer_array_target(target)
            && !ASRUtils::is_allocatable(ASRUtils::expr_type(target))
            && !is_c_pointer_array_base_expr(target)
            && !is_c_pointer_array_base_expr(value)
            && is_c_rank2_unit_slice_array_expr(target)
            && is_c_rank2_plain_data_copy_expr(value)
            && !c_expr_references_root_symbol(value,
                get_c_array_assignment_root_symbol(target));
    }

    bool should_leave_rank2_scalarizable_array_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        return pass_options.c_backend
            && target != nullptr
            && value != nullptr
            && !is_compiler_created_array_temp_expr(target)
            && !is_whole_allocatable_or_pointer_array_target(target)
            && !ASRUtils::is_allocatable(ASRUtils::expr_type(target))
            && !is_c_pointer_array_base_expr(target)
            && !is_c_pointer_array_base_expr(value)
            && is_c_rank2_full_array_expr(target)
            && is_c_rank2_scalarizable_array_expr(value)
            && c_plain_array_copy_element_types_match(target, value)
            && !c_expr_references_root_symbol(value,
                get_c_array_assignment_root_symbol(target));
    }

    bool should_leave_rank2_allocatable_matmul_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        return pass_options.c_backend
            && target != nullptr
            && value != nullptr
            && is_whole_allocatable_or_pointer_array_target(target)
            && ASRUtils::is_allocatable(ASRUtils::expr_type(target))
            && !is_compiler_created_array_temp_expr(target)
            && !is_c_pointer_array_base_expr(target)
            && is_c_rank2_unit_slice_array_expr(target)
            && is_c_rank2_matmul_expr(value)
            && c_plain_array_copy_element_types_match(target, value)
            && !c_expr_references_root_symbol(value,
                get_c_array_assignment_root_symbol(target));
    }

    bool should_leave_rank2_section_scalar_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        target = ASRUtils::get_past_array_physical_cast(target);
        if (!pass_options.c_backend
                || target == nullptr
                || value == nullptr
                || !ASR::is_a<ASR::ArraySection_t>(*target)
                || !is_c_rank2_unit_slice_array_expr(target)
                || !is_c_plain_scalar_expr_for_section_fill(value)) {
            return false;
        }
        ASR::ttype_t *target_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(target));
        ASR::ttype_t *target_element_type = target_type
            ? ASRUtils::type_get_past_array(target_type) : nullptr;
        ASR::ttype_t *value_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(value));
        return target_element_type != nullptr
            && value_type != nullptr
            && ASRUtils::types_equal(
                target_element_type, value_type, nullptr, nullptr);
    }

    bool should_leave_reallocated_temp_plain_data_copy_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        if (!pass_options.c_backend || target == nullptr || value == nullptr) {
            return false;
        }
        target = unwrap_array_op_lvalue(target);
        value = unwrap_array_op_lvalue(value);
        if (target == nullptr || value == nullptr
                || !is_whole_allocatable_or_pointer_array_target(target)
                || !ASRUtils::is_allocatable(ASRUtils::expr_type(target))) {
            return false;
        }
        bool temp_copy_context = is_compiler_created_array_temp_expr(target)
            || is_compiler_created_array_temp_expr(value);
        if (!temp_copy_context
                || !c_plain_array_copy_element_types_match(target, value)
                || is_c_pointer_array_base_expr(target)) {
            return false;
        }
        bool value_is_pointer_base = is_c_pointer_array_base_expr(value);
        if (value_is_pointer_base && !is_compiler_created_array_temp_expr(target)) {
            return false;
        }
        bool rank1_copy = is_c_rank1_unit_array_expr(target)
            && is_c_rank1_plain_data_copy_expr(value);
        bool rank2_copy = is_c_rank2_unit_slice_array_expr(target)
            && is_c_rank2_plain_data_copy_expr(value);
        return (rank1_copy || rank2_copy)
            && !c_expr_references_root_symbol(value,
                get_c_array_assignment_root_symbol(target));
    }

    bool should_leave_allocatable_self_section_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        if (!pass_options.c_backend || target == nullptr || value == nullptr) {
            return false;
        }
        target = unwrap_array_op_lvalue(target);
        value = unwrap_array_op_lvalue(value);
        if (target == nullptr || value == nullptr
                || !is_whole_allocatable_or_pointer_array_target(target)
                || !ASRUtils::is_allocatable(ASRUtils::expr_type(target))
                || !ASR::is_a<ASR::ArraySection_t>(*value)
                || !c_plain_array_copy_element_types_match(target, value)) {
            return false;
        }
        ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(value);
        ASR::symbol_t *target_root = get_c_array_assignment_root_symbol(target);
        ASR::symbol_t *value_root = get_c_array_assignment_root_symbol(section->m_v);
        if (target_root == nullptr || target_root != value_root) {
            return false;
        }
        ASR::ttype_t *target_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(target));
        int target_rank = ASRUtils::extract_n_dims_from_ttype(target_type);
        if (target_rank == 1) {
            return is_c_rank1_unit_array_expr(value);
        }
        if (target_rank == 2) {
            return is_c_rank2_unit_slice_array_expr(value);
        }
        return false;
    }

    bool is_c_rank2_nonself_update_expr(ASR::expr_t *expr) const {
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (expr == nullptr) {
            return false;
        }
        ASR::ttype_t *type = ASRUtils::expr_type(expr);
        ASR::ttype_t *array_type = ASRUtils::type_get_past_allocatable_pointer(type);
        if ((array_type == nullptr || !ASRUtils::is_array(array_type))
                && !ASRUtils::is_array_t(expr)) {
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
            case ASR::exprType::IntrinsicArrayFunction: {
                ASR::IntrinsicArrayFunction_t *iaf =
                    ASR::down_cast<ASR::IntrinsicArrayFunction_t>(expr);
                return is_c_rank2_spread_scalarizable_expr(*iaf)
                    || is_c_rank2_transpose_scalarizable_expr(*iaf);
            }
            default: {
                return false;
            }
        }
    }

    template <typename BinOp>
    bool c_rank2_binop_has_one_full_self_ref(const BinOp *binop,
            ASR::expr_t *target_expr, ASR::symbol_t *target_root) const {
        bool left_is_self = c_rank2_full_ref_same_target(target_expr, binop->m_left);
        bool right_is_self = c_rank2_full_ref_same_target(target_expr, binop->m_right);
        if (left_is_self == right_is_self) {
            return false;
        }
        ASR::expr_t *other = left_is_self ? binop->m_right : binop->m_left;
        return is_c_rank2_nonself_update_expr(other)
            && !c_expr_references_root_symbol(other, target_root);
    }

    bool c_rank2_full_ref_same_target(ASR::expr_t *target, ASR::expr_t *expr) const {
        target = ASRUtils::get_past_array_physical_cast(target);
        expr = ASRUtils::get_past_array_physical_cast(expr);
        if (target == nullptr || expr == nullptr
                || !is_c_rank2_full_array_expr(target)
                || !is_c_rank2_full_array_expr(expr)) {
            return false;
        }
        ASR::symbol_t *target_root = get_c_array_assignment_root_symbol(target);
        ASR::symbol_t *expr_root = get_c_array_assignment_root_symbol(expr);
        return target_root != nullptr && target_root == expr_root;
    }

    bool should_leave_rank2_full_self_update_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        if (target == nullptr
                || value == nullptr
                || (!ASRUtils::is_array(ASRUtils::expr_type(value))
                    && !ASRUtils::is_array_t(value))
                || !is_c_rank2_full_array_expr(target)
                || !is_c_rank2_scalarizable_array_expr(value)) {
            return false;
        }
        ASR::symbol_t *target_root = get_c_array_assignment_root_symbol(target);
        if (target_root == nullptr) {
            return false;
        }
        switch (ASRUtils::get_past_array_physical_cast(value)->type) {
            case ASR::exprType::RealBinOp: {
                return c_rank2_binop_has_one_full_self_ref(
                    ASR::down_cast<ASR::RealBinOp_t>(
                        ASRUtils::get_past_array_physical_cast(value)),
                    target, target_root);
            }
            case ASR::exprType::IntegerBinOp: {
                return c_rank2_binop_has_one_full_self_ref(
                    ASR::down_cast<ASR::IntegerBinOp_t>(
                        ASRUtils::get_past_array_physical_cast(value)),
                    target, target_root);
            }
            case ASR::exprType::UnsignedIntegerBinOp: {
                return c_rank2_binop_has_one_full_self_ref(
                    ASR::down_cast<ASR::UnsignedIntegerBinOp_t>(
                        ASRUtils::get_past_array_physical_cast(value)),
                    target, target_root);
            }
            default: {
                return false;
            }
        }
    }

    ASR::expr_t *get_c_section_left_bound(
            ASR::ArraySection_t *section, const Location & /*loc*/) {
        if (section->m_args[0].m_left != nullptr) {
            return section->m_args[0].m_left;
        }
        return PassUtils::get_bound(section->m_v, 1, "lbound", al, get_index_kind());
    }

    ASR::expr_t *get_c_section_right_bound(
            ASR::ArraySection_t *section, const Location & /*loc*/) {
        if (section->m_args[0].m_right != nullptr) {
            return section->m_args[0].m_right;
        }
        return PassUtils::get_bound(section->m_v, 1, "ubound", al, get_index_kind());
    }

    ASR::expr_t *get_c_section_loop_lbound(const Location &loc) {
        return make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t,
            1, get_index_kind(), loc);
    }

    ASR::expr_t *get_c_section_loop_ubound(
            ASR::ArraySection_t *section, const Location &loc) {
        ASRUtils::ASRBuilder builder(al, loc);
        ASR::expr_t *left = get_c_section_left_bound(section, loc);
        ASR::expr_t *right = get_c_section_right_bound(section, loc);
        ASR::expr_t *one = get_c_section_loop_lbound(loc);
        return builder.Add(builder.Sub(right, left), one);
    }

    ASR::expr_t *make_c_section_element_ref(
            ASR::ArraySection_t *section, ASR::expr_t *index_expr,
            const Location &loc) {
        ASRUtils::ASRBuilder builder(al, loc);
        ASR::expr_t *one = get_c_section_loop_lbound(loc);
        ASR::expr_t *base_index = builder.Add(
            get_c_section_left_bound(section, loc), builder.Sub(index_expr, one));
        Vec<ASR::array_index_t> indices;
        indices.reserve(al, section->n_args);
        bool replaced_slice = false;
        for (size_t i = 0; i < section->n_args; i++) {
            ASR::array_index_t idx = section->m_args[i];
            bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
            ASR::array_index_t array_index;
            array_index.loc = loc;
            array_index.m_left = nullptr;
            array_index.m_step = nullptr;
            if (is_slice && !replaced_slice) {
                array_index.m_right = base_index;
                replaced_slice = true;
            } else {
                array_index.m_right = idx.m_right;
            }
            indices.push_back(al, array_index);
        }
        ASR::ttype_t *element_type = ASRUtils::extract_type(section->m_type);
        return ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc,
            section->m_v, indices.p, indices.size(), element_type,
            ASR::arraystorageType::ColMajor, nullptr));
    }

    bool should_leave_large_array_constant_section_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        if (!pass_options.c_backend) {
            return false;
        }
        target = unwrap_array_op_lvalue(target);
        if (target == nullptr) {
            return false;
        }
        ASR::expr_t *base_expr = target;
        if (ASR::is_a<ASR::ArraySection_t>(*target)) {
            ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(target);
            if (section->n_args != 1 || !is_unit_step_expr(section->m_args[0].m_step)) {
                return false;
            }
            base_expr = section->m_v;
        }
        ASR::ttype_t *target_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(target));
        if (target_type == nullptr || !ASRUtils::is_array(target_type)
                || ASRUtils::extract_n_dims_from_ttype(target_type) != 1) {
            return false;
        }
        ASR::ArrayConstant_t *arr = get_array_constant_value(value);
        if (arr == nullptr) {
            return false;
        }
        ASR::ttype_t *arr_type = ASRUtils::type_get_past_allocatable_pointer(arr->m_type);
        if (arr_type == nullptr || !ASRUtils::is_array(arr_type)) {
            return false;
        }
        ASR::ttype_t *element_type = ASRUtils::extract_type(arr_type);
        if (element_type == nullptr || ASRUtils::is_character(*element_type)) {
            return false;
        }
        static const int64_t compact_constant_threshold = 64;
        if (arr->m_n_data < compact_constant_threshold) {
            return false;
        }

        ASR::ttype_t *base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(base_expr));
        if (base_type == nullptr || !ASR::is_a<ASR::Array_t>(*base_type)) {
            return false;
        }
        ASR::array_physical_typeType phys =
            ASR::down_cast<ASR::Array_t>(base_type)->m_physical_type;
        return phys == ASR::array_physical_typeType::DescriptorArray
            || phys == ASR::array_physical_typeType::PointerArray
            || phys == ASR::array_physical_typeType::UnboundedPointerArray
            || phys == ASR::array_physical_typeType::ISODescriptorArray
            || phys == ASR::array_physical_typeType::NumPyArray;
    }

    bool should_leave_small_array_constant_section_assignment_for_c(
            ASR::expr_t *target, ASR::expr_t *value) const {
        if (!pass_options.c_backend) {
            return false;
        }
        target = unwrap_array_op_lvalue(target);
        if (target == nullptr || !ASR::is_a<ASR::ArraySection_t>(*target)) {
            return false;
        }
        ASR::ArraySection_t *section = ASR::down_cast<ASR::ArraySection_t>(target);
        if (section->n_args != 1) {
            return false;
        }
        ASR::array_index_t idx = section->m_args[0];
        bool is_slice = idx.m_left || idx.m_step || idx.m_right == nullptr;
        if (!is_slice
                || !is_simple_integer_section_bound_expr(idx.m_left)
                || !is_simple_integer_section_bound_expr(idx.m_right)
                || !is_simple_integer_section_bound_expr(idx.m_step)) {
            return false;
        }
        ASR::ttype_t *base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(section->m_v));
        ASR::ttype_t *target_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(target));
        if (base_type == nullptr || target_type == nullptr
                || !ASRUtils::is_array(base_type)
                || !ASRUtils::is_array(target_type)
                || ASRUtils::extract_n_dims_from_ttype(base_type) != 1
                || ASRUtils::extract_n_dims_from_ttype(target_type) != 1) {
            return false;
        }
        ASR::ArrayConstant_t *arr = get_array_constant_value(value);
        if (arr == nullptr) {
            return false;
        }
        ASR::ttype_t *value_type = ASRUtils::type_get_past_allocatable_pointer(
            arr->m_type);
        if (value_type == nullptr || !ASRUtils::is_fixed_size_array(value_type)
                || ASRUtils::extract_n_dims_from_ttype(value_type) != 1) {
            return false;
        }
        int64_t length = ASRUtils::get_fixed_size_of_array(value_type);
        ASR::ttype_t *target_element_type = ASRUtils::type_get_past_array(target_type);
        ASR::ttype_t *value_element_type = ASRUtils::type_get_past_array(value_type);
        return length > 0 && length <= 8
            && target_element_type != nullptr
            && value_element_type != nullptr
            && !ASRUtils::is_character(*target_element_type)
            && !ASRUtils::is_character(*value_element_type)
            && (ASRUtils::is_integer(*target_element_type)
                || ASRUtils::is_unsigned_integer(*target_element_type)
                || ASRUtils::is_real(*target_element_type)
                || ASRUtils::is_logical(*target_element_type))
            && ASRUtils::types_equal(
                target_element_type, value_element_type, nullptr, nullptr);
    }

    void call_replacer() {
        replacer.current_expr = current_expr;
        replacer.current_scope = current_scope;
        replacer.replace_expr(*current_expr);
    }

    ArrayOpVisitor(Allocator& al_, const LCompilers::PassOptions& pass_options_):
        al(al_), replacer(al, pass_result, remove_original_stmt),
        parent_body(nullptr), realloc_lhs(pass_options_.realloc_lhs_arrays),
        bounds_checking(pass_options_.bounds_checking),
        remove_original_stmt(false), skip_allocate_source_expansion(false),
        pass_options(pass_options_),
        allocate_source_copy_assignments_to_skip_realloc(0) {
        pass_result.n = 0;
        pass_result.reserve(al, 0);
    }

    void visit_Variable(const ASR::Variable_t& /*x*/) {
        // Do nothing
    }

    void visit_GpuKernelFunction(const ASR::GpuKernelFunction_t& /*x*/) {
        // GPU kernel functions are emitted by the Metal/CUDA codegen
        // which handles array operations directly. The array_op pass
        // must not transform functions inside them (e.g. expanding
        // elemental functions into array loops would produce broken code
        // that references outer-scope array variables from within a
        // scalar-parameter helper function in the GPU shader).
    }

    void visit_FileWrite(const ASR::FileWrite_t& x) {
        /* 
        Handle FileWrite with character-array arguments, where x and a are arrays:
            write(x,"format") (a)
        Expand as following in ASR:
            do i=1,n:
                write(x(i),"format") (a(i))
            end do
        Also handles arraysections on LHS and RHS, eg:
            write(x(i:j),"format") (a(k:m))
        And do-loops in write statements, eg:
            write(x,"format") (a(i),i=k,m)
        TODO: Generalize for N-Dimensional arrays, currently handles only 1-D access
        */
        if (!x.m_unit || !ASRUtils::is_character(*ASRUtils::expr_type(x.m_unit)) ||
            !ASRUtils::is_array(ASRUtils::expr_type(x.m_unit))) return;
        if (x.n_values == 0 || !ASR::is_a<ASR::StringFormat_t>(*x.m_values[0])) return;

        ASR::StringFormat_t* fmt = ASR::down_cast<ASR::StringFormat_t>(x.m_values[0]);
        if (fmt->n_args == 0) return;
        ASR::expr_t* val_arg = fmt->m_args[0];
        bool is_do_loop = ASR::is_a<ASR::ImpliedDoLoop_t>(*val_arg);
        if (!is_do_loop && !ASRUtils::is_array(ASRUtils::expr_type(val_arg))) return;

        // For formatted writes with plain array values whose format contains
        // a slash (/), skip the per-element loop transformation. The slash
        // means "advance to next record" (next array element in internal
        // write). Splitting into per-element writes breaks this because each
        // call only has one value, so the slash is never reached. The LLVM
        // codegen handles this correctly via is_string_array_unit and
        // _lfortran_string_write's newline-based record splitting.
        if (x.m_is_formatted && !is_do_loop && fmt->m_fmt &&
            ASR::is_a<ASR::StringConstant_t>(*fmt->m_fmt)) {
            ASR::StringConstant_t* fmt_str =
                ASR::down_cast<ASR::StringConstant_t>(fmt->m_fmt);
            if (strchr(fmt_str->m_s, '/') != nullptr) return;
        }

        const Location& loc = x.base.base.loc;
        int ikind = get_index_kind();
        ASR::ttype_t* int_type = get_index_type(loc);

        // Extract unit base and bounds
        ASR::expr_t* unit = x.m_unit;
        ASR::expr_t* lb_lhs = PassUtils::get_bound(x.m_unit, 1, "lbound", al, ikind);
        ASR::expr_t* ub_lhs = PassUtils::get_bound(x.m_unit, 1, "ubound", al, ikind);
        // For array-sections, use bounds passed as args
        if (ASR::is_a<ASR::ArraySection_t>(*x.m_unit)) {
            ASR::ArraySection_t* sec = ASR::down_cast<ASR::ArraySection_t>(x.m_unit);
            unit = sec->m_v;
            if (sec->m_args[0].m_left){
                lb_lhs = sec->m_args[0].m_left;
            }
            if (sec->m_args[0].m_right) {
                ub_lhs = sec->m_args[0].m_right;
            }
        }
        
        ASR::expr_t* lb_rhs = nullptr;
        if (is_do_loop){
            lb_rhs = ASR::down_cast<ASR::ImpliedDoLoop_t>(val_arg)->m_start;
        } else {
            lb_rhs = PassUtils::get_bound(val_arg, 1, "lbound", al, ikind);
        }
        // Loop variable: offset = 0 .. ub_lhs - lb_lhs
        Vec<ASR::expr_t*> idx_vars;
        idx_vars.reserve(al, 1);
        PassUtils::create_idx_vars(idx_vars, 1, loc, al, current_scope, "_fw", ikind);
        ASR::expr_t* offset = idx_vars[0];
        ASR::expr_t* zero = make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 0, ikind, loc);
        ASR::expr_t* loop_end = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
            al, loc, ub_lhs, ASR::binopType::Sub, lb_lhs, int_type, nullptr));

        // unit(lb_lhs + offset)
        ASR::expr_t* unit_i = PassUtils::create_array_ref(unit,
            ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, lb_lhs, ASR::binopType::Add, offset, int_type, nullptr)),
            al, current_scope);

        // Replace inline do-loop logic (since they are handled outside)
        ASR::expr_t* rhs_idx = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
            al, loc, lb_rhs, ASR::binopType::Add, offset, int_type, nullptr));
        ASR::expr_t* val_i = nullptr;  
        if (!is_do_loop) {
            val_i = PassUtils::create_array_ref(val_arg, rhs_idx, al, current_scope);
        } else { 
            ASR::ImpliedDoLoop_t* idl = ASR::down_cast<ASR::ImpliedDoLoop_t>(val_arg);
            ASRUtils::ExprStmtDuplicator dup(al);
            ASR::expr_t* body_copy = dup.duplicate_expr(idl->m_values[0]);
            struct ReplaceIDLVar : public ASR::BaseExprReplacer<ReplaceIDLVar> {
                ASR::symbol_t* idl_sym;
                ASR::expr_t* replacement;
                ReplaceIDLVar(ASR::symbol_t* s, ASR::expr_t* r) : idl_sym(s), replacement(r) {}
                void replace_Var(ASR::Var_t* v) {
                    if (v->m_v == idl_sym) *current_expr = replacement;
                }
            };
            ReplaceIDLVar replacer(ASR::down_cast<ASR::Var_t>(idl->m_var)->m_v, rhs_idx);
            replacer.current_expr = &body_copy;
            replacer.replace_expr(body_copy);
            val_i = body_copy;
        }

        // Rewrite StringFormat with scalar val_i, then inner FileWrite
        Vec<ASR::expr_t*> fmt_args; fmt_args.reserve(al, 1);
        fmt_args.push_back(al, val_i);
        ASR::expr_t* fmt_i = ASRUtils::EXPR(ASRUtils::make_StringFormat_t_util(
            al, loc, fmt->m_fmt, fmt_args.p, 1, fmt->m_kind, fmt->m_type, fmt->m_value));
        Vec<ASR::expr_t*> inner_vals; inner_vals.reserve(al, 1);
        inner_vals.push_back(al, fmt_i);
        ASR::stmt_t* inner_write = ASRUtils::STMT(ASR::make_FileWrite_t(
            al, loc, x.m_label, unit_i,
            x.m_iomsg, x.m_iostat, x.m_id,
            inner_vals.p, 1,
            x.m_separator, x.m_end, x.m_overloaded,
            x.m_is_formatted, x.m_nml, x.m_rec, x.m_pos));

        // Wrap Scalar FileWrites in DoLoop
        Vec<ASR::stmt_t*> loop_body; loop_body.reserve(al, 1);
        loop_body.push_back(al, inner_write);
        ASR::do_loop_head_t head;
        head.m_v = offset; head.m_start = zero; head.m_end = loop_end;
        head.m_increment = nullptr; head.loc = loc;
        pass_result.push_back(al, ASRUtils::STMT(ASR::make_DoLoop_t(
            al, loc, nullptr, head, loop_body.p, 1, nullptr, 0)));
        remove_original_stmt = true;
    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        bool remove_original_stmt_copy = remove_original_stmt;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        if( parent_body ) {
            for (size_t j=0; j < pass_result.size(); j++) {
                parent_body->push_back(al, pass_result[j]);
            }
        }
        for (size_t i = 0; i < n_body; i++) {
            pass_result.n = 0;
            pass_result.reserve(al, 1);
            remove_original_stmt = false;
            Vec<ASR::stmt_t*>* parent_body_copy = parent_body;
            parent_body = &body;
            bool skip_allocate_source_expansion_copy =
                skip_allocate_source_expansion;
            if (ASR::is_a<ASR::Allocate_t>(*m_body[i])) {
                skip_allocate_source_expansion =
                    allocate_source_assignment_follows(
                        *ASR::down_cast<ASR::Allocate_t>(m_body[i]),
                        m_body, n_body, i);
            } else {
                skip_allocate_source_expansion = false;
            }
            visit_stmt(*m_body[i]);
            skip_allocate_source_expansion =
                skip_allocate_source_expansion_copy;
            parent_body = parent_body_copy;
            if( pass_result.size() > 0 ) {
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
            } else {
                if( !remove_original_stmt ) {
                    body.push_back(al, m_body[i]);
                    remove_original_stmt = false;
                }
            }
        }
        m_body = body.p;
        n_body = body.size();
        pass_result.n = 0;
        remove_original_stmt = remove_original_stmt_copy;
    }

    bool call_replace_on_expr(ASR::exprType expr_type) {
        switch( expr_type ) {
            case ASR::exprType::ArrayConstant:
            case ASR::exprType::ArrayConstructor: {
                return true;
            }
            default: {
                return false;
            }
        }
    }

    void increment_index_variables(std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
                                   size_t var_with_maxrank, int64_t loop_depth,
                                   Vec<ASR::stmt_t*>& do_loop_body, const Location& loc,
                                   std::unordered_map<ASR::expr_t*, ASR::expr_t*>* index2step = nullptr) {
        ASR::expr_t* default_step = make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, get_index_kind(), loc);
        for( size_t i = 0; i < var2indices.size(); i++ ) {
            if( i == var_with_maxrank ) {
                continue;
            }
            // Skip variables with lower rank than the current loop depth
            if( loop_depth >= static_cast<int64_t>(var2indices[i].n) ) {
                continue;
            }
            ASR::expr_t* index_var = var2indices[i].p[loop_depth];
            if( index_var == nullptr ) {
                continue;
            }
            ASR::expr_t* step = default_step;
            if( index2step != nullptr ) {
                auto it = index2step->find(index_var);
                if( it != index2step->end() ) {
                    step = it->second;
                }
            }
            ASR::expr_t* plus_one = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, index_var,
                ASR::binopType::Add, step, ASRUtils::expr_type(index_var), nullptr));
            ASR::stmt_t* increment = ASRUtils::STMT(ASRUtils::make_Assignment_t_util(
                al, loc, index_var, plus_one, nullptr, false, false));
            do_loop_body.push_back(al, increment);
        }
    }

    void set_index_variables(std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
                             Vec<ASR::expr_t*>& vars_expr, size_t var_with_maxrank,
                             int64_t loop_depth, Vec<ASR::stmt_t*>& dest_vec, const Location& loc) {
        for( size_t i = 0; i < var2indices.size(); i++ ) {
            if( i == var_with_maxrank ) {
                continue;
            }
            // Skip variables with lower rank than the current loop depth
            if( loop_depth >= static_cast<int64_t>(var2indices[i].n) ) {
                continue;
            }
            ASR::expr_t* index_var = var2indices[i].p[loop_depth];
            if( index_var == nullptr ) {
                continue;
            }
            ASR::expr_t* lbound = is_c_rank1_unit_section(vars_expr[i])
                ? get_c_section_loop_lbound(loc)
                : PassUtils::get_bound(vars_expr[i],
                    loop_depth + 1, "lbound", al, get_index_kind());
            ASR::stmt_t* set_index_var = ASRUtils::STMT(ASRUtils::make_Assignment_t_util(
                al, loc, index_var, lbound, nullptr, false, false));
            dest_vec.push_back(al, set_index_var);
        }
    }

    enum IndexType {
        ScalarIndex, ArrayIndex
    };

    void set_index_variables(std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
                             std::unordered_map<ASR::expr_t*, std::pair<ASR::expr_t*, IndexType>>& index2var,
                             size_t var_with_maxrank, int64_t loop_depth,
                             Vec<ASR::stmt_t*>& dest_vec, const Location& loc) {
        for( size_t i = 0; i < var2indices.size(); i++ ) {
            if( i == var_with_maxrank ) {
                continue;
            }
            // Skip variables with lower rank than the current loop depth
            if( loop_depth >= static_cast<int64_t>(var2indices[i].n) ) {
                continue;
            }
            ASR::expr_t* index_var = var2indices[i].p[loop_depth];
            if( index_var == nullptr ) {
                continue;
            }
            size_t bound_dim = loop_depth + 1;
            if( index2var[index_var].second == IndexType::ArrayIndex ) {
                bound_dim = 1;
            }
            ASR::expr_t* lbound;
            if( !ASRUtils::is_array(ASRUtils::expr_type(index2var[index_var].first)) ){
                lbound = index2var[index_var].first;
            } else {
                lbound = PassUtils::get_bound(
                    index2var[index_var].first, bound_dim, "lbound", al, get_index_kind());
            }
            ASR::stmt_t* set_index_var = ASRUtils::STMT(ASRUtils::make_Assignment_t_util(
                al, loc, index_var, lbound, nullptr, false, false));
            dest_vec.push_back(al, set_index_var);
        }
    }

    inline void fill_array_indices_in_vars_expr(
        ASR::expr_t* expr, bool is_expr_array,
        Vec<ASR::expr_t*>& vars_expr,
        size_t& offset_for_array_indices) {
        if( is_expr_array ) {
            ASR::array_index_t* m_args = nullptr; size_t n_args = 0;
            ASRUtils::extract_indices(expr, m_args, n_args);
            for( size_t i = 0; i < n_args; i++ ) {
                if( m_args[i].m_left == nullptr &&
                    m_args[i].m_right != nullptr &&
                    m_args[i].m_step == nullptr ) {
                    if( ASRUtils::is_array(ASRUtils::expr_type(
                            m_args[i].m_right)) ) {
                        vars_expr.push_back(al, m_args[i].m_right);
                    }
                }
            }
            offset_for_array_indices++;
        }
    }

    inline void create_array_item_array_indexed_expr(
            ASR::expr_t* expr, ASR::expr_t** expr_address,
            bool is_expr_array, int var2indices_key,
            size_t var_rank, const Location& loc,
            std::unordered_map<size_t, Vec<ASR::expr_t*>>& var2indices,
            size_t& j) {
        if( is_expr_array ) {
            ASR::array_index_t* m_args = nullptr; size_t n_args = 0;
            Vec<ASR::array_index_t> array_item_args;
            array_item_args.reserve(al, n_args);
            ASRUtils::extract_indices(expr, m_args, n_args);
            ASRUtils::ExprStmtDuplicator expr_duplicator(al);
            Vec<ASR::expr_t*> new_indices; new_indices.reserve(al, n_args);
            int k = 0;
            for( size_t i = 0; i < (n_args == 0 ? var_rank : n_args); i++ ) {
                if( m_args && m_args[i].m_left == nullptr &&
                    m_args[i].m_right != nullptr &&
                    m_args[i].m_step == nullptr ) {
                    if( ASRUtils::is_array(ASRUtils::expr_type(
                            m_args[i].m_right)) ) {
                        ASR::array_index_t array_index;
                        array_index.loc = loc;
                        array_index.m_left = nullptr;
                        Vec<ASR::array_index_t> indices1; indices1.reserve(al, 1);
                        ASR::array_index_t index1; index1.loc = loc; index1.m_left = nullptr;
                        index1.m_right = var2indices[j][0]; index1.m_step = nullptr;
                        new_indices.push_back(al, index1.m_right);
                        indices1.push_back(al, index1);
                        ASR::ttype_t* nested_index_type = ASRUtils::extract_type(
                            ASRUtils::expr_type(m_args[i].m_right));
                        array_index.m_right = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc,
                            m_args[i].m_right, indices1.p, 1, nested_index_type,
                            ASR::arraystorageType::ColMajor, nullptr));
                        array_index.m_step = nullptr;
                        array_item_args.push_back(al, array_index);
                        j++;
                        k++;
                    } else {
                        array_item_args.push_back(al, m_args[i]);
                    }
                } else {
                    ASR::array_index_t index1; index1.loc = loc; index1.m_left = nullptr;
                    index1.m_right = var2indices[var2indices_key][k]; index1.m_step = nullptr;
                    array_item_args.push_back(al, index1);
                    new_indices.push_back(al, var2indices[var2indices_key][k]);
                    k++;
                }
            }
            var2indices[var2indices_key] = new_indices;
            ASR::ttype_t* expr_type = ASRUtils::extract_type(
                    ASRUtils::expr_type(expr));
            *expr_address = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(
                al, loc, ASRUtils::extract_array_variable(expr), array_item_args.p,
                array_item_args.size(), expr_type, ASR::arraystorageType::ColMajor, nullptr));
        }
    }

    template <typename T>
    void generate_loop_for_array_indexed_with_array_indices(const T& x,
        ASR::expr_t** target_address, ASR::expr_t** value_address,
        const Location& loc) {
        ASR::expr_t* target = *target_address;
        ASR::expr_t* value = *value_address;
        size_t var_rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(target));
        Vec<ASR::expr_t*> vars_expr; vars_expr.reserve(al, 2);
        bool is_target_array = ASRUtils::is_array(ASRUtils::expr_type(target));
        bool is_value_array = ASRUtils::is_array(ASRUtils::expr_type(value));
        Vec<ASR::array_index_t> array_indices_args; array_indices_args.reserve(al, 1);
        Vec<ASR::array_index_t> rhs_array_indices_args; rhs_array_indices_args.reserve(al, 1);
        int n_array_indices_args = -1;
        int temp_n = -1;
        size_t do_loop_depth = 0;
        if( is_target_array ) {
            vars_expr.push_back(al, ASRUtils::extract_array_variable(target));
            ASRUtils::extract_array_indices(target, al, array_indices_args, n_array_indices_args);
        }
        if( is_value_array ) {
            vars_expr.push_back(al, ASRUtils::extract_array_variable(value));
            ASRUtils::extract_array_indices(value, al, rhs_array_indices_args, temp_n);
        }

        size_t offset_for_array_indices = 0;

        fill_array_indices_in_vars_expr(
            target, is_target_array,
            vars_expr, offset_for_array_indices);
        fill_array_indices_in_vars_expr(
            value, is_value_array,
            vars_expr, offset_for_array_indices);

        // Common code for target and value
        std::unordered_map<size_t, Vec<ASR::expr_t*>> var2indices;
        std::unordered_map<ASR::expr_t*, std::pair<ASR::expr_t*, IndexType>> index2var;
        std::unordered_map<ASR::expr_t*, ASR::expr_t*> index2step;
        ASR::ttype_t* index_type = get_index_type(loc);
        for( size_t i = 0; i < vars_expr.size(); i++ ) {
            Vec<ASR::expr_t*> indices;
            indices.reserve(al, var_rank);
            for( size_t j = 0; j < (i >= offset_for_array_indices ? 1 : var_rank); j++ ) {
                std::string index_var_name = current_scope->get_unique_name(
                    "__libasr_index_" + std::to_string(j) + "_");
                ASR::symbol_t* index = ASR::down_cast<ASR::symbol_t>(ASRUtils::make_Variable_t_util(
                    al, loc, current_scope, s2c(al, index_var_name), nullptr, 0, ASR::intentType::Local,
                    nullptr, nullptr, ASR::storage_typeType::Default, index_type, nullptr,
                    ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required, false));
                current_scope->add_symbol(index_var_name, index);
                ASR::expr_t* index_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, index));
                if ((i == offset_for_array_indices - 1) && is_value_array && j < rhs_array_indices_args.size() &&
                        rhs_array_indices_args[j].m_left != nullptr) {
                    index2var[index_expr] = std::make_pair(rhs_array_indices_args[j].m_left, IndexType::ScalarIndex);
                    if (rhs_array_indices_args[j].m_step != nullptr) {
                        index2step[index_expr] = rhs_array_indices_args[j].m_step;
                    }
                } else {
                    index2var[index_expr] = std::make_pair(vars_expr[i],
                        i >= offset_for_array_indices ? IndexType::ArrayIndex : IndexType::ScalarIndex);
                }
                indices.push_back(al, index_expr);
            }
            var2indices[i] = indices;
        }

        size_t j = offset_for_array_indices;

        create_array_item_array_indexed_expr(
            target, target_address, is_target_array, 0,
            var_rank, loc, var2indices, j);
        create_array_item_array_indexed_expr(
            value, value_address, is_value_array, 1,
            var_rank, loc, var2indices, j);

        size_t vars_expr_size = vars_expr.size();
        for( size_t i = offset_for_array_indices; i < vars_expr_size; i++ ) {
            var2indices.erase(i);
        }
        vars_expr.n = offset_for_array_indices;

        size_t var_with_maxrank = 0;

        ASR::do_loop_head_t do_loop_head;
        do_loop_head.loc = loc;
        do_loop_head.m_v = var2indices[var_with_maxrank].p[0];
        size_t bound_dim = do_loop_depth + 1;
        if( index2var[do_loop_head.m_v].second == IndexType::ArrayIndex ) {
            bound_dim = 1;
        }
        int64_t array_indices_args_i = 0;
        if( n_array_indices_args > -1 && array_indices_args[array_indices_args_i].m_right != nullptr &&
                array_indices_args[array_indices_args_i].m_left != nullptr &&
                array_indices_args[array_indices_args_i].m_step != nullptr) {
            do_loop_head.m_start = array_indices_args[array_indices_args_i].m_left;
            do_loop_head.m_end = array_indices_args[array_indices_args_i].m_right;
            do_loop_head.m_increment = array_indices_args[array_indices_args_i].m_step;
        } else {
            do_loop_head.m_start = PassUtils::get_bound(
                index2var[do_loop_head.m_v].first, bound_dim, "lbound", al, get_index_kind());
            do_loop_head.m_end = PassUtils::get_bound(
                index2var[do_loop_head.m_v].first, bound_dim, "ubound", al, get_index_kind());
            do_loop_head.m_increment = nullptr;
        }
        Vec<ASR::stmt_t*> parent_do_loop_body; parent_do_loop_body.reserve(al, 1);
        Vec<ASR::stmt_t*> do_loop_body; do_loop_body.reserve(al, 1);
        set_index_variables(var2indices, index2var, var_with_maxrank,
                            0, parent_do_loop_body, loc);
        do_loop_body.push_back(al, const_cast<ASR::stmt_t*>(&(x.base)));
        increment_index_variables(var2indices, var_with_maxrank, 0,
                                  do_loop_body, loc, &index2step);
        ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
            do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
        do_loop_depth += 1;
        array_indices_args_i += 1;
        parent_do_loop_body.push_back(al, do_loop);
        do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
        parent_do_loop_body.reserve(al, 1);

        for( int64_t i = 1; i < static_cast<int64_t>(var_rank); i++ ) {
            set_index_variables(var2indices, index2var, var_with_maxrank,
                                i, parent_do_loop_body, loc);
            increment_index_variables(var2indices, var_with_maxrank, i,
                                      do_loop_body, loc, &index2step);
            ASR::do_loop_head_t do_loop_head;
            do_loop_head.loc = loc;
            do_loop_head.m_v = var2indices[var_with_maxrank].p[i];
            bound_dim = do_loop_depth + 1;
            if( index2var[do_loop_head.m_v].second == IndexType::ArrayIndex ) {
                bound_dim = 1;
            }
            if( n_array_indices_args > -1 && array_indices_args[array_indices_args_i].m_right != nullptr &&
                    array_indices_args[array_indices_args_i].m_left != nullptr &&
                    array_indices_args[array_indices_args_i].m_step != nullptr) {
                do_loop_head.m_start = array_indices_args[array_indices_args_i].m_left;
                do_loop_head.m_end = array_indices_args[array_indices_args_i].m_right;
                do_loop_head.m_increment = array_indices_args[array_indices_args_i].m_step;
            } else {
                do_loop_head.m_start = PassUtils::get_bound(
                    index2var[do_loop_head.m_v].first, bound_dim, "lbound", al, get_index_kind());
                do_loop_head.m_end = PassUtils::get_bound(
                    index2var[do_loop_head.m_v].first, bound_dim, "ubound", al, get_index_kind());
                do_loop_head.m_increment = nullptr;
            }
            ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
                do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
            do_loop_depth++;
            array_indices_args_i += 1;
            parent_do_loop_body.push_back(al, do_loop);
            do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
            parent_do_loop_body.reserve(al, 1);
        }

        for( size_t i = 0; i < do_loop_body.size(); i++ ) {
            pass_result.push_back(al, do_loop_body[i]);
        }
    }

    template <typename T>
    void generate_loop(const T& x, Vec<ASR::expr_t**>& vars,
                       Vec<ASR::expr_t**>& fix_types_args,
                       const Location& loc,
                       const std::vector<ASR::expr_t*>* scalar_targets = nullptr) {
        Vec<size_t> var_ranks;
        Vec<ASR::expr_t*> vars_expr;
        var_ranks.reserve(al, vars.size()); vars_expr.reserve(al, vars.size());
        for( size_t i = 0; i < vars.size(); i++ ) {
            ASR::expr_t* expr = *vars[i];
            ASR::ttype_t* type = ASRUtils::expr_type(expr);
            var_ranks.push_back(al, ASRUtils::extract_n_dims_from_ttype(type));
            vars_expr.push_back(al, expr);
        }

        std::unordered_map<size_t, Vec<ASR::expr_t*>> var2indices;
        ASR::ttype_t* index_type = get_index_type(loc);
        for( size_t i = 0; i < vars.size(); i++ ) {
            Vec<ASR::expr_t*> indices;
            indices.reserve(al, var_ranks[i]);
            for( size_t j = 0; j < var_ranks[i]; j++ ) {
                std::string index_var_name = current_scope->get_unique_name(
                    "__libasr_index_" + std::to_string(j) + "_");
                ASR::symbol_t* index = ASR::down_cast<ASR::symbol_t>(ASRUtils::make_Variable_t_util(
                    al, loc, current_scope, s2c(al, index_var_name), nullptr, 0, ASR::intentType::Local,
                    nullptr, nullptr, ASR::storage_typeType::Default, index_type, nullptr,
                    ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required, false));
                current_scope->add_symbol(index_var_name, index);
                ASR::expr_t* index_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, index));
                indices.push_back(al, index_expr);
            }
            var2indices[i] = indices;
        }

        for( size_t i = 0; i < vars.size(); i++ ) {
            if (var_ranks[i] == 0) continue;
            Vec<ASR::array_index_t> indices;
            indices.reserve(al, var_ranks[i]);
            for( size_t j = 0; j < var_ranks[i]; j++ ) {
                ASR::array_index_t array_index;
                array_index.loc = loc;
                array_index.m_left = nullptr;
                array_index.m_right = get_singleton_safe_index(*vars[i],
                    var2indices[i][j], j + 1, loc, scalar_targets);
                array_index.m_step = nullptr;
                indices.push_back(al, array_index);
            }
            ASR::ttype_t* var_i_type = ASRUtils::extract_type(
                ASRUtils::expr_type(*vars[i]));
            if (is_c_rank1_unit_section(*vars[i])) {
                *vars[i] = make_c_section_element_ref(
                    ASR::down_cast<ASR::ArraySection_t>(*vars[i]),
                    indices[0].m_right, loc);
            } else {
                *vars[i] = ASRUtils::EXPR(ASRUtils::make_ArrayItem_t_util(al, loc, *vars[i], indices.p,
                    indices.size(), var_i_type, ASR::arraystorageType::ColMajor, nullptr));
            }
        }

        ASRUtils::RemoveArrayProcessingNodeVisitor array_broadcast_visitor(al);
        for( size_t i = 0; i < fix_types_args.size(); i++ ) {
            array_broadcast_visitor.current_expr = fix_types_args[i];
            array_broadcast_visitor.call_replacer();
        }

        CleanupDegenerateArraySection cleanup(al);
        for( size_t i = 0; i < fix_types_args.size(); i++ ) {
            cleanup.current_expr = fix_types_args[i];
            cleanup.replace_expr(*fix_types_args[i]);
        }

        FixTypeVisitor fix_types(al);
        fix_types.current_scope = current_scope;
        for( size_t i = 0; i < fix_types_args.size(); i++ ) {
            fix_types.visit_expr(*(*fix_types_args[i]));
        }

        size_t var_with_maxrank = 0;
        for( size_t i = 0; i < var_ranks.size(); i++ ) {
            if( var_ranks[i] > var_ranks[var_with_maxrank] ) {
                var_with_maxrank = i;
            }
        }

        ASR::do_loop_head_t do_loop_head;
        do_loop_head.loc = loc;
        do_loop_head.m_v = var2indices[var_with_maxrank].p[0];
        if (is_c_rank1_unit_section(vars_expr[var_with_maxrank])) {
            ASR::ArraySection_t *section =
                ASR::down_cast<ASR::ArraySection_t>(vars_expr[var_with_maxrank]);
            do_loop_head.m_start = get_c_section_loop_lbound(loc);
            do_loop_head.m_end = get_c_section_loop_ubound(section, loc);
        } else {
            do_loop_head.m_start = PassUtils::get_bound(vars_expr[var_with_maxrank],
                1, "lbound", al, get_index_kind());
            do_loop_head.m_end = PassUtils::get_bound(vars_expr[var_with_maxrank],
                1, "ubound", al, get_index_kind());
        }
        do_loop_head.m_increment = nullptr;
        Vec<ASR::stmt_t*> parent_do_loop_body; parent_do_loop_body.reserve(al, 1);
        Vec<ASR::stmt_t*> do_loop_body; do_loop_body.reserve(al, 1);
        set_index_variables(var2indices, vars_expr, var_with_maxrank,
                            0, parent_do_loop_body, loc);
        do_loop_body.push_back(al, const_cast<ASR::stmt_t*>(&(x.base)));
        increment_index_variables(var2indices, var_with_maxrank, 0,
                                  do_loop_body, loc);
        ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
            do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
        parent_do_loop_body.push_back(al, do_loop);
        do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
        parent_do_loop_body.reserve(al, 1);

        for( int64_t i = 1; i < static_cast<int64_t>(var_ranks[var_with_maxrank]); i++ ) {
            set_index_variables(var2indices, vars_expr, var_with_maxrank,
                                i, parent_do_loop_body, loc);
            increment_index_variables(var2indices, var_with_maxrank, i,
                                      do_loop_body, loc);
            ASR::do_loop_head_t do_loop_head;
            do_loop_head.loc = loc;
            do_loop_head.m_v = var2indices[var_with_maxrank].p[i];
            do_loop_head.m_start = PassUtils::get_bound(
                vars_expr[var_with_maxrank], i + 1, "lbound", al, get_index_kind());
            do_loop_head.m_end = PassUtils::get_bound(
                vars_expr[var_with_maxrank], i + 1, "ubound", al, get_index_kind());
            do_loop_head.m_increment = nullptr;
            ASR::stmt_t* do_loop = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
                do_loop_head, do_loop_body.p, do_loop_body.size(), nullptr, 0));
            parent_do_loop_body.push_back(al, do_loop);
            do_loop_body.from_pointer_n_copy(al, parent_do_loop_body.p, parent_do_loop_body.size());
            parent_do_loop_body.reserve(al, 1);
        }

        for( size_t i = 0; i < do_loop_body.size(); i++ ) {
            pass_result.push_back(al, do_loop_body[i]);
        }
    }

    ASR::expr_t* compute_indexed_dim_size(const ASR::array_index_t& idx,
            const Location& loc, ASRUtils::ASRBuilder& builder,
            ASR::ttype_t* idx_type) {
        if (idx.m_left == nullptr && idx.m_step == nullptr &&
                idx.m_right != nullptr &&
                ASRUtils::is_array(ASRUtils::expr_type(idx.m_right))) {
            ASR::expr_t* dim_one = builder.i_t(1, idx_type);
            return ASRUtils::EXPR(ASR::make_ArraySize_t(al, loc,
                idx.m_right, dim_one, idx_type, nullptr));
        }
        if (idx.m_left != nullptr && idx.m_right != nullptr) {
            ASRUtils::ExprStmtDuplicator d(al);
            ASR::expr_t* lo = d.duplicate_expr(idx.m_left);
            ASR::expr_t* hi = d.duplicate_expr(idx.m_right);
            ASR::expr_t* step = idx.m_step
                ? d.duplicate_expr(idx.m_step)
                : builder.i_t(1, idx_type);
            ASR::expr_t* diff = builder.Sub(hi, lo);
            ASR::expr_t* num = builder.Add(builder.Div(diff, step),
                builder.i_t(1, idx_type));
            return num;
        }
        return nullptr;
    }

    bool insert_realloc_for_indexed_value(ASR::expr_t* target,
            ASR::expr_t* value, const Location& loc) {
        ASR::array_index_t* args = nullptr;
        size_t n_args = 0;
        if (ASR::is_a<ASR::ArrayItem_t>(*value)) {
            ASR::ArrayItem_t* ai = ASR::down_cast<ASR::ArrayItem_t>(value);
            args = ai->m_args; n_args = ai->n_args;
        } else if (ASR::is_a<ASR::ArraySection_t>(*value)) {
            ASR::ArraySection_t* as = ASR::down_cast<ASR::ArraySection_t>(value);
            args = as->m_args; n_args = as->n_args;
        } else {
            return false;
        }
        ASRUtils::ASRBuilder builder(al, loc);
        ASR::ttype_t* idx_type = get_index_type(loc);
        Vec<ASR::dimension_t> realloc_dims;
        realloc_dims.reserve(al, n_args);
        for (size_t i = 0; i < n_args; i++) {
            const ASR::array_index_t& idx = args[i];
            bool is_scalar_index = (idx.m_left == nullptr &&
                idx.m_step == nullptr && idx.m_right != nullptr &&
                !ASRUtils::is_array(ASRUtils::expr_type(idx.m_right)));
            if (is_scalar_index) continue;
            ASR::expr_t* dim_size = compute_indexed_dim_size(idx, loc,
                builder, idx_type);
            if (dim_size == nullptr) return false;
            ASR::dimension_t d;
            d.loc = loc;
            d.m_start = builder.i_t(1, idx_type);
            d.m_length = dim_size;
            realloc_dims.push_back(al, d);
        }
        size_t target_rank = ASRUtils::extract_n_dims_from_ttype(
            ASRUtils::expr_type(target));
        if (realloc_dims.size() != target_rank) return false;

        ASR::expr_t* realloc_str_len = nullptr;
        ASR::ttype_t* target_type = ASRUtils::expr_type(target);
        if (ASRUtils::is_character(*target_type)) {
            ASR::String_t* target_str = ASR::down_cast<ASR::String_t>(
                ASRUtils::extract_type(target_type));
            ASRUtils::ExprStmtDuplicator d(al);
            if (target_str->m_len_kind != ASR::string_length_kindType::DeferredLength
                    && target_str->m_len != nullptr) {
                int64_t target_len {};
                if (ASRUtils::is_value_constant(target_str->m_len, target_len)) {
                    realloc_str_len = builder.i32(target_len);
                } else {
                    realloc_str_len = d.duplicate_expr(target_str->m_len);
                }
            } else {
                ASR::expr_t* len_value = nullptr;
                int64_t len {};
                ASR::String_t* val_str = ASR::down_cast<ASR::String_t>(
                    ASRUtils::extract_type(ASRUtils::expr_type(value)));
                if (val_str->m_len != nullptr &&
                        ASRUtils::is_value_constant(val_str->m_len, len)) {
                    len_value = builder.i32(len);
                }
                ASR::expr_t* len_src = nullptr;
                if (ASR::is_a<ASR::ArrayItem_t>(*value)) {
                    len_src = ASR::down_cast<ASR::ArrayItem_t>(value)->m_v;
                } else if (ASR::is_a<ASR::ArraySection_t>(*value)) {
                    len_src = ASR::down_cast<ASR::ArraySection_t>(value)->m_v;
                }
                if (len_src == nullptr) {
                    return false;
                }
                realloc_str_len = ASRUtils::EXPR(ASR::make_StringLen_t(
                    al, loc, d.duplicate_expr(len_src), int32, len_value));
            }
        }

        Vec<ASR::alloc_arg_t> alloc_args;
        alloc_args.reserve(al, 1);
        ASR::alloc_arg_t aa;
        aa.loc = loc;
        aa.m_a = target;
        aa.m_dims = realloc_dims.p;
        aa.n_dims = realloc_dims.size();
        aa.m_len_expr = realloc_str_len;
        aa.m_type = nullptr;
        aa.m_sym_subclass = nullptr;
        alloc_args.push_back(al, aa);
        pass_result.push_back(al, ASRUtils::STMT(ASR::make_ReAlloc_t(al,
            loc, alloc_args.p, alloc_args.size())));
        return true;
    }

    void insert_realloc_for_target(ASR::expr_t* target, ASR::expr_t* value, Vec<ASR::expr_t**>& vars, bool per_assign_realloc = false) {
        ASR::expr_t *realloc_target = ASRUtils::get_past_array_physical_cast(target);
        if (ASR::is_a<ASR::ArraySection_t>(*realloc_target)
                || ASR::is_a<ASR::ArrayItem_t>(*realloc_target)) {
            return;
        }
        ASR::ttype_t* target_type = ASRUtils::expr_type(target);
        bool target_is_allocatable = ASRUtils::is_allocatable(target_type);
        if (!target_is_allocatable) {
            return;
        }
        bool array_copy = ASR::is_a<ASR::Var_t>(*value) && ASR::is_a<ASR::Var_t>(*target);
        if (!realloc_lhs && !per_assign_realloc) {
            return;
        }
        if( target_is_allocatable && !per_assign_realloc && !(realloc_lhs || array_copy) ) {
            return ;
        }

        ASRUtils::ExprStmtDuplicator d(al);
        ASR::expr_t* size_expr = ASRUtils::get_expr_size_expr(value);
        if (size_expr == nullptr) {
            return;
        }
        ASR::expr_t* realloc_var = d.duplicate_expr(size_expr);

        Location loc; loc.first = 1, loc.last = 1;
        ASRUtils::ASRBuilder builder(al, loc);
        ASR::ttype_t* idx_type = get_index_type(loc);
        int idx_kind = get_index_kind();
        // When assigning back from function result, the LHS
        // allocatable variable must have lower bounds reset to 1, 
        // It should not inherit the bounds set within the function from target. 
        // Note: Instead of string matching, a more robust approach can be using 
        // m_move_allocation flags, but it requires parsing entire ASR to 
        // extract those flags, which might increase time for this rare edge case.
        // So, using string matching patterns for now.
        bool from_function_result = false;
        if (ASR::is_a<ASR::Var_t>(*value)) {
            ASR::symbol_t* sym = ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Var_t>(value)->m_v);
            if (ASR::is_a<ASR::Variable_t>(*sym)) {
                // Temporary allocatable array for return type 
                // Created inside pass: subroutine_from_function
                std::string src_name = std::string(ASR::down_cast<ASR::Variable_t>(sym)->m_name);
                from_function_result = src_name.find("__libasr_created__function_call_") == 0;
            }
        }
        Vec<ASR::dimension_t> realloc_dims;
        size_t target_rank = ASRUtils::extract_n_dims_from_ttype(target_type);
        realloc_dims.reserve(al, target_rank);
        for( size_t i = 0; i < target_rank; i++ ) {
            ASR::dimension_t realloc_dim;
            realloc_dim.loc = loc;
            if (from_function_result) {
                // Reset lower bound to 1, for function return assignment
                realloc_dim.m_start = make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, idx_kind, loc);
            } else {
                // For other cases (For example, variable copy a = b) preserve source lbounds
                realloc_dim.m_start = PassUtils::get_bound(realloc_var, i + 1, "lbound", al, idx_kind);
            }
            ASR::expr_t* dim_expr = make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, i + 1, idx_kind, loc);
            realloc_dim.m_length = ASRUtils::EXPR(ASR::make_ArraySize_t(
                al, loc, realloc_var, dim_expr, idx_type, nullptr));
            realloc_dims.push_back(al, realloc_dim);
        }
        ASR::expr_t* realloc_str_len {};
        if(ASRUtils::is_character(*ASRUtils::expr_type(realloc_var))){
            bool use_target_len = false;
            if (ASRUtils::is_character(*target_type)) {
                ASR::String_t* target_str = ASR::down_cast<ASR::String_t>(
                    ASRUtils::extract_type(target_type));
                if (target_str->m_len_kind != ASR::string_length_kindType::DeferredLength
                        && target_str->m_len != nullptr) {
                    int64_t target_len {};
                    if (ASRUtils::is_value_constant(target_str->m_len, target_len)) {
                        realloc_str_len = builder.i32(target_len);
                    } else {
                        ASRUtils::ExprStmtDuplicator d2(al);
                        realloc_str_len = d2.duplicate_expr(target_str->m_len);
                    }
                    use_target_len = true;
                }
            }
            if (!use_target_len) {
                if (ASR::is_a<ASR::IntrinsicElementalFunction_t>(*value) &&
                    ASR::down_cast<ASR::IntrinsicElementalFunction_t>(value)->m_intrinsic_id ==
                        static_cast<int64_t>(ASRUtils::IntrinsicElementalFunctions::StringConcat)) {
                    realloc_str_len = ASRUtils::StringConcat::get_safe_string_len(
                        al, loc, value, ASRUtils::expr_type(value), builder);
                } else {
                    ASR::expr_t* len_value{}; // Compile-Time Length
                    int64_t len {};
                    if(ASRUtils::is_value_constant(ASR::down_cast<ASR::String_t>(
                        ASRUtils::extract_type(ASRUtils::expr_type(realloc_var)))->m_len), len) {
                        len_value = builder.i32(len);
                    }
                    realloc_str_len = ASRUtils::EXPR(ASR::make_StringLen_t(
                        al, loc, realloc_var, int32, len_value));
                }
            }
        }

        Vec<ASR::alloc_arg_t> alloc_args; alloc_args.reserve(al, 1);
        ASR::alloc_arg_t alloc_arg;
        alloc_arg.loc = loc;
        alloc_arg.m_a = target;
        alloc_arg.m_dims = realloc_dims.p;
        alloc_arg.n_dims = realloc_dims.size();
        alloc_arg.m_len_expr = realloc_str_len;
        alloc_arg.m_type = nullptr;
        alloc_arg.m_sym_subclass = nullptr;
        alloc_args.push_back(al, alloc_arg);

        pass_result.push_back(al, ASRUtils::STMT(ASR::make_ReAlloc_t(
                al, loc, alloc_args.p, alloc_args.size())));
    }

    ASR::Variable_t* get_base_variable(ASR::expr_t* expr) {
        ASR::expr_t* cur = expr;
        while (cur) {
            if (ASR::is_a<ASR::Var_t>(*cur)) {
                ASR::symbol_t* sym = ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::Var_t>(cur)->m_v);
                if (ASR::is_a<ASR::Variable_t>(*sym)) {
                    return ASR::down_cast<ASR::Variable_t>(sym);
                }
                return nullptr;
            } else if (ASR::is_a<ASR::StructInstanceMember_t>(*cur)) {
                cur = ASR::down_cast<ASR::StructInstanceMember_t>(cur)->m_v;
            } else if (ASR::is_a<ASR::ArrayItem_t>(*cur)) {
                cur = ASR::down_cast<ASR::ArrayItem_t>(cur)->m_v;
            } else if (ASR::is_a<ASR::ArraySection_t>(*cur)) {
                cur = ASR::down_cast<ASR::ArraySection_t>(cur)->m_v;
            } else if (ASR::is_a<ASR::ArrayPhysicalCast_t>(*cur)) {
                cur = ASR::down_cast<ASR::ArrayPhysicalCast_t>(cur)->m_arg;
            } else if (ASR::is_a<ASR::Cast_t>(*cur)) {
                cur = ASR::down_cast<ASR::Cast_t>(cur)->m_arg;
            } else {
                return nullptr;
            }
        }
        return nullptr;
    }

    bool should_auto_realloc_component_assignment(ASR::expr_t* target) {
        if (!ASR::is_a<ASR::StructInstanceMember_t>(*target)) {
            return false;
        }
        ASR::ttype_t* target_type = ASRUtils::expr_type(target);
        if (!ASRUtils::is_array(target_type) || !ASRUtils::is_allocatable(target_type)) {
            return false;
        }
        ASR::Variable_t* base_var = get_base_variable(target);
        if (!base_var) {
            return false;
        }
        return base_var->m_intent == ASR::intentType::Out ||
            base_var->m_intent == ASR::intentType::ReturnVar;
    }

    void collect_array_if_scalar_targets(ASR::stmt_t** body, size_t n_body,
            std::vector<ASR::expr_t*>& scalar_targets) {
        for (size_t i = 0; i < n_body; i++) {
            ASR::stmt_t* stmt = body[i];
            if (ASR::is_a<ASR::Assignment_t>(*stmt)) {
                ASR::Assignment_t& assignment = *ASR::down_cast<ASR::Assignment_t>(stmt);
                ASR::expr_t* value = ASRUtils::get_past_array_broadcast(assignment.m_value);
                if (assignment.m_move_allocation ||
                    !ASRUtils::is_array(ASRUtils::expr_type(assignment.m_target)) ||
                    ASRUtils::is_array(ASRUtils::expr_type(value))) {
                    continue;
                }

                if (std::find(scalar_targets.begin(), scalar_targets.end(),
                        assignment.m_target) != scalar_targets.end()) {
                    continue;
                }
                scalar_targets.push_back(assignment.m_target);
            } else if (ASR::is_a<ASR::If_t>(*stmt)) {
                ASR::If_t& if_stmt = *ASR::down_cast<ASR::If_t>(stmt);
                collect_array_if_scalar_targets(if_stmt.m_body,
                    if_stmt.n_body, scalar_targets);
                collect_array_if_scalar_targets(if_stmt.m_orelse,
                    if_stmt.n_orelse, scalar_targets);
            }
        }
    }

    ASR::expr_t* get_singleton_safe_index(ASR::expr_t* expr, ASR::expr_t* index_expr,
            size_t dim, const Location& loc,
            const std::vector<ASR::expr_t*>* scalar_targets) {
        if (!scalar_targets ||
                std::find(scalar_targets->begin(), scalar_targets->end(), expr) ==
                    scalar_targets->end()) {
            return index_expr;
        }

        ASRUtils::ASRBuilder builder(al, loc);
        ASR::ttype_t* idx_type = get_index_type(loc);
        int idx_kind = get_index_kind();
        ASR::expr_t* dim_expr = make_ConstantWithKind(make_IntegerConstant_t,
            make_Integer_t, dim, idx_kind, loc);
        ASR::expr_t* one = make_ConstantWithKind(make_IntegerConstant_t,
            make_Integer_t, 1, idx_kind, loc);
        ASR::expr_t* target_extent = ASRUtils::EXPR(ASR::make_ArraySize_t(
            al, loc, expr, dim_expr, idx_type, nullptr));
        ASR::expr_t* is_singleton_extent = builder.Eq(target_extent, one);
        ASR::expr_t* lower_bound = PassUtils::get_bound(expr, dim,
            "lbound", al, idx_kind);
        return ASRUtils::EXPR(ASR::make_IfExp_t(al, loc, is_singleton_extent,
            lower_bound, index_expr, idx_type, nullptr));
    }

    void generate_assumed_rank_source_loop(ASR::Allocate_t& xx, size_t arg_i,
            const Location& loc) {
        size_t n_dims = xx.m_args[arg_i].n_dims;
        ASR::expr_t* target_var = xx.m_args[arg_i].m_a;
        ASR::ttype_t* idx_type = get_index_type(loc);
        int idx_kind = get_index_kind();

        ASR::ttype_t* base_type = ASRUtils::type_get_past_allocatable_pointer(
            ASRUtils::expr_type(target_var));
        ASR::ttype_t* descriptor_array_type =
            ASRUtils::create_array_type_with_empty_dims(al, n_dims,
                ASRUtils::extract_type(base_type));
        ASR::expr_t* casted_target = ASRUtils::EXPR(
            ASRUtils::make_ArrayPhysicalCast_t_util(al, loc, target_var,
                ASR::array_physical_typeType::AssumedRankArray,
                ASR::array_physical_typeType::DescriptorArray,
                descriptor_array_type, nullptr));

        Vec<ASR::expr_t*> index_exprs;
        index_exprs.reserve(al, n_dims);
        for (size_t j = 0; j < n_dims; j++) {
            std::string idx_name = current_scope->get_unique_name(
                "__libasr_index_" + std::to_string(j) + "_");
            ASR::symbol_t* idx_sym = ASR::down_cast<ASR::symbol_t>(
                ASRUtils::make_Variable_t_util(al, loc, current_scope,
                    s2c(al, idx_name), nullptr, 0, ASR::intentType::Local,
                    nullptr, nullptr, ASR::storage_typeType::Default, idx_type,
                    nullptr, ASR::abiType::Source, ASR::accessType::Public,
                    ASR::presenceType::Required, false));
            current_scope->add_symbol(idx_name, idx_sym);
            index_exprs.push_back(al, ASRUtils::EXPR(
                ASR::make_Var_t(al, loc, idx_sym)));
        }

        Vec<ASR::array_index_t> array_indices;
        array_indices.reserve(al, n_dims);
        for (size_t j = 0; j < n_dims; j++) {
            ASR::array_index_t ai;
            ai.loc = loc;
            ai.m_left = nullptr;
            ai.m_right = index_exprs[j];
            ai.m_step = nullptr;
            array_indices.push_back(al, ai);
        }
        ASR::ttype_t* elem_type = ASRUtils::extract_type(
            ASRUtils::expr_type(target_var));
        ASR::expr_t* indexed_target = ASRUtils::EXPR(
            ASRUtils::make_ArrayItem_t_util(al, loc, casted_target,
                array_indices.p, array_indices.size(), elem_type,
                ASR::arraystorageType::ColMajor, nullptr));

        ASRUtils::ExprStmtDuplicator dup(al);
        ASR::expr_t* source_copy = dup.duplicate_expr(xx.m_source);

        ASR::stmt_t* curr_stmt = ASRUtils::STMT(
            ASRUtils::make_Assignment_t_util(al, loc, indexed_target,
                source_copy, nullptr, realloc_lhs, false));

        for (size_t k = 0; k < n_dims; k++) {
            size_t dim = n_dims - 1 - k;
            ASRUtils::ExprStmtDuplicator bound_dup(al);
            ASR::expr_t* lb_arr = bound_dup.duplicate_expr(casted_target);
            ASR::expr_t* ub_arr = bound_dup.duplicate_expr(casted_target);
            ASR::do_loop_head_t loop_head;
            loop_head.loc = loc;
            loop_head.m_v = index_exprs[dim];
            loop_head.m_start = PassUtils::get_bound(lb_arr,
                dim + 1, "lbound", al, idx_kind);
            loop_head.m_end = PassUtils::get_bound(ub_arr,
                dim + 1, "ubound", al, idx_kind);
            loop_head.m_increment = nullptr;
            Vec<ASR::stmt_t*> body;
            body.reserve(al, 1);
            body.push_back(al, curr_stmt);
            curr_stmt = ASRUtils::STMT(ASR::make_DoLoop_t(al, loc, nullptr,
                loop_head, body.p, body.size(), nullptr, 0));
        }
        pass_result.push_back(al, curr_stmt);
    }

    void visit_Allocate(const ASR::Allocate_t& x) {
        ASR::Allocate_t& xx = const_cast<ASR::Allocate_t&>(x);
        if (xx.m_source) {
            pass_result.push_back(al, &(xx.base));
            if (skip_allocate_source_expansion) {
                xx.m_source = nullptr;
                return;
            }
            bool generated_loop = false;
            // Pushing assignment statements to source
            for (size_t i = 0; i < x.n_args ; i++) {
                if( !ASRUtils::is_array(
                        ASRUtils::expr_type(x.m_args[i].m_a)) ) {
                    continue;
                }
                // Skip mold-based allocations: m_type being set indicates
                // the allocate uses mold= (type/shape only, no data copy).
                if (x.m_args[i].m_type != nullptr) {
                    continue;
                }
                // Assumed-rank target: generate_loop can't read the rank from
                // the variable's static type (which has 0 dims). The runtime
                // rank is given by the allocate dims, so build the loop nest
                // explicitly using m_args[i].n_dims.
                ASR::ttype_t* tgt_t = ASRUtils::type_get_past_allocatable_pointer(
                    ASRUtils::expr_type(x.m_args[i].m_a));
                if (ASRUtils::is_assumed_rank_array(tgt_t) &&
                        x.m_args[i].n_dims > 0) {
                    generate_assumed_rank_source_loop(xx, i, x.base.base.loc);
                    generated_loop = true;
                    continue;
                }
                ASRUtils::ExprStmtDuplicator duplicator(al);
                ASR::expr_t* source_copy = duplicator.duplicate_expr(xx.m_source);
                ASR::stmt_t* assign = ASRUtils::STMT(ASRUtils::make_Assignment_t_util(
                    al, xx.m_args[i].m_a->base.loc, xx.m_args[i].m_a, source_copy, nullptr, realloc_lhs, false));
                ASR::Assignment_t* assignment_t = ASR::down_cast<ASR::Assignment_t>(assign);
                Vec<ASR::expr_t**> fix_type_args;
                fix_type_args.reserve(al, 2);
                fix_type_args.push_back(al, const_cast<ASR::expr_t**>(&(assignment_t->m_target)));
                fix_type_args.push_back(al, const_cast<ASR::expr_t**>(&(assignment_t->m_value)));

                Vec<ASR::expr_t**> vars1;
                vars1.reserve(al, 1);
                ArrayVarAddressCollector var_collector_target(al, vars1);
                var_collector_target.current_expr = const_cast<ASR::expr_t**>(&(assignment_t->m_target));
                var_collector_target.call_replacer();
                ArrayVarAddressCollector var_collector_value(al, vars1);
                var_collector_value.current_expr = const_cast<ASR::expr_t**>(&(assignment_t->m_value));
                var_collector_value.call_replacer();

                // TODO: Remove the following. Instead directly handle
                // allocate with source in the backend.
                if( xx.m_args[i].n_dims == 0 ) {
                    Vec<ASR::expr_t**> vars2;
                    vars2.reserve(al, 1);
                    ArrayVarAddressCollector var_collector2_target(al, vars2);
                    var_collector2_target.current_expr = const_cast<ASR::expr_t**>(&(assignment_t->m_target));
                    var_collector2_target.call_replacer();
                    ArrayVarAddressCollector var_collector2_value(al, vars2);
                    var_collector2_value.current_expr = const_cast<ASR::expr_t**>(&(assignment_t->m_value));
                    var_collector2_value.call_replacer();
                    insert_realloc_for_target(
                        xx.m_args[i].m_a, xx.m_source, vars2);
                }
                generate_loop(*assignment_t, vars1, fix_type_args, x.base.base.loc);
                generated_loop = true;
            }
            // Clear source when data copy loops were generated, since
            // the loops handle the copy. Keeping array-typed source
            // expressions (e.g. real(a)) would cause backend errors.
            if (generated_loop) {
                xx.m_source = nullptr;
            }
        }
    }

    // Don't visit inside DebugCheckArrayBounds, it may contain ArrayConstant and result_expr will be nullptr
    void visit_DebugCheckArrayBounds(const ASR::DebugCheckArrayBounds_t& x) {
        (void)x;
    }

    bool is_looping_necessary_for_bitcast(ASR::expr_t* value) {
        if (ASR::is_a<ASR::BitCast_t>(*value)) {
            ASR::BitCast_t* bit_cast = ASR::down_cast<ASR::BitCast_t>(value);
            return !ASRUtils::is_string_only(ASRUtils::expr_type(bit_cast->m_source));
        } else {
            return false;
        }
    }


    void visit_Assignment(const ASR::Assignment_t& x) {
        if (ASRUtils::is_simd_array(x.m_target)) {
            if( !(ASRUtils::is_allocatable(x.m_value) ||
                  ASRUtils::is_pointer(ASRUtils::expr_type(x.m_value))) ) {
                return ;
            }
        }
        ASR::Assignment_t& xx = const_cast<ASR::Assignment_t&>(x);
        bool skip_allocate_source_copy_realloc = false;
        if (allocate_source_copy_assignments_to_skip_realloc > 0) {
            skip_allocate_source_copy_realloc = true;
            allocate_source_copy_assignments_to_skip_realloc--;
            xx.m_realloc_lhs = false;
        }
        const std::vector<ASR::exprType>& skip_exprs = {
            ASR::exprType::IntrinsicArrayFunction,
            ASR::exprType::ArrayReshape,
        };
        if ( ASR::is_a<ASR::IntrinsicArrayFunction_t>(*xx.m_value) ) {
            // We need to do this because, we may have an assignment
            // in which IntrinsicArrayFunction is evaluated already and
            // value is an ArrayConstant, thus we need to unroll it.
            ASR::IntrinsicArrayFunction_t* iaf = ASR::down_cast<ASR::IntrinsicArrayFunction_t>(xx.m_value);
            if ( iaf->m_value != nullptr ) {
                xx.m_value = iaf->m_value;
            }
        }
        if ( ASR::is_a<ASR::ArrayReshape_t>(*xx.m_value) ) {
            ASR::ArrayReshape_t* ar = ASR::down_cast<ASR::ArrayReshape_t>(xx.m_value);
            if ( ar->m_value != nullptr ) {
                xx.m_value = ar->m_value;
            }
        }
        if (is_unlimited_polymorphic_array_type(ASRUtils::expr_type(xx.m_target))) {
            return;
        }
        if( !ASRUtils::is_array(ASRUtils::expr_type(xx.m_target)) ||
            std::find(skip_exprs.begin(), skip_exprs.end(), xx.m_value->type) != skip_exprs.end() ||
            (ASRUtils::is_simd_array(xx.m_target) && ASRUtils::is_simd_array(xx.m_value)) ) {
            return ;
        }
        bool is_target_assumed_rank = (ASR::is_a<ASR::ArrayPhysicalCast_t>(*xx.m_target) && 
            ASR::down_cast<ASR::ArrayPhysicalCast_t>(xx.m_target)->m_old == ASR::array_physical_typeType::AssumedRankArray) 
            || ASRUtils::is_assumed_rank_array(ASRUtils::expr_type(xx.m_target));
        bool is_value_assumed_rank = (ASR::is_a<ASR::ArrayPhysicalCast_t>(*xx.m_value) && 
            ASR::down_cast<ASR::ArrayPhysicalCast_t>(xx.m_value)->m_old == ASR::array_physical_typeType::AssumedRankArray)
            || ASRUtils::is_assumed_rank_array(ASRUtils::expr_type(xx.m_value));
        xx.m_value = ASRUtils::get_past_array_broadcast(xx.m_value);
        const Location loc = x.base.base.loc;
        if (should_leave_large_array_constant_section_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_small_array_constant_section_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_rank2_full_self_update_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_allocatable_self_section_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_rank2_allocatable_matmul_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_reallocated_temp_plain_data_copy_assignment_for_c(
                xx.m_target, xx.m_value)) {
            bool target_is_compiler_alloc_temp =
                is_compiler_created_array_temp_expr(xx.m_target)
                && ASRUtils::is_allocatable(ASRUtils::expr_type(xx.m_target));
            bool per_assign_realloc = !skip_allocate_source_copy_realloc
                && (xx.m_realloc_lhs ||
                    (!is_compiler_created_array_temp_expr(xx.m_target)
                        && should_auto_realloc_component_assignment(xx.m_target))
                    || (target_is_compiler_alloc_temp
                        && !previous_stmt_allocates_target(xx.m_target)));
            if (per_assign_realloc) {
                Vec<ASR::expr_t**> realloc_vars;
                realloc_vars.reserve(al, 1);
                realloc_vars.push_back(al, const_cast<ASR::expr_t**>(&(xx.m_value)));
                insert_realloc_for_target(
                    xx.m_target, xx.m_value, realloc_vars, per_assign_realloc);
            }
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, false,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_rank2_plain_data_copy_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_rank2_scalarizable_array_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_rank2_section_scalar_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_rank1_matmul_self_update_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }
        if (should_leave_scalarizable_array_assignment_for_c(
                xx.m_target, xx.m_value)) {
            pass_result.push_back(al, ASRUtils::STMT(
                ASRUtils::make_Assignment_t_util(al, loc, xx.m_target,
                    xx.m_value, xx.m_overloaded, xx.m_realloc_lhs,
                    xx.m_move_allocation)));
            return;
        }

        auto needs_array_indexed_loop = [&](ASR::expr_t *expr) {
            return (ASR::is_a<ASR::ArraySection_t>(*expr)
                    && !is_c_rank1_unit_section(expr))
                || (ASR::is_a<ASR::ArrayItem_t>(*expr)
                    && ASRUtils::is_array_indexed_with_array_indices(
                        ASR::down_cast<ASR::ArrayItem_t>(expr)));
        };
        if( needs_array_indexed_loop(xx.m_value) ||
            needs_array_indexed_loop(xx.m_target) ) {
            if (ASRUtils::is_array(ASRUtils::expr_type(xx.m_value))
                    && !skip_allocate_source_copy_realloc) {
                bool per_assign_realloc = xx.m_realloc_lhs ||
                    ASRUtils::is_allocatable(ASRUtils::expr_type(xx.m_target)) ||
                    should_auto_realloc_component_assignment(xx.m_target);
                bool inserted_indexed_realloc = false;
                if ((realloc_lhs || xx.m_realloc_lhs ||
                        should_auto_realloc_component_assignment(xx.m_target)) &&
                        ASR::is_a<ASR::Var_t>(*xx.m_target) &&
                        ASRUtils::is_allocatable(ASRUtils::expr_type(xx.m_target)) &&
                        needs_array_indexed_loop(xx.m_value)) {
                    inserted_indexed_realloc = insert_realloc_for_indexed_value(
                        xx.m_target, xx.m_value, loc);
                }
                if (!inserted_indexed_realloc) {
                    Vec<ASR::expr_t**> realloc_vars;
                    realloc_vars.reserve(al, 1);
                    realloc_vars.push_back(al,
                        const_cast<ASR::expr_t**>(&(xx.m_value)));
                    insert_realloc_for_target(xx.m_target, xx.m_value,
                        realloc_vars, per_assign_realloc);
                }
            }
            generate_loop_for_array_indexed_with_array_indices(
                x, &(xx.m_target), &(xx.m_value), loc);
            return ;
        }

        if( call_replace_on_expr(xx.m_value->type) ||
            ASR::is_a<ASR::ImpliedDoLoop_t>(*xx.m_value) ) {
            if (ASR::is_a<ASR::ImpliedDoLoop_t>(*xx.m_value)) {
                ASR::ImpliedDoLoop_t* idl = ASR::down_cast<ASR::ImpliedDoLoop_t>(xx.m_value);
                Vec<ASR::expr_t*> args;
                args.reserve(al, 1);
                args.push_back(al, xx.m_value);
                xx.m_value = ASRUtils::EXPR(ASR::make_ArrayConstructor_t(
                    al, idl->base.base.loc, args.p, args.size(),
                    idl->m_type, nullptr, ASR::arraystorageType::ColMajor, nullptr));
            }
            bool per_assign_realloc = !skip_allocate_source_copy_realloc
                && (xx.m_realloc_lhs ||
                    ASRUtils::is_allocatable(ASRUtils::expr_type(xx.m_target)) ||
                    should_auto_realloc_component_assignment(xx.m_target));
            if (per_assign_realloc && ASR::is_a<ASR::ArrayConstructor_t>(*xx.m_value)) {
                ASR::ArrayConstructor_t *ac =
                    ASR::down_cast<ASR::ArrayConstructor_t>(xx.m_value);
                ASR::Variable_t *target_base_var = get_base_variable(xx.m_target);
                static int array_ctor_temp_copy_counter = 0;
                if (target_base_var != nullptr) {
                    for (size_t i = 0; i < ac->n_args; i++) {
                        ASR::expr_t *arg = ac->m_args[i];
                        if (!ASRUtils::is_array(ASRUtils::expr_type(arg))) {
                            continue;
                        }
                        ASR::Variable_t *arg_base_var = get_base_variable(arg);
                        if (arg_base_var != nullptr && arg_base_var == target_base_var) {
                            ASR::ttype_t *arg_type = PassUtils::get_matching_type(arg, al);
                            ASR::expr_t *arg_tmp = PassUtils::create_var(
                                array_ctor_temp_copy_counter++, "_array_ctor_src_",
                                x.base.base.loc, arg_type, al, current_scope, arg);
                            pass_result.push_back(al, ASRUtils::STMT(
                                ASRUtils::make_Assignment_t_util(
                                    al, x.base.base.loc, arg_tmp, arg, nullptr, true, false)));
                            ac->m_args[i] = arg_tmp;
                        }
                    }
                }
                size_t target_rank = ASRUtils::extract_n_dims_from_ttype(
                    ASRUtils::expr_type(xx.m_target));
                if (target_rank == 1) {
                    const Location &loc = x.base.base.loc;
                    Vec<ASR::dimension_t> realloc_dims;
                    realloc_dims.reserve(al, 1);
                    ASR::dimension_t realloc_dim;
                    realloc_dim.loc = loc;
                    realloc_dim.m_start = make_ConstantWithKind(
                        make_IntegerConstant_t, make_Integer_t, 1, get_index_kind(), loc);
                    realloc_dim.m_length = ASRUtils::get_ArrayConstructor_size(al, ac);
                    realloc_dims.push_back(al, realloc_dim);

                    Vec<ASR::alloc_arg_t> alloc_args;
                    alloc_args.reserve(al, 1);
                    ASR::alloc_arg_t alloc_arg;
                    alloc_arg.loc = loc;
                    alloc_arg.m_a = xx.m_target;
                    alloc_arg.m_dims = realloc_dims.p;
                    alloc_arg.n_dims = realloc_dims.size();
                    alloc_arg.m_len_expr = nullptr;
                    alloc_arg.m_type = nullptr;
                    alloc_arg.m_sym_subclass = nullptr;
                    alloc_args.push_back(al, alloc_arg);

                    pass_result.push_back(al, ASRUtils::STMT(ASR::make_ReAlloc_t(
                        al, loc, alloc_args.p, alloc_args.size())));
                }
            }
            replacer.result_expr = xx.m_target;
            ASR::expr_t** current_expr_copy = current_expr;
            current_expr = const_cast<ASR::expr_t**>(&xx.m_value);
            this->call_replacer();
            current_expr = current_expr_copy;
            replacer.result_expr = nullptr;
            return ;
        }

        // Pre-evaluate any BitCast (transfer) nodes with array results
        // that are nested inside the value expression. BitCast is a
        // whole-array operation and must not be expanded element-wise.
        // We materialize them into temporaries before the loop so that
        // the loop can index into the temporary instead.
        // Skip when the value IS a BitCast directly — that case is
        // handled specially below.
        if (!ASR::is_a<ASR::BitCast_t>(*xx.m_value)) {
            BitCastArrayVisitor bc_visitor(al);
            bc_visitor.set_scope(current_scope);
            bc_visitor.init_bc_stmts();
            bc_visitor.current_expr = const_cast<ASR::expr_t**>(&(xx.m_value));
            bc_visitor.call_replacer();
            // Lower each `tmp = BitCast(...)` assignment through visit_stmt
            // so it goes through the full array_op lowering (element-wise
            // loop expansion for char BitCast, memcpy for mismatched sizes, etc.)
            Vec<ASR::stmt_t*>& bc_stmts = bc_visitor.get_bc_stmts();
            for (size_t i = 0; i < bc_stmts.size(); i++) {
                Vec<ASR::stmt_t*> saved_pass_result;
                saved_pass_result.from_pointer_n_copy(al,
                    pass_result.p, pass_result.size());
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                visit_stmt(*bc_stmts[i]);
                if (pass_result.size() > 0) {
                    for (size_t j = 0; j < pass_result.size(); j++) {
                        saved_pass_result.push_back(al, pass_result[j]);
                    }
                } else {
                    saved_pass_result.push_back(al, bc_stmts[i]);
                }
                pass_result.from_pointer_n_copy(al,
                    saved_pass_result.p, saved_pass_result.size());
            }
        }

        Vec<ASR::expr_t**> vars;
        vars.reserve(al, 1);
        ArrayVarAddressCollector var_collector_target(al, vars);
        var_collector_target.current_expr = const_cast<ASR::expr_t**>(&(xx.m_target));
        if (!is_target_assumed_rank) {
            var_collector_target.call_replacer();
        } else {
            vars.push_back(al, const_cast<ASR::expr_t**>(&(xx.m_target)));
        }
        ArrayVarAddressCollector var_collector_value(al, vars);
        var_collector_value.current_expr = const_cast<ASR::expr_t**>(&(xx.m_value));
        if (!is_value_assumed_rank) {
            var_collector_value.call_replacer();
        } else {
            vars.push_back(al, const_cast<ASR::expr_t**>(&(xx.m_value)));
        }

        // Collect array variables from overloaded SubroutineCall arguments
        // so they are replaced with indexed ArrayItem nodes in generate_loop
        if (xx.m_overloaded != nullptr &&
                ASR::is_a<ASR::SubroutineCall_t>(*xx.m_overloaded)) {
            ASR::SubroutineCall_t* sc = ASR::down_cast<ASR::SubroutineCall_t>(
                xx.m_overloaded);
            if (ASRUtils::is_elemental(sc->m_name)) {
                for (size_t i = 0; i < sc->n_args; i++) {
                    if (sc->m_args[i].m_value != nullptr &&
                            ASRUtils::is_array(ASRUtils::expr_type(
                                sc->m_args[i].m_value))) {
                        vars.push_back(al, &(sc->m_args[i].m_value));
                    }
                }
            }
        }

        if (vars.size() == 1 && !is_looping_necessary_for_bitcast(xx.m_value) && 
            ASRUtils::is_array(ASRUtils::expr_type(ASRUtils::get_past_array_broadcast(xx.m_value)))
        ) {
            return ;
        }

        if (ASRUtils::is_array(ASRUtils::expr_type(xx.m_value))
                && !skip_allocate_source_copy_realloc) {
            bool per_assign_realloc = xx.m_realloc_lhs ||
                ASRUtils::is_allocatable(ASRUtils::expr_type(xx.m_target)) ||
                should_auto_realloc_component_assignment(xx.m_target);
            insert_realloc_for_target(xx.m_target, xx.m_value, vars, per_assign_realloc);
        }

        if (bounds_checking && 
            ASRUtils::is_array(ASRUtils::expr_type(x.m_target)) &&
            ASRUtils::is_array(ASRUtils::expr_type(x.m_value)) &&
            ASRUtils::get_expr_size_expr(x.m_target) != nullptr) {
            ASRUtils::ExprStmtDuplicator expr_duplicator(al);
            ASR::expr_t* d_target = expr_duplicator.duplicate_expr(x.m_target);
            ASR::expr_t* d_value = expr_duplicator.duplicate_expr(x.m_value);

            Vec<ASR::expr_t*> vars;
            vars.reserve(al, 1);

            CollectComponentsFromElementalExpr cv(al, vars);
            cv.visit_expr(*d_value);

            if (debug_inserted.find(&x) == debug_inserted.end()) {
                pass_result.push_back(al, ASRUtils::STMT(ASR::make_DebugCheckArrayBounds_t(al, x.base.base.loc, d_target, vars.p, vars.n, x.m_move_allocation)));
                if (!x.m_move_allocation) {
                    debug_inserted.insert(&x);
                }
            }
        }

        // Don't generate a loop for a move assignment
        // The assignment should be handled in the backend
        if (x.m_move_allocation) {
            if (ASRUtils::is_allocatable(ASRUtils::expr_type(x.m_target)) && vars.size() != 1) {
                // ReAlloc was inserted for an allocatable target in previous part,
                // but the move assignment overwrites target.data
                // with value.data — leaking the buffer ReAlloc just allocated.
                // Free it first to avoid this memory leak. 
                // Note: This fix is temporary. This fix should be in backend, where 
                // while assigning to any allocatable variable target, backend should 
                // deallocate target first to free any heap allocation (malloc) used
                // to store in the target variable
                Vec<ASR::expr_t*> dealloc_vars;
                dealloc_vars.reserve(al, 1); 
                dealloc_vars.push_back(al, const_cast<ASR::expr_t*>(x.m_target));
                pass_result.push_back(al, ASRUtils::STMT(ASR::make_ImplicitDeallocate_t(al, loc,
                    dealloc_vars.p, dealloc_vars.size())));
            } 
            ASR::stmt_t* stmt = ASRUtils::STMT(ASRUtils::make_Assignment_t_util(al, loc, x.m_target, x.m_value, x.m_overloaded, x.m_realloc_lhs, x.m_move_allocation));
            pass_result.push_back(al, stmt);
            debug_inserted.insert(ASR::down_cast<ASR::Assignment_t>(stmt));
            return;
        }

        if (ASR::is_a<ASR::BitCast_t>(*xx.m_value)) {
            ASR::BitCast_t* bc = ASR::down_cast<ASR::BitCast_t>(xx.m_value);
            ASR::ttype_t* src_type = ASRUtils::expr_type(bc->m_source);
            ASR::ttype_t* result_type = bc->m_type;

            auto is_len_one_character_array = [](ASR::ttype_t* type) -> bool {
                type = ASRUtils::type_get_past_allocatable_pointer(type);
                if (!type || !ASRUtils::is_array(type)) {
                    return false;
                }
                ASR::ttype_t* element_type = ASRUtils::extract_type(type);
                if (!element_type || !ASRUtils::is_character(*element_type)) {
                    return false;
                }
                ASR::String_t* string_type = ASR::down_cast<ASR::String_t>(element_type);
                int64_t len = 0;
                return string_type->m_len_kind == ASR::string_length_kindType::ExpressionLength
                    && string_type->m_len != nullptr
                    && ASRUtils::extract_value(string_type->m_len, len)
                    && len == 1;
            };

            // transfer(scalar, char(1) array mold) is a byte reinterpretation of
            // the scalar into a whole character array. Lowering it as an
            // element-wise array loop is incorrect because each destination
            // element sees the full scalar expression again. Leave the whole
            // BitCast assignment intact for the backend to lower.
            if (is_len_one_character_array(result_type) &&
                (!ASRUtils::is_array(src_type) || ASR::is_a<ASR::ArrayItem_t>(*bc->m_source))) {
                ASR::stmt_t* stmt = ASRUtils::STMT(
                    ASRUtils::make_Assignment_t_util(al, loc,
                        x.m_target, x.m_value, x.m_overloaded,
                        x.m_realloc_lhs, x.m_move_allocation));
                pass_result.push_back(al, stmt);
                return;
            }

            if (bc->m_size != nullptr &&
                ASRUtils::is_array(src_type) &&
                !ASRUtils::is_string_only(src_type)) {
                ASR::stmt_t* stmt = ASRUtils::STMT(
                    ASRUtils::make_Assignment_t_util(al, loc,
                        x.m_target, x.m_value, x.m_overloaded,
                        x.m_realloc_lhs, x.m_move_allocation));
                pass_result.push_back(al, stmt);
                return;
            }

            // Don't generate an element-wise loop for transfer() (BitCast) when
            // source and result are arrays with different element byte sizes.
            // The element counts differ so the loop indices would go out
            // of bounds on the source array. Leave the whole-array BitCast for
            // the LLVM backend to lower as a memcpy.
            if (ASRUtils::is_array(src_type) && ASRUtils::is_array(bc->m_type)) {
                int64_t src_size = ASRUtils::get_type_byte_size(
                    ASRUtils::extract_type(src_type));
                int64_t res_size = ASRUtils::get_type_byte_size(
                    ASRUtils::extract_type(bc->m_type));
                if (src_size != res_size) {
                    pass_result.push_back(al,
                        const_cast<ASR::stmt_t*>(&(x.base)));
                    return;
                }
            }
        }

        Vec<ASR::expr_t**> fix_type_args;
        fix_type_args.reserve(al, 2);
        fix_type_args.push_back(al, const_cast<ASR::expr_t**>(&(xx.m_target)));
        fix_type_args.push_back(al, const_cast<ASR::expr_t**>(&(xx.m_value)));
        generate_loop(x, vars, fix_type_args, loc);
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
        if( !ASRUtils::is_elemental(x.m_name) ) {
            return ;
        }
        const Location loc = x.base.base.loc;

        Vec<ASR::expr_t**> vars;
        vars.reserve(al, 1);
        for( size_t i = 0; i < x.n_args; i++ ) {
            if( x.m_args[i].m_value != nullptr &&
                ASRUtils::is_array(ASRUtils::expr_type(x.m_args[i].m_value)) ) {
                vars.push_back(al, &(x.m_args[i].m_value));
            }
        }

        // If x.m_dt is an array, and the subroutine is elemental, we need to replace the m_dt with an array item
        // and call the elemental function on each element of the x.m_dt array not on the whole array
        if (x.m_dt && ASRUtils::is_array(ASRUtils::expr_type(x.m_dt))) {
            ASR::SubroutineCall_t& xx = const_cast<ASR::SubroutineCall_t&>(x);
            vars.push_back(al, &(xx.m_dt));
        }

        if( vars.size() == 0 ) {
            return ;
        }

        Vec<ASR::expr_t**> fix_type_args;
        fix_type_args.reserve(al, 1);

        generate_loop(x, vars, fix_type_args, loc);
    }

    void visit_If(const ASR::If_t& x) {
        if( !ASRUtils::is_array(ASRUtils::expr_type(x.m_test)) ) {
            ASR::CallReplacerOnExpressionsVisitor<ArrayOpVisitor>::visit_If(x);
            return ;
        }

        const Location loc = x.base.base.loc;

        Vec<ASR::expr_t**> vars;
        vars.reserve(al, 1);
        ArrayVarAddressCollector array_var_adress_collector_target(al, vars);
        array_var_adress_collector_target.visit_If(x);

        if( vars.size() == 0 ) {
            return ;
        }

        std::vector<ASR::expr_t*> scalar_targets;
        collect_array_if_scalar_targets(x.m_body, x.n_body, scalar_targets);
        collect_array_if_scalar_targets(x.m_orelse, x.n_orelse, scalar_targets);

        Vec<ASR::expr_t**> fix_type_args;
        fix_type_args.reserve(al, 1);

        generate_loop(x, vars, fix_type_args, loc, &scalar_targets);

        ASRUtils::RemoveArrayProcessingNodeVisitor remove_array_processing_node_visitor(al);
        remove_array_processing_node_visitor.visit_If(x);

        FixTypeVisitor fix_type_visitor(al);
        fix_type_visitor.current_scope = current_scope;
        fix_type_visitor.visit_If(x);
    }

};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit,
                           const LCompilers::PassOptions& pass_options) {
    ArrayOpVisitor v(al, pass_options);
    v.call_replacer_on_value = false;
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
