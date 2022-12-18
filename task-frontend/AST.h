#pragma once
#include "location.hh"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace AST {

using LocT = yy::location;
struct ValsT {
  std::unordered_map<std::string, llvm::Value *> LValues;
  std::unordered_map<std::string, llvm::Value *> RValues;
  void clear() {
    LValues.clear();
    RValues.clear();
  }
};

struct INode {
  virtual ~INode() = default;
};

struct Expr : public INode {
  LocT Loc;
  Expr() {}
  Expr(LocT L) : Loc(L) {}
  auto *addLocation(LocT L) {
    Loc = L;
    return this;
  }
  virtual llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                             llvm::IRBuilder<> &Builder,
                             ValsT &NamedValues) const = 0;
};

struct Empty : public Expr {
  Empty(LocT loc) : Expr(loc) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ExprFunc;

struct Scope : public Expr {
private:
  std::vector<Expr *> Blocks;

public:
  auto *addBlock(INode *B) {
    Blocks.emplace_back(static_cast<Expr *>(B));
    return this;
  }
  ~Scope() {
    for (auto *B : Blocks)
      delete B;
  }
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct While : public Expr {
private:
  std::unique_ptr<Expr> Condition;
  std::unique_ptr<Expr> Body;

public:
  While(LocT L, INode *C, INode *B)
      : Expr(L), Condition(static_cast<Expr *>(C)),
        Body(static_cast<Expr *>(B)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct If : public Expr {
private:
  std::unique_ptr<Expr> Condition;
  std::unique_ptr<Expr> Then;
  std::unique_ptr<Expr> Else;

public:
  If(LocT L, INode *C, INode *T, INode *E = nullptr)
      : Expr(L), Condition(static_cast<Expr *>(C)),
        Then(static_cast<Expr *>(T)), Else(static_cast<Expr *>(E)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct Return : public Expr {
private:
  std::unique_ptr<Expr> Value;

public:
  Return(LocT L, INode *V) : Expr(L), Value(static_cast<Expr *>(V)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ExprInt : public Expr {
private:
  llvm::APInt Value;

public:
  ExprInt(LocT L, llvm::APInt V) : Expr(L), Value(V) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ExprId : public Expr {
  std::string Name;
  ExprId(LocT L, std::string N) : Expr(L), Name(N) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ExprQmark : public Expr {
  std::string Name;
  ExprQmark(LocT L) : Expr(L) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};
struct ExprPrint : public Expr {
private:
  std::unique_ptr<Expr> Arg;

public:
  ExprPrint(LocT L, INode *A) : Expr(L), Arg(static_cast<Expr *>(A)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};
struct Let : public Expr {
private:
  std::unique_ptr<ExprId> Id;
  std::unique_ptr<Expr> Value;

public:
  Let(LocT L, INode *I, INode *E)
      : Expr(L), Id(static_cast<ExprId *>(I)), Value(static_cast<Expr *>(E)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ExprFunc : public Expr {
private:
  std::unique_ptr<Scope> Body;
  std::vector<ExprId *> ArgDecls;
  std::unique_ptr<ExprId> Id;

public:
  auto *addBody(INode *B) {
    Body.reset(static_cast<Scope *>(B));
    return this;
  }
  auto *addArgDecl(INode *D) {
    ArgDecls.emplace_back(static_cast<ExprId *>(D));
    return this;
  }
  auto *addId(INode *I) {
    Id.reset(static_cast<ExprId *>(I));
    return this;
  }
  ~ExprFunc() {
    for (auto *D : ArgDecls)
      delete D;
  }
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ExprApply : public Expr {
private:
  std::unique_ptr<ExprId> Id;
  std::vector<Expr *> Args;

public:
  auto *addId(INode *I) {
    Id.reset(static_cast<ExprId *>(I));
    return this;
  }
  auto *addArg(INode *A) {
    Args.push_back(static_cast<Expr *>(A));
    return this;
  }
  ~ExprApply() {
    for (auto *A : Args)
      delete A;
  }
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ExprAssign : public Expr {
private:
  std::unique_ptr<ExprId> Id;
  std::unique_ptr<Expr> Value;

public:
  ExprAssign(LocT L, INode *I, INode *V)
      : Expr(L), Id(static_cast<ExprId *>(I)), Value(static_cast<Expr *>(V)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

struct ASTModule : public Expr {
private:
  std::vector<ExprFunc *> Funcs;

public:
  auto *addFunction(INode *F) {
    Funcs.emplace_back(static_cast<ExprFunc *>(F));
    return this;
  }
  ~ASTModule() {
    for (auto *F : Funcs)
      delete F;
  }
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override;
};

template <typename OpTy> struct ExprBinOp : public Expr {
private:
  std::unique_ptr<Expr> LHS;
  std::unique_ptr<Expr> RHS;

public:
  ExprBinOp(LocT Loc, INode *L, INode *R)
      : Expr(Loc), LHS(static_cast<Expr *>(L)), RHS(static_cast<Expr *>(R)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder, ValsT &NamedValues) const {
    auto *L = LHS->genIR(Context, Module, Builder, NamedValues);
    auto *R = RHS->genIR(Context, Module, Builder, NamedValues);
    return OpTy::genIR(Builder, L, R);
  }
};

template <typename OpTy> struct ExprUnOp : public Expr {
private:
  std::unique_ptr<Expr> RHS;

public:
  ExprUnOp(LocT Loc, INode *R) : Expr(Loc), RHS(static_cast<Expr *>(R)) {}
  llvm::Value *genIR(llvm::LLVMContext &Context, llvm::Module &Module,
                     llvm::IRBuilder<> &Builder,
                     ValsT &NamedValues) const override {
    auto *R = RHS->genIR(Context, Module, Builder, NamedValues);
    return OpTy::genIR(Builder, R);
  }
};

struct BinOpMul {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateMul(LHS, RHS);
  }
};
struct BinOpDiv {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateSDiv(LHS, RHS);
  }
};
struct BinOpMod {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateSRem(LHS, RHS);
  }
};
struct BinOpPlus {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateAdd(LHS, RHS);
  }
};
struct BinOpMinus {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateSub(LHS, RHS);
  }
};
struct BinOpLess {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateICmpSLT(LHS, RHS);
  }
};
struct BinOpGrtr {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateICmpSGT(LHS, RHS);
  }
};
struct BinOpLessOrEq {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateICmpSLE(LHS, RHS);
  }
};
struct BinOpGrtrOrEq {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateICmpSGE(LHS, RHS);
  }
};
struct BinOpEqual {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateICmpEQ(LHS, RHS);
  }
};
struct BinOpNotEqual {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateICmpNE(LHS, RHS);
  }
};
struct BinOpAnd {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateAnd(LHS, RHS);
  }
};
struct BinOpOr {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder, llvm::Value *LHS,
                                   llvm::Value *RHS) {
    return Builder.CreateOr(LHS, RHS);
  }
};

struct UnOpPlus {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder,
                                   llvm::Value *RHS) {
    return RHS;
  }
};
struct UnOpMinus {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder,
                                   llvm::Value *RHS) {
    return Builder.CreateNeg(RHS);
  }
};
struct UnOpNot {
  static inline llvm::Value *genIR(llvm::IRBuilder<> &Builder,
                                   llvm::Value *RHS) {
    return Builder.CreateNot(RHS);
  }
};
} // namespace AST
