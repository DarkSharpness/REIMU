#pragma once
#include "assembly/forward.h"
#include "assembly/storage/visitor.h"
#include "fmtlib.h"
#include "linker/exception.h"

namespace dark {

struct LinkVisitor : StorageVisitor {
    // Visit a storage object and handle any linkage errors.
    template <typename _F>
    auto visit_safe(Storage &storage, _F &&f) -> void {
        try {
            std::forward<_F>(f)(storage);
        } catch (FailToLink &e) {
            handle_build_failure(
                fmt::format("Fail to link source assembly.\n  {}", e.inner),
                storage.line_info.to_string(), storage.line_info.line
            );
        }
    }
};

} // namespace dark
