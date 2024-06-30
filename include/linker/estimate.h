// Should only be included once by linker.cpp
#include "linker.h"
#include "helper.h"
#include <utility.h>
#include <storage.h>
#include <ustring.h>

namespace dark {

/**
 * A pessimistic size estimator for the linker.
 * It will give the wurst case for the size of the section.
 * 
 * e.g: a call may take at most 2 commands, but can be optimized
 * to 1 command if the jump target is near enough.
 * 
 */
struct SizeEstimator final : StorageVisitor {
    static constexpr std::size_t kCommand = 4;

    void align_to(std::size_t alignment) {
        runtime_assert(std::has_single_bit(alignment));
        std::size_t bitmask = alignment - 1;
        std::size_t current = this->position;
        this->position = (current + bitmask) & ~bitmask;
    }

    explicit SizeEstimator(std::size_t start) : position(start) {}

    void estimate_section(Linker::_Details_Vec_t &vec) {
        for (auto &details : vec) {
            auto offsets = details.get_offsets();
            offsets[0] = 0;
            const auto start = this->get_position();
            details.set_start(start);
            std::size_t index = 0;
            for (auto &&[storage, _] : details) {
                this->visit(*storage);
                offsets[++index] = this->get_position() - start;
            }
        }
    }

  private:
    std::size_t position;

    void visitStorage(ArithmeticReg &storage)       override { this->update(storage, 1); }
    void visitStorage(ArithmeticImm &storage)       override { this->update(storage, 1); }
    void visitStorage(LoadStore &storage)           override { this->update(storage, 1); }
    void visitStorage(Branch &storage)              override { this->update(storage, 1); }
    void visitStorage(JumpRelative &storage)        override { this->update(storage, 1); }
    void visitStorage(JumpRegister &storage)        override { this->update(storage, 1); }
    void visitStorage(CallFunction &storage)        override { this->update(storage, 2); }
    void visitStorage(LoadImmediate &storage)       override { this->update(storage, 2); }
    void visitStorage(LoadUpperImmediate &storage)  override { this->update(storage, 1); }
    void visitStorage(AddUpperImmediatePC &storage) override { this->update(storage, 1); }
    void visitStorage(Alignment &storage)           override { this->update(storage); }
    void visitStorage(IntegerData &storage)         override { this->update(storage); }
    void visitStorage(ZeroBytes &storage)           override { this->update(storage); }
    void visitStorage(ASCIZ &storage)               override { this->update(storage); }

    template <std::derived_from <RealData> _Data>
    void update(_Data &storage) {
        this->align_to(__details::align_size(storage));
        this->position += __details::real_size(storage);
    }

    template <std::derived_from <Command> _Command>
    void update(_Command &command, std::size_t count) {
        this->align_to(kCommand);
        this->position += kCommand * count;
    }

    std::size_t get_position() const { return this->position; }
};

} // namespace dark