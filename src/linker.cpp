#include <utility.h>
#include <storage.h>
#include <linker/linker.h>
#include <linker/helper.h>
#include <assembly/assembly.h>

namespace dark {

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
    //  2.2 RODATA Section (may merge with Data Section)
    //  2.3 BSS Section
    // 3. Free Space until 0x10000000 # 256MiB
    // 4. Stack from 0x10000000 # 256MiB
    //          to   0x20000000 # 512MiB

    // This is required by the RISC-V ABI
    std::size_t position = 0x10000;

    auto &text = this->section_vec[static_cast <std::size_t> (Section::TEXT)];
    for (auto iter : text) {
        auto &[base, details] = *iter;
        auto size = details.section_size();
        for (auto &storage : std::span(base, size)) {

        }
    }
}

} // namespace dark
