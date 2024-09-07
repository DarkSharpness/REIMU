#pragma once
#include "assembly/forward.h"
#include "assembly/storage/visitor.h"
#include "assembly/storage/immediate.h"

namespace dark {

struct StaticData : Storage {};

struct Alignment final : StaticData {
    std::size_t alignment;
    explicit Alignment(std::size_t alignment);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct IntegerData final : StaticData {
    Immediate data;
    enum class Type {
        BYTE = 0, SHORT = 1, LONG = 2
    } type;
    explicit IntegerData(Immediate data, Type type);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct ZeroBytes final : StaticData {
    std::size_t count;
    explicit ZeroBytes(std::size_t count);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

struct ASCIZ final : StaticData {
    std::string data;
    explicit ASCIZ(std::string str);
    void debug(std::ostream &os) const override;
    void accept(StorageVisitor &visitor) override { visitor.visitStorage(*this); }
};

} // namespace dark
