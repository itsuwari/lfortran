#ifndef LFORTRAN_ASR_TO_C_H
#define LFORTRAN_ASR_TO_C_H

#include <libasr/asr.h>
#include <libasr/utils.h>
#include <vector>

namespace LCompilers {

    struct CTranslationUnitSplitResult {
        std::vector<std::string> source_files;
        std::string header_file;
        bool has_main_program = false;
    };

    Result<std::string> asr_to_c(Allocator &al, ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics, CompilerOptions &co,
        int64_t default_lower_bound);

    Result<CTranslationUnitSplitResult> asr_to_c_split(Allocator &al,
        ASR::TranslationUnit_t &asr, diag::Diagnostics &diagnostics,
        CompilerOptions &co, int64_t default_lower_bound,
        const std::string &output_dir, const std::string &project_name);

} // namespace LCompilers

#endif // LFORTRAN_ASR_TO_C_H
