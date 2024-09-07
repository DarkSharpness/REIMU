#pragma once
#include "assembly/forward.h"
#include "declarations.h"
#include <memory>

namespace dark {

struct Immediate {
    std::unique_ptr <ImmediateBase> data;
    explicit Immediate() = default;
    explicit Immediate(std::unique_ptr <ImmediateBase> data) noexcept : data(std::move(data)) {}
    explicit Immediate(target_size_t data);
    std::string to_string() const;
};
    
} // namespace dark
