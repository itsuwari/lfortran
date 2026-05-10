#include <string>
#include <set>
#include <vector>

#include <libasr/config.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/modfile.h>
#include <libasr/serialization.h>
#include <libasr/bwriter.h>
#include <libasr/exception.h>

namespace LCompilers {

const std::string lfortran_modfile_type_string = "LCompilers Modfile";
static constexpr uint32_t lfortran_modfile_format_version = 0x4D4F4432; // "MOD2"

namespace {

struct SavedStmtBody {
    ASR::stmt_t ***body_ptr;
    size_t *n_body_ptr;
    ASR::stmt_t **body;
    size_t n_body;
};

struct SavedDependencies {
    char ***dependencies_ptr;
    size_t *n_dependencies_ptr;
    char **dependencies;
    size_t n_dependencies;
    std::vector<char*> filtered_dependencies;
};

struct SavedSymbol {
    SymbolTable *symtab;
    std::string name;
    ASR::symbol_t *symbol;
};

class ModfileBodyStripper {
    std::vector<SavedStmtBody> saved_bodies;
    std::vector<SavedDependencies> saved_dependencies;
    std::vector<SavedSymbol> saved_symbols;

    void save_body(ASR::stmt_t **&body, size_t &n_body) {
        if (n_body == 0) {
            return;
        }
        saved_bodies.push_back({&body, &n_body, body, n_body});
        body = nullptr;
        n_body = 0;
    }

    void filter_dependencies(char **&dependencies, size_t &n_dependencies,
            const std::set<std::string> &interface_dependencies) {
        if (n_dependencies == 0) {
            return;
        }
        std::vector<char*> filtered;
        filtered.reserve(n_dependencies);
        for (size_t i = 0; i < n_dependencies; i++) {
            if (interface_dependencies.find(std::string(dependencies[i]))
                    != interface_dependencies.end()) {
                filtered.push_back(dependencies[i]);
            }
        }
        if (filtered.size() == n_dependencies) {
            return;
        }
        saved_dependencies.push_back({&dependencies, &n_dependencies,
            dependencies, n_dependencies, std::move(filtered)});
        SavedDependencies &saved = saved_dependencies.back();
        if (saved.filtered_dependencies.empty()) {
            dependencies = nullptr;
            n_dependencies = 0;
        } else {
            dependencies = saved.filtered_dependencies.data();
            n_dependencies = saved.filtered_dependencies.size();
        }
    }

    void strip_unreachable_symbols(SymbolTable *symtab,
            const std::set<std::string> &interface_symbols) {
        std::vector<SavedSymbol> stripped_symbols;
        for (auto &item : symtab->get_scope()) {
            if (interface_symbols.find(item.first) == interface_symbols.end()) {
                stripped_symbols.push_back({symtab, item.first, item.second});
            }
        }
        for (auto &item : stripped_symbols) {
            item.symtab->erase_symbol(item.name);
            saved_symbols.push_back(item);
        }
    }

    bool is_inline_function(const ASR::Function_t &x) const {
        if (x.m_function_signature &&
                ASR::is_a<ASR::FunctionType_t>(*x.m_function_signature)) {
            ASR::FunctionType_t *ft = ASR::down_cast<ASR::FunctionType_t>(x.m_function_signature);
            return ft->m_inline;
        }
        return false;
    }

    class InterfaceDependencyCollector
        : public ASR::BaseWalkVisitor<InterfaceDependencyCollector> {
        ASR::Function_t &function;
        SymbolTable *function_symtab;
        SymbolTable *parent_symtab;

        void add_symbol_table_key(SymbolTable *symtab, ASR::symbol_t *sym) {
            if (symtab == nullptr || sym == nullptr) {
                return;
            }
            for (auto &item : symtab->get_scope()) {
                if (item.second == sym) {
                    dependencies.insert(item.first);
                }
            }
        }

        void add_symbol(ASR::symbol_t *sym, bool keep_procedure_dependency=false) {
            if (sym == nullptr) {
                return;
            }
            ASR::symbol_t *past_external = ASRUtils::symbol_get_past_external(sym);
            if (!keep_procedure_dependency && past_external != nullptr
                    && ASR::is_a<ASR::Function_t>(*past_external)) {
                return;
            }
            std::string name = ASRUtils::symbol_name(sym);
            if (function_symtab && function_symtab->get_symbol(name)) {
                dependencies.insert(name);
            }
            if (parent_symtab && parent_symtab->get_symbol(name)) {
                dependencies.insert(name);
            }
            add_symbol_table_key(function_symtab, sym);
            add_symbol_table_key(parent_symtab, sym);
        }

        void collect_function_interface(const ASR::Function_t &x) {
            if (x.m_function_signature) {
                this->visit_ttype(*x.m_function_signature);
            }
            for (size_t i = 0; i < x.n_args; i++) {
                this->visit_expr(*x.m_args[i]);
                if (ASR::is_a<ASR::Var_t>(*x.m_args[i])) {
                    ASR::symbol_t *sym = ASR::down_cast<ASR::Var_t>(
                        x.m_args[i])->m_v;
                    this->visit_symbol(*sym);
                }
            }
            if (x.m_return_var) {
                this->visit_expr(*x.m_return_var);
                if (ASR::is_a<ASR::Var_t>(*x.m_return_var)) {
                    ASR::symbol_t *sym = ASR::down_cast<ASR::Var_t>(
                        x.m_return_var)->m_v;
                    this->visit_symbol(*sym);
                }
            }
        }

        void collect_dependency_closure() {
            bool changed = true;
            while (changed) {
                size_t n_dependencies = dependencies.size();
                std::vector<std::string> current_dependencies(
                    dependencies.begin(), dependencies.end());
                for (const std::string &name : current_dependencies) {
                    if (function_symtab) {
                        if (ASR::symbol_t *sym = function_symtab->get_symbol(name)) {
                            this->visit_symbol(*sym);
                        }
                    }
                }
                changed = dependencies.size() != n_dependencies;
            }
        }

    public:
        std::set<std::string> dependencies;

        InterfaceDependencyCollector(ASR::Function_t &function) :
            function(function), function_symtab(function.m_symtab),
            parent_symtab(function.m_symtab ? function.m_symtab->parent : nullptr) {
        }

        void collect() {
            collect_function_interface(function);
            collect_dependency_closure();
        }

        void visit_Function(const ASR::Function_t &x) {
            collect_function_interface(x);
        }

        void visit_GenericProcedure(const ASR::GenericProcedure_t &x) {
            for (size_t i = 0; i < x.n_procs; i++) {
                add_symbol(x.m_procs[i], true);
            }
        }

        void visit_CustomOperator(const ASR::CustomOperator_t &x) {
            for (size_t i = 0; i < x.n_procs; i++) {
                add_symbol(x.m_procs[i], true);
            }
        }

        void visit_StructMethodDeclaration(const ASR::StructMethodDeclaration_t &x) {
            add_symbol(x.m_proc, true);
        }

        void visit_Namelist(const ASR::Namelist_t &x) {
            for (size_t i = 0; i < x.n_var_list; i++) {
                add_symbol(x.m_var_list[i]);
            }
        }

        void visit_Variable(const ASR::Variable_t &x) {
            add_symbol(x.m_type_declaration);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_Variable(x);
        }

        void visit_StructInstanceMember(const ASR::StructInstanceMember_t &x) {
            add_symbol(x.m_m);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_StructInstanceMember(x);
        }

        void visit_StructStaticMember(const ASR::StructStaticMember_t &x) {
            add_symbol(x.m_m);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_StructStaticMember(x);
        }

        void visit_EnumStaticMember(const ASR::EnumStaticMember_t &x) {
            add_symbol(x.m_m);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_EnumStaticMember(x);
        }

        void visit_UnionInstanceMember(const ASR::UnionInstanceMember_t &x) {
            add_symbol(x.m_m);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_UnionInstanceMember(x);
        }

        void visit_FunctionCall(const ASR::FunctionCall_t &x) {
            add_symbol(x.m_name, true);
            if (x.m_original_name) {
                add_symbol(x.m_original_name, true);
            }
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_FunctionCall(x);
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
            add_symbol(x.m_name, true);
            if (x.m_original_name) {
                add_symbol(x.m_original_name, true);
            }
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_SubroutineCall(x);
        }

        void visit_Var(const ASR::Var_t &x) {
            add_symbol(x.m_v);
        }

        void visit_StructConstructor(const ASR::StructConstructor_t &x) {
            add_symbol(x.m_dt_sym);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_StructConstructor(x);
        }

        void visit_StructConstant(const ASR::StructConstant_t &x) {
            add_symbol(x.m_dt_sym);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_StructConstant(x);
        }

        void visit_EnumConstructor(const ASR::EnumConstructor_t &x) {
            add_symbol(x.m_dt_sym);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_EnumConstructor(x);
        }

        void visit_UnionConstructor(const ASR::UnionConstructor_t &x) {
            add_symbol(x.m_dt_sym);
            ASR::BaseWalkVisitor<InterfaceDependencyCollector>::visit_UnionConstructor(x);
        }
    };

    void strip_symbol_table(SymbolTable *symtab, bool preserve_bodies=false) {
        for (auto &item : symtab->get_scope()) {
            ASR::symbol_t *sym = item.second;
            switch (sym->type) {
                case ASR::symbolType::Module: {
                    ASR::Module_t *x = ASR::down_cast<ASR::Module_t>(sym);
                    strip_symbol_table(x->m_symtab);
                    break;
                }
                case ASR::symbolType::Function: {
                    ASR::Function_t *x = ASR::down_cast<ASR::Function_t>(sym);
                    bool preserve_function_body = is_inline_function(*x);
                    strip_symbol_table(x->m_symtab, preserve_function_body);
                    if (!preserve_function_body) {
                        InterfaceDependencyCollector collector(*x);
                        collector.collect();
                        save_body(x->m_body, x->n_body);
                        filter_dependencies(x->m_dependencies, x->n_dependencies,
                            collector.dependencies);
                        strip_unreachable_symbols(x->m_symtab, collector.dependencies);
                    }
                    break;
                }
                case ASR::symbolType::Struct: {
                    ASR::Struct_t *x = ASR::down_cast<ASR::Struct_t>(sym);
                    strip_symbol_table(x->m_symtab, preserve_bodies);
                    break;
                }
                case ASR::symbolType::Enum: {
                    ASR::Enum_t *x = ASR::down_cast<ASR::Enum_t>(sym);
                    strip_symbol_table(x->m_symtab, preserve_bodies);
                    break;
                }
                case ASR::symbolType::Union: {
                    ASR::Union_t *x = ASR::down_cast<ASR::Union_t>(sym);
                    strip_symbol_table(x->m_symtab, preserve_bodies);
                    break;
                }
                case ASR::symbolType::AssociateBlock: {
                    ASR::AssociateBlock_t *x = ASR::down_cast<ASR::AssociateBlock_t>(sym);
                    strip_symbol_table(x->m_symtab, preserve_bodies);
                    if (!preserve_bodies) {
                        save_body(x->m_body, x->n_body);
                    }
                    break;
                }
                case ASR::symbolType::Block: {
                    ASR::Block_t *x = ASR::down_cast<ASR::Block_t>(sym);
                    strip_symbol_table(x->m_symtab, preserve_bodies);
                    if (!preserve_bodies) {
                        save_body(x->m_body, x->n_body);
                    }
                    break;
                }
                case ASR::symbolType::GpuKernelFunction: {
                    ASR::GpuKernelFunction_t *x = ASR::down_cast<ASR::GpuKernelFunction_t>(sym);
                    strip_symbol_table(x->m_symtab, preserve_bodies);
                    break;
                }
                default:
                    break;
            }
        }
    }

public:
    ModfileBodyStripper(const ASR::TranslationUnit_t &m) {
        strip_symbol_table(m.m_symtab);
    }

    ~ModfileBodyStripper() {
        for (auto it = saved_symbols.rbegin(); it != saved_symbols.rend(); ++it) {
            it->symtab->add_symbol(it->name, it->symbol);
        }
        for (auto it = saved_dependencies.rbegin(); it != saved_dependencies.rend(); ++it) {
            *it->dependencies_ptr = it->dependencies;
            *it->n_dependencies_ptr = it->n_dependencies;
        }
        for (auto it = saved_bodies.rbegin(); it != saved_bodies.rend(); ++it) {
            *it->body_ptr = it->body;
            *it->n_body_ptr = it->n_body;
        }
    }
};

} // namespace

inline void save_asr(const ASR::TranslationUnit_t &m, std::string& asr_string, LCompilers::LocationManager lm) {
    #ifdef WITH_LFORTRAN_BINARY_MODFILES
    BinaryWriter b;
#else
    TextWriter b;
#endif
    // Header
    b.write_string(lfortran_modfile_type_string);
    b.write_string(LFORTRAN_VERSION);
    b.write_int32(lfortran_modfile_format_version);

    // AST section: Original module source code:
    // Currently empty.
    // Note: in the future we can save here:
    // * A path to the original source code
    // * Hash of the orig source code
    // * AST binary export of it (this AST only changes if the hash changes)

    // ASR section:

    // Export ASR:
    // Currently empty.

    // Save only the module's own location map. Imported modules loaded while
    // compiling this module have their own modfiles, and serializing their
    // location maps into every dependent modfile makes downstream builds carry
    // transitive diagnostic metadata repeatedly.
    LCompilers::LocationManager mod_lm;
    if (!lm.files.empty()) {
        mod_lm.files.push_back(lm.files[0]);
    }
    if (!lm.file_ends.empty()) {
        mod_lm.file_ends.push_back(lm.file_ends[0]);
    }

    // LocationManager:
    b.write_int32(mod_lm.files.size());
    for(auto file: mod_lm.files) {
        // std::vector<FileLocations> files;
        b.write_string(file.in_filename);
        b.write_int32(file.current_line);

        // std::vector<uint32_t> out_start
        b.write_int32(file.out_start.size());
        for(auto i: file.out_start) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_start
        b.write_int32(file.in_start.size());
        for(auto i: file.in_start) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_newlines
        b.write_int32(file.in_newlines.size());
        for(auto i: file.in_newlines) {
            b.write_int32(i);
        }

        // bool preprocessor
        b.write_int32(file.preprocessor);

        // std::vector<uint32_t> out_start0
        b.write_int32(file.out_start0.size());
        for(auto i: file.out_start0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_start0
        b.write_int32(file.in_start0.size());
        for(auto i: file.in_start0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_size0
        b.write_int32(file.in_size0.size());
        for(auto i: file.in_size0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> interval_type0
        b.write_int32(file.interval_type0.size());
        for(auto i: file.interval_type0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_newlines0
        b.write_int32(file.in_newlines0.size());
        for(auto i: file.in_newlines0) {
            b.write_int32(i);
        }
    }

    // std::vector<uint32_t> file_ends
    b.write_int32(mod_lm.file_ends.size());
    for(auto i: mod_lm.file_ends) {
        b.write_int32(i);
    }

    // Full ASR:
    b.write_string(serialize(m));

    asr_string = b.get_str();
}

// The save_modfile() and load_modfile() must stay consistent. What is saved
// must be loaded in exactly the same order.

/*
    Saves the module into a binary stream.

    That stream can be saved to a mod file by the caller.
    The sections in the file/stream are saved using write_string(), so they
    can be efficiently read by the loader and ignored if needed.

    Comments below show some possible future improvements to the mod format.
*/
std::string save_modfile(const ASR::TranslationUnit_t &m, LCompilers::LocationManager lm) {
    LCOMPILERS_ASSERT(m.m_symtab->get_scope().size()== 1);
    for (auto &a : m.m_symtab->get_scope()) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
        if ((bool&)a) { } // Suppress unused warning in Release mode
    }

    std::string asr_string;
    // A module file is an interface artifact. Procedure bodies are emitted in
    // the owning object file; keeping them in every dependent .mod file makes
    // downstream compiles repeatedly deserialize and walk implementation ASR.
    ModfileBodyStripper strip_bodies(m);
    save_asr(m, asr_string, lm);
    return asr_string;
}

std::string save_pycfile(const ASR::TranslationUnit_t &m, LCompilers::LocationManager lm) {
    std::string asr_string;
    save_asr(m, asr_string, lm);
    return asr_string;
}

inline bool load_serialised_asr(const std::string &s, std::string& asr_binary,
                                LCompilers::LocationManager &lm, std::string& error_message) {
    if (s.empty()) {
        error_message = "Modfile is empty";
        return false;
    }
#ifdef WITH_LFORTRAN_BINARY_MODFILES
    auto read_u32_be = [&](size_t &pos) -> uint32_t {
        if (pos + 4 > s.size()) {
            throw LCompilersException("load_modfile: String is too short for deserialization.");
        }
        uint32_t n = string_to_uint32(&s[pos]);
        pos += 4;
        return n;
    };
    auto read_u64_be = [&](size_t &pos) -> uint64_t {
        if (pos + 8 > s.size()) {
            throw LCompilersException("load_modfile: String is too short for deserialization.");
        }
        uint64_t n = string_to_uint64(&s[pos]);
        pos += 8;
        return n;
    };
    auto read_str_be = [&](size_t &pos) -> std::string {
        uint64_t n = read_u64_be(pos);
        if (pos + n > s.size()) {
            throw LCompilersException("load_modfile: String is too short for deserialization.");
        }
        std::string r(&s[pos], n);
        pos += n;
        return r;
    };
    size_t pos = 0;
#else
    TextReader b(s);
#endif
#ifdef WITH_LFORTRAN_BINARY_MODFILES
    std::string file_type = read_str_be(pos);
#else
    std::string file_type = b.read_string();
#endif
    if (file_type != lfortran_modfile_type_string) {
        error_message = "LCompilers Modfile format not recognized";
        return false;
    }
#ifdef WITH_LFORTRAN_BINARY_MODFILES
    std::string version = read_str_be(pos);
#else
    std::string version = b.read_string();
#endif
    // The serialized modfile schema is versioned independently below. Rejecting
    // all compiler build-string mismatches forces unnecessary clean rebuilds of
    // downstream build trees even when the modfile format is unchanged.
    (void)version;
#ifdef WITH_LFORTRAN_BINARY_MODFILES
    uint32_t format_version = read_u32_be(pos);
    BinaryReader b(s.substr(pos));
#else
    uint32_t format_version = b.read_int32();
#endif
    if (format_version != lfortran_modfile_format_version) {
        error_message = "Incompatible format: LFortran Modfile schema version is '"
                        + std::to_string(format_version)
                        + "', but current LFortran modfile schema version is '"
                        + std::to_string(lfortran_modfile_format_version)
                        + "'. Rebuild all dependent modules with the current compiler.";
        return false;
    }
    LCompilers::LocationManager serialized_lm;
    int32_t n_files = b.read_int32();
    std::vector<LCompilers::LocationManager::FileLocations> files;
    for(int i=0; i<n_files; i++) {
        LCompilers::LocationManager::FileLocations file;
        file.in_filename = b.read_string();
        file.current_line = b.read_int32();

        int32_t n_out_start = b.read_int32();
        for(int i=0; i<n_out_start; i++) {
            file.out_start.push_back(b.read_int32());
        }

        int32_t n_in_start = b.read_int32();
        for(int i=0; i<n_in_start; i++) {
            file.in_start.push_back(b.read_int32());
        }

        int32_t n_in_newlines = b.read_int32();
        for(int i=0; i<n_in_newlines; i++) {
            file.in_newlines.push_back(b.read_int32());
        }

        file.preprocessor = b.read_int32();

        int32_t n_out_start0 = b.read_int32();
        for(int i=0; i<n_out_start0; i++) {
            file.out_start0.push_back(b.read_int32());
        }

        int32_t n_in_start0 = b.read_int32();
        for(int i=0; i<n_in_start0; i++) {
            file.in_start0.push_back(b.read_int32());
        }

        int32_t n_in_size0 = b.read_int32();
        for(int i=0; i<n_in_size0; i++) {
            file.in_size0.push_back(b.read_int32());
        }

        int32_t n_interval_type0 = b.read_int32();
        for(int i=0; i<n_interval_type0; i++) {
            file.interval_type0.push_back(b.read_int32());
        }

        int32_t n_in_newlines0 = b.read_int32();
        for(int i=0; i<n_in_newlines0; i++) {
            file.in_newlines0.push_back(b.read_int32());
        }

        serialized_lm.files.push_back(file);
    }

    int32_t n_file_ends = b.read_int32();
    for(int i=0; i<n_file_ends; i++) {
        serialized_lm.file_ends.push_back(b.read_int32());
    }

    // Append the module's location information into the current LocationManager.
    // The serialized LocationManager was built with the module starting at 0,
    // so we shift its output positions by the current global offset while
    // keeping input positions unchanged.
    const uint32_t offset = lm.file_ends.empty() ? 0 : lm.file_ends.back();
    LCompilers::LocationManager::FileLocations adjusted_file = serialized_lm.files[0];
    for (size_t i = 0; i < adjusted_file.out_start.size(); i++) {
        adjusted_file.out_start[i] += offset;
    }
    // Note: we do NOT adjust out_start0 because the preprocessor remapping
    // operates on positions that are already file-relative (after the first
    // level of remapping via out_start/in_start).
    lm.files.push_back(adjusted_file);
    lm.file_ends.push_back(serialized_lm.file_ends[0] + offset);

    asr_binary = b.read_string();
    return true;
}

Result<ASR::TranslationUnit_t*, ErrorMessage> load_modfile(Allocator &al, const std::string &s,
        bool load_symtab_id, SymbolTable &symtab, LCompilers::LocationManager &lm) {
    std::string asr_binary;
    std::string error_message;
    if (!load_serialised_asr(s, asr_binary, lm, error_message)) {
        return ErrorMessage(error_message);
    }
    // take offset as second-to-last element of file_ends (the offset
    // before load_serialised_asr pushed its entry). When file_ends
    // has fewer than 2 elements the modfile was loaded into a fresh
    // LocationManager, so the base offset is 0.
    uint32_t offset = lm.file_ends.size() >= 2
        ? lm.file_ends[lm.file_ends.size()-2] : 0;
    ASR::asr_t *asr = nullptr;
    try {
        asr = deserialize_asr(al, asr_binary, load_symtab_id, symtab, offset);
    } catch (const LCompilersException &e) {
        return ErrorMessage(
            "Failed to deserialize an LFortran modfile. "
            "The module is likely stale or incompatible with the current compiler; "
            "rebuild all dependent modules with the current compiler. Details: " + e.msg());
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
    return tu;
}

Result<ASR::TranslationUnit_t*, ErrorMessage> load_pycfile(Allocator &al, const std::string &s,
        bool load_symtab_id, LCompilers::LocationManager &lm) {
    std::string asr_binary;
    std::string error_message;
    if (!load_serialised_asr(s, asr_binary, lm, error_message)) {
        return ErrorMessage(error_message);
    }
    uint32_t offset = 0;
    ASR::asr_t *asr = nullptr;
    try {
        asr = deserialize_asr(al, asr_binary, load_symtab_id, offset);
    } catch (const LCompilersException &e) {
        return ErrorMessage(
            "Failed to deserialize an LFortran pyc file. "
            "The file is likely stale or incompatible with the current compiler. Details: "
            + e.msg());
    }

    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
    return tu;
}

} // namespace LCompilers
