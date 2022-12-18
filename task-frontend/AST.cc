#include "AST.h"
#include <cassert>

using namespace llvm;
namespace {
auto *getIntTy(LLVMContext &Context) {
  return Type::getIntNTy(Context, sizeof(int) * 8);
}
void createPrintFunctionDef(LLVMContext &Context, Module &Module) {
  auto *IntTy = getIntTy(Context);
  std::vector<Type *> ArgTys{1, IntTy};
  auto *FT = FunctionType::get(IntTy, ArgTys, false);
  Function::Create(FT, Function::ExternalLinkage, "__print", Module);
}

void createQmarkFunctionDef(LLVMContext &Context, Module &Module) {
  auto *IntTy = getIntTy(Context);
  std::vector<Type *> NoArgs{};
  auto *FT = FunctionType::get(IntTy, NoArgs, false);
  Function::Create(FT, Function::ExternalLinkage, "__qmark", Module);
}

auto *getPredFromInt(LLVMContext &Context, IRBuilder<> &Builder,
                     Value *Condition) {
  return Builder.CreateICmpNE(Condition,
                              ConstantInt::getSigned(Condition->getType(), 0));
}
} // namespace
namespace AST {

Value *Empty::genIR(LLVMContext &Context, Module &Module, IRBuilder<> &Builder,
                    ValsT &NamedValues) const {
  return Builder.CreateNot(ConstantInt::getFalse(Context), "nop");
}

Value *ASTModule::genIR(LLVMContext &Context, Module &Module,
                        IRBuilder<> &Builder, ValsT &NamedValues) const {
  createPrintFunctionDef(Context, Module);
  createQmarkFunctionDef(Context, Module);
  for (auto *F : Funcs)
    F->genIR(Context, Module, Builder, NamedValues);
  return nullptr;
}

Value *Scope::genIR(LLVMContext &Context, Module &Module, IRBuilder<> &Builder,
                    ValsT &NamedValues) const {
  for (auto *Block : Blocks)
    Block->genIR(Context, Module, Builder, NamedValues);
  return nullptr;
}
Value *While::genIR(LLVMContext &Context, Module &Module, IRBuilder<> &Builder,
                    ValsT &NamedValues) const {
  auto *Function = Builder.GetInsertBlock()->getParent();
  auto *HeaderBB = BasicBlock::Create(Context, "while.condition", Function);
  Builder.CreateBr(HeaderBB);
  auto *BodyBB = BasicBlock::Create(Context, "while.body", Function);
  auto *ExitBB = BasicBlock::Create(Context, "while.exit", Function);
  Builder.SetInsertPoint(HeaderBB);
  auto *Pred =
      getPredFromInt(Context, Builder,
                     Condition->genIR(Context, Module, Builder, NamedValues));
  Builder.CreateCondBr(Pred, BodyBB, ExitBB);
  Builder.SetInsertPoint(BodyBB);
  Body->genIR(Context, Module, Builder, NamedValues);
  Builder.CreateBr(HeaderBB);
  Builder.SetInsertPoint(ExitBB);
  return nullptr;
}

Value *If::genIR(LLVMContext &Context, Module &Module, IRBuilder<> &Builder,
                 ValsT &NamedValues) const {
  auto *Function = Builder.GetInsertBlock()->getParent();
  auto *HeaderBB = BasicBlock::Create(Context, "if.condition", Function);
  Builder.CreateBr(HeaderBB);
  Builder.SetInsertPoint(HeaderBB);
  auto *ThenBB = BasicBlock::Create(Context, "if.then", Function);
  auto *ExitBB = BasicBlock::Create(Context, "if.exit", Function);
  auto *ElseBB = ExitBB;
  auto *Pred =
      getPredFromInt(Context, Builder,
                     Condition->genIR(Context, Module, Builder, NamedValues));

  Builder.SetInsertPoint(ThenBB);
  Then->genIR(Context, Module, Builder, NamedValues);
  Builder.CreateBr(ExitBB);

  if (Else) {
    ElseBB = BasicBlock::Create(Context, "if.else", Function);
    Builder.SetInsertPoint(ElseBB);
    Else->genIR(Context, Module, Builder, NamedValues);
    Builder.CreateBr(ExitBB);
  }

  Builder.SetInsertPoint(HeaderBB);
  Builder.CreateCondBr(Pred, ThenBB, ElseBB);
  Builder.SetInsertPoint(ExitBB);
  return nullptr;
}

Value *ExprAssign::genIR(LLVMContext &Context, Module &Module,
                         IRBuilder<> &Builder, ValsT &NamedValues) const {
  assert(NamedValues.LValues.find(Id->Name) != NamedValues.LValues.end());
  auto *Addr = NamedValues.LValues[Id->Name];
  auto *Res = Value->genIR(Context, Module, Builder, NamedValues);
  Builder.CreateStore(Res, Addr);
  return Res;
}

Value *ExprInt::genIR(LLVMContext &Context, Module &Module,
                      IRBuilder<> &Builder, ValsT &NamedValues) const {
  return ConstantInt::get(Context, Value);
}

Value *ExprId::genIR(LLVMContext &Context, Module &Module, IRBuilder<> &Builder,
                     ValsT &NamedValues) const {
  auto RVIt = NamedValues.RValues.find(Name);
  if (RVIt != NamedValues.RValues.end())
    return RVIt->second;
  auto LVIt = NamedValues.LValues.find(Name);
  assert(LVIt != NamedValues.LValues.end());
  return Builder.CreateLoad(getIntTy(Context), LVIt->second, Name);
}

Value *Let::genIR(LLVMContext &Context, Module &Module, IRBuilder<> &Builder,
                  ValsT &NamedValues) const {
  assert(NamedValues.LValues.find(Id->Name) == NamedValues.LValues.end() &&
         NamedValues.RValues.find(Id->Name) == NamedValues.RValues.end());
  auto *Addr = NamedValues.LValues[Id->Name] =
      Builder.CreateAlloca(getIntTy(Context));
  auto *Res = Value->genIR(Context, Module, Builder, NamedValues);
  Builder.CreateStore(Res, Addr);
  return nullptr;
}

Value *Return::genIR(LLVMContext &Context, Module &Module, IRBuilder<> &Builder,
                     ValsT &NamedValues) const {
  auto *RetVal = Value->genIR(Context, Module, Builder, NamedValues);
  auto *Res = Builder.CreateRet(RetVal);
  auto *Function = Builder.GetInsertBlock()->getParent();
  auto *BB = BasicBlock::Create(Context, "unreachable", Function);
  Builder.SetInsertPoint(BB);
  return Res;
}

Value *ExprFunc::genIR(LLVMContext &Context, Module &Module,
                       IRBuilder<> &Builder, ValsT &NamedValues) const {
  auto *IntTy = getIntTy(Context);
  std::vector<Type *> ArgTys{ArgDecls.size(), IntTy};
  auto *FT = FunctionType::get(IntTy, ArgTys, false);
  auto *Function =
      Function::Create(FT, Function::ExternalLinkage, Id->Name, Module);
  unsigned Idx = 0;
  for (auto &Arg : Function->args())
    Arg.setName(ArgDecls[Idx++]->Name);
  auto *BB = BasicBlock::Create(Context, "entry", Function);
  Builder.SetInsertPoint(BB);
  NamedValues.clear();
  for (auto &Arg : Function->args())
    NamedValues.RValues[Arg.getName().str()] = &Arg;

  Body->genIR(Context, Module, Builder, NamedValues);
  Builder.CreateRet(ConstantInt::getSigned(getIntTy(Context), 0));

  verifyFunction(*Function);
  return Function;
}

Value *ExprApply::genIR(LLVMContext &Context, Module &Module,
                        IRBuilder<> &Builder, ValsT &NamedValues) const {
  auto *Callee = Module.getFunction(Id->Name);
  assert(Callee && (Callee->arg_size() == Args.size()));

  std::vector<Value *> ArgsV;
  for (auto Arg : Args) {
    ArgsV.push_back(Arg->genIR(Context, Module, Builder, NamedValues));
  }

  return Builder.CreateCall(Callee, ArgsV, "calltmp");
}
Value *ExprQmark::genIR(LLVMContext &Context, Module &Module,
                        IRBuilder<> &Builder, ValsT &NamedValues) const {
  auto *Callee = Module.getFunction("__qmark");
  std::vector<Value *> ArgsV{};
  return Builder.CreateCall(Callee, ArgsV, "calltmp");
}
Value *ExprPrint::genIR(LLVMContext &Context, Module &Module,
                        IRBuilder<> &Builder, ValsT &NamedValues) const {
  auto *Callee = Module.getFunction("__print");
  std::vector<Value *> ArgsV{Arg->genIR(Context, Module, Builder, NamedValues)};
  return Builder.CreateCall(Callee, ArgsV, "calltmp");
}
} // namespace AST
