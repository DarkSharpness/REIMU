#pragma once
#include "assembly/forward.h"
#include "declarations.h"
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace dark {

struct AssemblyLayout {
    using _Storage_t = std::unique_ptr<Storage>;

    struct SectionStorage {
        std::span<_Storage_t> storages;
        Section section;
    };

    struct LabelData {
        std::size_t line_number;
        _Storage_t *storage;
        std::string label_name;
        bool global;
        Section section;
    };

    std::vector<SectionStorage> sections;
    std::vector<LabelData> labels;

    // The real storage of span may be hidden within.
    std::vector<_Storage_t> static_pool;
};

} // namespace dark
