#pragma once
#include "assembly/forward.h"

namespace dark {

struct ArithmeticReg;
struct ArithmeticImm;
struct LoadStore;
struct Branch;
struct JumpRelative;
struct JumpRegister;
struct CallFunction;
struct LoadImmediate;
struct LoadUpperImmediate;
struct AddUpperImmediatePC;

struct Alignment;
struct IntegerData;
struct ZeroBytes;
struct ASCIZ;

struct StorageVisitor {
    virtual ~StorageVisitor() = default;
    void visit(Storage &storage) { return storage.accept(*this); }
    virtual void visitStorage(ArithmeticReg &storage) = 0;
    virtual void visitStorage(ArithmeticImm &storage) = 0;
    virtual void visitStorage(LoadStore &storage) = 0;
    virtual void visitStorage(Branch &storage) = 0;
    virtual void visitStorage(JumpRelative &storage) = 0;
    virtual void visitStorage(JumpRegister &storage) = 0;
    virtual void visitStorage(CallFunction &storage) = 0;
    virtual void visitStorage(LoadImmediate &storage) = 0;
    virtual void visitStorage(LoadUpperImmediate &storage) = 0;
    virtual void visitStorage(AddUpperImmediatePC &storage) = 0;
    virtual void visitStorage(Alignment &storage) = 0;
    virtual void visitStorage(IntegerData &storage) = 0;
    virtual void visitStorage(ZeroBytes &storage) = 0;
    virtual void visitStorage(ASCIZ &storage) = 0;
};

} // namespace dark