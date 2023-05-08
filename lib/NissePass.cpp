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
#include "llvm/IR/IRBuilder.h"
#include <iostream>

using namespace llvm;
using namespace std;

namespace nisse {

Value *NissePass::insertEntryFn(Function &F) {
  IRBuilder<> builder(&F.getEntryBlock(), F.getEntryBlock().begin());
  auto *I = builder.getInt32Ty();
  ArrayType *arrayType = ArrayType::get(I, this->size);
  auto inst = builder.CreateAlloca(arrayType, nullptr, "counter-array");
  Value *indexList[] = {builder.getInt32(0)};
  auto cast = builder.CreateGEP(I, inst, indexList);
  auto zero = builder.getInt8(0);
  builder.CreateMemSet(cast, zero, this->size * 4, inst->getAlign());
  return inst;
}

void NissePass::insertIncrFn(Instruction *instruction, int i, Value *inst) {
  IRBuilder<> builder(instruction);
  auto *I = builder.getInt32Ty();
  Value *indexList[] = {builder.getInt32(i)};
  Value *one = builder.getInt32(1);
  auto inst1 = builder.CreateGEP(I, inst, indexList);
  auto inst2 = builder.CreateLoad(I, inst1);
  auto inst3 = builder.CreateAdd(inst2, one);
  builder.CreateStore(inst3, inst1);
}

void NissePass::insertExitFn(llvm::Function &F, llvm::Value *inst) {
  BlockPtr BB = NisseAnalysis::findReturnBlock(F);
  auto insertion_point = BB->getTerminator();
  IRBuilder<> builder(insertion_point);

  llvm::Type *r_type = builder.getVoidTy();

  llvm::SmallVector<llvm::Type *, 3> a_types;
  llvm::SmallVector<llvm::Value *, 3> a_vals;

  a_types.push_back(builder.getInt8PtrTy());
  llvm::StringRef function_name = insertion_point->getFunction()->getName();
  a_vals.push_back(builder.CreateGlobalStringPtr(function_name, "str"));

  a_types.push_back(inst->getType());
  a_vals.push_back(inst);

  a_types.push_back(builder.getInt32Ty());
  a_vals.push_back(builder.getInt32(this->size));

  llvm::FunctionType *f_type = llvm::FunctionType::get(r_type, a_types, false);
  llvm::Module *mod = insertion_point->getModule();
  llvm::FunctionCallee f_call = mod->getOrInsertFunction("print_data", f_type);
  builder.CreateCall(f_call, a_vals);
}

PreservedAnalyses NissePass::run(Function &F, FunctionAnalysisManager &FAM) {
  auto &edges = FAM.getResult<NisseAnalysis>(F);
  auto &reverseSTEdges = get<2>(edges);
  this->size = reverseSTEdges.size();

  auto inst = this->insertEntryFn(F);

  int i = 0;
  for (auto p : reverseSTEdges) {
    this->insertIncrFn(p.getInstrument(), i++, inst);
  }

  this->insertExitFn(F, inst);

  return PreservedAnalyses::all();
}

PreservedAnalyses NissePassPrint::run(Function &F,
                                      FunctionAnalysisManager &FAM) {
  auto &edges = FAM.getResult<NisseAnalysis>(F);
  auto &reverseSTEdges = get<2>(edges);
  this->size = reverseSTEdges.size();

  OS << "\n" << F.getName() << "\n\tEdges:\n";

  for (auto p : get<0>(edges)) {
    OS << "\t\t" << p << "\n";
  }

  OS << "\tSpanning Tree:\n";

  for (auto p : get<1>(edges)) {
    OS << "\t\t" << p << "\n";
  }

  OS << "\tInstrumented edges:\n";

  for (auto p : reverseSTEdges) {
    OS << "\t\t" << p << "\n";
  }

  auto inst = this->insertEntryFn(F);

  int i = 0;
  for (auto p : reverseSTEdges) {
    this->insertIncrFn(p.getInstrument(), i++, inst);
  }

  this->insertExitFn(F, inst);

  return PreservedAnalyses::all();
}

} // namespace nisse
