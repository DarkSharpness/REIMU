#include <utility.h>
#include <storage.h>
#include <linker/linker.h>
#include <linker/helper.h>
#include <assembly/assembly.h>

namespace dark {

static void visit_section(Linker::_Section_Vec_t &vec, SizeEstimator &estimator) {
    for (auto iter : vec) {
        auto &[base, details] = *iter;
        auto size = details.section_size();
        details.offsets.push_back(0);
        std::size_t start = estimator.get_position();
        details.absolute_offset = start;
        for (auto &storage : std::span(base, size)) {
            estimator.visit(*storage);
            details.offsets.push_back(estimator.get_position() - start);
        }
        runtime_assert(details.section_size() == size);
    }
}

Linker::Linker(std::span <Assembler> data) {
    for (auto &assembler : data) {
        for (auto [slice, section] : assembler.split_by_section()) {
            auto [iter, success] = storage_map.try_emplace(slice.data());
            runtime_assert(success);
            iter->second.offsets.reserve(slice.size() + 1);
            auto index  = static_cast <std::size_t> (section);
            runtime_assert(index < this->kSections);
            auto &vec   = this->section_vec[index];
            vec.push_back(iter);
        }
    }

    // Memory Layout:
    // 1. Text Section from 0x10000 # 64KiB
    //  1.1 Libc functions
    //  1.2 User functions
    // 2. Static Data Section next
    //  2.1 Data Section
    //  2.2 RODATA Section
    //  2.3 BSS Section
    //   (all these parts may be merged into one section)
    // 3. Free Space until 0x10000000 # 256MiB
    // 4. Stack from 0x10000000 # 256MiB
    //          to   0x20000000 # 512MiB

    // This is required by the RISC-V ABI

    // Set the initial offset for each section
    SizeEstimator estimator { 0x10000 };

    const auto __get_section = [this](Section section) -> _Section_Vec_t & {
        return this->section_vec[static_cast <std::size_t> (section)];
    };

    constexpr std::size_t kPageSize = 0x1000;

    visit_section(__get_section(Section::TEXT), estimator);
    estimator.align_to(kPageSize);
    visit_section(__get_section(Section::DATA), estimator);
    visit_section(__get_section(Section::RODATA), estimator);
    visit_section(__get_section(Section::BSS), estimator);
    estimator.align_to(kPageSize);
}

} // namespace dark
