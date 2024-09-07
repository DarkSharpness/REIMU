#include "assembly/storage/static.h"
#include "libc/libc.h"
#include "linker/estimate.h"
#include "linker/linker.h"
#include "utility/error.h"

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
    void align_to(target_size_t alignment) {
        runtime_assert(std::has_single_bit(alignment));
        auto bitmask   = alignment - 1;
        auto current   = this->position;
        this->position = (current + bitmask) & ~bitmask;
    }

    explicit SizeEstimator(target_size_t start) : position(start) {}

    void estimate_section(Linker::_Details_Vec_t &vec) {
        for (auto &details : vec) {
            auto offsets     = details.get_offsets();
            offsets[0]       = 0;
            const auto start = this->get_position();
            details.set_start(start);
            target_size_t index = 0;
            for (auto &&[storage, _] : details) {
                this->visit(*storage);
                offsets[++index] = this->get_position() - start;
            }
        }
    }

private:
    target_size_t position;

    void visitStorage(ArithmeticReg &storage) override { this->update(storage, 1); }
    void visitStorage(ArithmeticImm &storage) override { this->update(storage, 1); }
    void visitStorage(LoadStore &storage) override { this->update(storage, 1); }
    void visitStorage(Branch &storage) override { this->update(storage, 1); }
    void visitStorage(JumpRelative &storage) override { this->update(storage, 1); }
    void visitStorage(JumpRegister &storage) override { this->update(storage, 1); }
    void visitStorage(CallFunction &storage) override { this->update(storage, 2); }
    void visitStorage(LoadImmediate &storage) override { this->update(storage, 2); }
    void visitStorage(LoadUpperImmediate &storage) override { this->update(storage, 1); }
    void visitStorage(AddUpperImmediatePC &storage) override { this->update(storage, 1); }
    void visitStorage(Alignment &storage) override { this->update(storage); }
    void visitStorage(IntegerData &storage) override { this->update(storage); }
    void visitStorage(ZeroBytes &storage) override { this->update(storage); }
    void visitStorage(ASCIZ &storage) override { this->update(storage); }

    template <std::derived_from<StaticData> _Data>
    void update(_Data &storage) {
        this->align_to(__details::align_size(storage));
        this->position += __details::real_size(storage);
    }

    template <std::derived_from<Command> _Command>
    void update(_Command &, int count) {
        this->align_to(alignof(command_size_t));
        this->position += sizeof(command_size_t) * count;
    }

    auto get_position() const -> decltype(this->position) { return this->position; }
};

/**
 * Make the estimate of the linker.
 *
 * This will estimate the size of the sections and
 * align each section to the page size.
 *
 * Currently, bss, rodata, and data sections are
 * merged to be in the same section, without any
 * separation.
 */
void Linker::make_estimate() {
    SizeEstimator estimator{libc::kLibcEnd};

    constexpr target_size_t kPageSize = 0x1000;

    estimator.estimate_section(this->get_section(Section::TEXT));

    estimator.align_to(kPageSize);

    estimator.estimate_section(this->get_section(Section::DATA));
    estimator.estimate_section(this->get_section(Section::RODATA));
    estimator.estimate_section(this->get_section(Section::UNKNOWN));
    estimator.estimate_section(this->get_section(Section::BSS));

    estimator.align_to(kPageSize);
}

} // namespace dark
