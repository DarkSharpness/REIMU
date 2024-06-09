// Should only be included once by linker.cpp
#include "linker.h"
#include <storage.h>
#include <ustring.h>

namespace dark {

namespace __details {

template <std::derived_from <RealData> _Data>
std::size_t align_size(_Data &storage) {
    if constexpr (std::same_as <_Data, Alignment>) {
        return storage.alignment;
    } else if constexpr (std::same_as <_Data, IntegerData>) {
        return std::size_t(1) << static_cast <int> (storage.type);
    } else if constexpr (std::same_as <_Data, ZeroBytes>) {
        return 1;
    } else if constexpr (std::same_as <_Data, ASCIZ>) {
        return 1;
    } else {
        static_assert(false, "Invalid type");
    }
}

template <std::derived_from <RealData> _Data>
std::size_t real_size(_Data &storage) {
    if constexpr (std::same_as <_Data, Alignment>) {
        return 0;
    } else if constexpr (std::same_as <_Data, IntegerData>) {
        return align_size(storage) * 1;
    } else if constexpr (std::same_as <_Data, ZeroBytes>) {
        return storage.count;
    } else if constexpr (std::same_as <_Data, ASCIZ>) {
        return storage.data.size() + 1; // Null terminator
    } else {
        static_assert(false, "Invalid type");
    }
}

} // namespace __details

struct SizeEstimator final : StorageVisitor {
    const std::size_t start;
    std::size_t size;
    static constexpr std::size_t kCommand = 4;

    void align_to(std::size_t alignment) {
        runtime_assert(std::has_single_bit(alignment));
        std::size_t bitmask = alignment - 1;
        std::size_t current = this->start + this->size;
        std::size_t aligned = (current + bitmask) & ~bitmask;
        this->size = aligned - this->start;
    }

    explicit SizeEstimator(std::size_t start) : start(start), size(0) {}

    void visitStorage(ArithmeticReg &storage)   override { this->update(storage, 1); }
    void visitStorage(ArithmeticImm &storage)   override { this->update(storage, 1); }
    void visitStorage(LoadStore &storage)       override { this->update(storage, 1); }
    void visitStorage(Branch &storage)          override { this->update(storage, 1); }
    void visitStorage(JumpRelative &storage)    override { this->update(storage, 1); }
    void visitStorage(JumpRegister &storage)    override { this->update(storage, 1); }
    void visitStorage(CallFunction &storage)    override { this->update(storage, 2); }
    void visitStorage(LoadImmediate &storage)   override { this->update(storage, 2); }
    void visitStorage(LoadUpperImmediate &storage)  override { this->update(storage, 1); }
    void visitStorage(AddUpperImmediatePC &storage) override { this->update(storage, 1); }
    void visitStorage(Alignment &storage)   override { this->update(storage); }
    void visitStorage(IntegerData &storage) override { this->update(storage); }
    void visitStorage(ZeroBytes &storage)   override { this->update(storage); }
    void visitStorage(ASCIZ &storage)       override { this->update(storage); }

    template <std::derived_from <RealData> _Data>
    void update(_Data &storage) {
        this->align_to(__details::align_size(storage));
        this->size += __details::real_size(storage);
    }

    template <std::derived_from <Command> _Command>
    void update(_Command &command, std::size_t count) {
        this->align_to(kCommand);
        this->size += kCommand * count;
    }
};

} // namespace dark
