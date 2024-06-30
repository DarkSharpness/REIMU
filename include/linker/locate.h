// Should only be included in linker.cpp
#include "evaluate.h"
#include "linker.h"

namespace dark {

struct EvaluationPass final : Evaluator, StorageVisitor {
  public:
    explicit EvaluationPass(const _Table_t &global_table, const Linker::_Details_Vec_t &vec)
        : Evaluator(global_table) {
        for (auto &details : vec) {
            this->set_local(details.get_local_table());
            for (auto &&[storage, position] : details) {
                this->set_position(position);
                this->visit(*storage);
            }
        }
    }

    void evaluate(Immediate &imm) {
        auto value = Evaluator::evaluate(*imm.data);
        imm.data = std::make_unique <IntImmediate> (value);
    }

  private:

    void visitStorage(ArithmeticReg &storage)       override {}
    void visitStorage(ArithmeticImm &storage)       override { this->evaluate(storage.imm); }
    void visitStorage(LoadStore &storage)           override { this->evaluate(storage.imm); }
    void visitStorage(Branch &storage)              override { this->evaluate(storage.imm); }
    void visitStorage(JumpRelative &storage)        override { this->evaluate(storage.imm); }
    void visitStorage(JumpRegister &storage)        override { this->evaluate(storage.imm); }
    void visitStorage(CallFunction &storage)        override { this->evaluate(storage.imm); }
    void visitStorage(LoadImmediate &storage)       override { this->evaluate(storage.imm); }
    void visitStorage(LoadUpperImmediate &storage)  override { this->evaluate(storage.imm); }
    void visitStorage(AddUpperImmediatePC &storage) override { this->evaluate(storage.imm); }
    void visitStorage(Alignment &storage)           override {}
    void visitStorage(IntegerData &storage)         override { this->evaluate(storage.data); }
    void visitStorage(ZeroBytes &storage)           override {}
    void visitStorage(ASCIZ &storage)               override {}
};


} // namespace dark
