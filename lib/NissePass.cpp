//===-- NissePass.cpp --------------------------------------------------===//
// Copyright (C) 2023 Leon Frenot
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation of the NissePass
///
//===----------------------------------------------------------------------===//

#include "Nisse.h"
#include "llvm-c/Core.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"

#include <set>
#include <utility>
#include <iostream>

using namespace llvm;
using namespace std;

namespace nisse {

Value *NissePass::insertEntryFn(Function &F, Module &M) {
  auto *I = IntegerType::getInt32Ty(M.getContext());
  auto num = this->reverseSTEdges.size();
  ArrayType *arrayType = ArrayType::get(I, num);
  // Instruction *funcEntry =
  //     dyn_cast<Instruction>(F.getEntryBlock().getFirstInsertionPt());
  IRBuilder<> builder(&F.getEntryBlock(), F.getEntryBlock().begin());
  auto inst = builder.CreateAlloca(arrayType, nullptr, "counter-array");
  Value *indexList[] = {ConstantInt::get(I, 0)};
  auto cast = builder.CreateGEP(I, inst, indexList);
  auto zero = ConstantInt::get(IntegerType::getInt8Ty(M.getContext()), 0);
  builder.CreateMemSet(cast, zero, num * 4, inst->getAlign());
  return inst;
}

void NissePass::insertIncrFn(Module &M, Instruction *instruction, int i,
                            Value *inst) {
  auto *I = IntegerType::getInt32Ty(M.getContext());
  IRBuilder<> builder(instruction);
  Value *indexList[1] = {ConstantInt::get(I, i)};
  Value *one = ConstantInt::get(I, 1);
  auto inst1 = builder.CreateGEP(I, inst, indexList);
  auto inst2 = builder.CreateLoad(I, inst1);
  auto inst3 = builder.CreateAdd(inst2, one);
  builder.CreateStore(inst3, inst1);
}

void NissePass::insertExitFn(BlockPtr BB, llvm::Function &F, llvm::Module &M,
                            llvm::Value *inst) {
  auto insertion_point = BB->getTerminator();

  llvm::LLVMContext &context = insertion_point->getContext();

  IRBuilder<> builder(insertion_point);

  llvm::Type *r_type = llvm::FunctionType::getVoidTy(context);

  llvm::SmallVector<llvm::Type *, 3> a_types;
  llvm::SmallVector<llvm::Value *, 3> a_vals;

  a_types.push_back(builder.getInt8PtrTy());
  llvm::StringRef function_name = insertion_point->getFunction()->getName();
  a_vals.push_back(builder.CreateGlobalStringPtr(function_name, "str"));

  a_types.push_back(inst->getType());
  a_vals.push_back(inst);

  a_types.push_back(builder.getInt32Ty());
  a_vals.push_back(builder.getInt32(this->reverseSTEdges.size()));

  llvm::FunctionType *f_type = llvm::FunctionType::get(r_type, a_types, false);
  llvm::Module *mod = insertion_point->getFunction()->getParent();
  llvm::FunctionCallee f_call = mod->getOrInsertFunction("print_data", f_type);
  builder.CreateCall(f_call, a_vals);
}

PreservedAnalyses NissePass::run(Function &F, FunctionAnalysisManager &FAM) {
  auto &edges = FAM.getResult<NisseAnalysis>(F);
  this->reverseSTEdges = edges.second.second;

  auto inst = this->insertEntryFn(F, *F.getParent());

  int i = 0;
  for (auto p : this->reverseSTEdges) {
    this->insertIncrFn(*F.getParent(), p.getInstrument(), i++, inst);
  }

  this->insertExitFn(NisseAnalysis::findExitBlock(F), F, *F.getParent(), inst);

  return PreservedAnalyses::all();
}

PreservedAnalyses NissePassPrint::run(Function &F, FunctionAnalysisManager &FAM) {
  auto &edges = FAM.getResult<NisseAnalysis>(F);
  this->reverseSTEdges = edges.second.second;

  OS << "\n" << F.getName() << "\n\tEdges:\n";

  for (auto p:edges.first) {
    OS << "\t\t" << p.getName() << "\n";
  }

  OS << "\tSpanning Tree:\n";

  for (auto p:edges.second.first) {
    OS << "\t\t" << p.getName() << "\n";
  }

  OS << "\tReverse:\n";

  for (auto p:this->reverseSTEdges) {
    OS << "\t\t" << p.getName() << "\n";
  }

  auto inst = this->insertEntryFn(F, *F.getParent());

  int i = 0;
  for (auto p : this->reverseSTEdges) {
    this->insertIncrFn(*F.getParent(), p.getInstrument(), i++, inst);
  }

  this->insertExitFn(NisseAnalysis::findExitBlock(F), F, *F.getParent(), inst);

  return PreservedAnalyses::all();
}

} // namespace nisse
