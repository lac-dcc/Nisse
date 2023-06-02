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
#include "llvm/Support/CommandLine.h"
#include <iostream>

using namespace llvm;
using namespace std;

static cl::opt<bool>
    DisableProfilePrinting("nisse-disable-print", cl::init(false), cl::Hidden,
                           cl::desc("Disable Profile Printing"));

namespace nisse {

std::pair<llvm::Value *, llvm::Value *>
NissePass::insertEntryFn(Function &F, multiset<Edge> &reverseSTEdges) {
  int size = reverseSTEdges.size();
  IRBuilder<> builder(&F.getEntryBlock(), F.getEntryBlock().begin());
  auto *I = builder.getInt32Ty();
  ArrayType *arrayType = ArrayType::get(I, size);
  Value *indexList[] = {builder.getInt32(0)};
  auto zero = builder.getInt8(0);

  auto counterInst = builder.CreateAlloca(arrayType, nullptr, "counter-array");
  auto cast = builder.CreateGEP(I, counterInst, indexList);
  builder.CreateMemSet(cast, zero, size * 4, counterInst->getAlign());

  auto indexInst = builder.CreateAlloca(arrayType, nullptr, "index-array");
  int index = 0;
  for (auto e : reverseSTEdges) {
    auto indexCst = builder.getInt32(e.getIndex());
    Value *indexList[] = {builder.getInt32(index++)};

    auto cast = builder.CreateGEP(I, indexInst, indexList);
    builder.CreateStore(indexCst, cast);
  }

  return pair(counterInst, indexInst);
}

void NissePass::insertExitFn(llvm::Function &F, llvm::Value *counterInst,
                             llvm::Value *indexInst, int size) {
  BlockPtr BB = NisseAnalysis::findReturnBlock(F);
  auto insertion_point = BB->getTerminator();
  IRBuilder<> builder(insertion_point);

  llvm::Type *r_type = builder.getVoidTy();

  llvm::SmallVector<llvm::Type *, 5> a_types;
  llvm::SmallVector<llvm::Value *, 5> a_vals;

  a_types.push_back(builder.getInt8PtrTy());
  llvm::StringRef function_name = insertion_point->getFunction()->getName();
  a_vals.push_back(builder.CreateGlobalStringPtr(function_name, "str"));

  a_types.push_back(builder.getInt32Ty());
  a_vals.push_back(builder.getInt32(function_name.size()));

  a_types.push_back(counterInst->getType());
  a_vals.push_back(counterInst);

  a_types.push_back(indexInst->getType());
  a_vals.push_back(indexInst);

  a_types.push_back(builder.getInt32Ty());
  a_vals.push_back(builder.getInt32(size));

  llvm::FunctionType *f_type = llvm::FunctionType::get(r_type, a_types, false);
  llvm::Module *mod = insertion_point->getModule();
  llvm::FunctionCallee f_call = mod->getOrInsertFunction("print_data", f_type);
  builder.CreateCall(f_call, a_vals);
}

PreservedAnalyses NissePass::run(Function &F, FunctionAnalysisManager &FAM) {

  auto &edges = FAM.getResult<NisseAnalysis>(F);
  auto &reverseSTEdges = get<2>(edges);
  int size = reverseSTEdges.size();

  if (size == 1) {
    errs() << "Function '" << F.getName()
           << "' has only 1 edge to instrument. Skipping...\n";
    return PreservedAnalyses::all();
  }

  auto pInst = this->insertEntryFn(F, reverseSTEdges);
  auto counterInst = pInst.first;
  auto indexInst = pInst.second;

  int i = 0;
  for (auto p : reverseSTEdges) {
    p.insertIncrFn(i++, counterInst);
  }

  if (!DisableProfilePrinting)
    this->insertExitFn(F, counterInst, indexInst, size);

  return PreservedAnalyses::all();
}

PreservedAnalyses BallPass::run(Function &F, FunctionAnalysisManager &FAM) {

  auto &edges = FAM.getResult<BallAnalysis>(F);
  auto &reverseSTEdges = get<2>(edges);
  int size = reverseSTEdges.size();

  if (size == 1) {
    errs() << "Function '" << F.getName()
           << "' has only 1 edge to instrument. Skipping...\n";
    return PreservedAnalyses::all();
  }

  auto pInst = this->insertEntryFn(F, reverseSTEdges);
  auto counterInst = pInst.first;
  auto indexInst = pInst.second;

  int i = 0;
  for (auto p : reverseSTEdges) {
    p.insertIncrFn(i++, counterInst);
  }

  if (!DisableProfilePrinting)
    this->insertExitFn(F, counterInst, indexInst, size);

  return PreservedAnalyses::all();
}

} // namespace nisse
