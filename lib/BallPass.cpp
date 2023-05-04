//===-- BallPass.cpp --------------------------------------------------===//
// Copyright (C) 2020  Luigi D. C. Soares, Augusto Dias Noronha
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
/// This file contains the implementation of the BallPass
///
//===----------------------------------------------------------------------===//

#include "Ball.h"
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

#include <set>
#include <utility>

using namespace llvm;
using namespace std;

namespace ball {

bool BallPass::inST(BlockPtr BB1, BlockPtr BB2) {
  PairBlock tmp = {BB1, BB2};
  return this->STEdges.count(tmp) == 1;
}

void BallPass::addToReverse(BlockPtr BB1, BlockPtr BB2) {
  if (!this->inST(BB1, BB2))
    return;
  for (BlockPtr BB : successors(BB1)) {
    if (BB == BB2) {
      this->reverseSTEdges.insert(PairBlock(BB1, BB2));
      return;
    }
  }
  this->reverseSTEdges.insert(PairBlock(BB2, BB1));
}

AllocaInst *BallPass::insertEntryFn(Function &F, Module &M) {
  auto *I = IntegerType::getInt32Ty(M.getContext());
  auto num = this->reverseSTEdges.size();
  ArrayType *arrayType = ArrayType::get(I, num);
  // Instruction *funcEntry =
  //     dyn_cast<Instruction>(F.getEntryBlock().getFirstInsertionPt());
  IRBuilder<> builder(&F.getEntryBlock(), F.getEntryBlock().begin());
  auto inst = builder.CreateAlloca(arrayType, nullptr, "counter-array");
  auto zero = Constant::getNullValue(I);
  builder.CreateMemSet(inst, zero, num, inst->getAlign());
  return inst;
}

void BallPass::insertIncrFn(BlockPtr BB1, BlockPtr BB2, int i,
                            AllocaInst *inst) {
  Instruction *target;
  if (BB1->getUniqueSuccessor()) {
    target = BB1->getTerminator();
  } else {
    target = &*BB2->getFirstInsertionPt();
  }
  auto I = inst->getType();
  IRBuilder<> builder(target);
  Value *indexList[1] = {ConstantInt::get(I, i)};
  Value *one = ConstantInt::get(I, 1);
  auto inst1 = builder.CreateGEP(I, inst, indexList);
  auto inst2 = builder.CreateLoad(I, inst1);
  auto inst3 = builder.CreateAdd(inst2, one);
  builder.CreateStore(inst3, inst1);
}

void BallPass::insertExitFn(BlockPtr BB, llvm::Function &F, llvm::Module &M, llvm::AllocaInst *inst) {
  auto insertion_point = BB->getTerminator();

  llvm::LLVMContext& context = insertion_point->getContext();

  IRBuilder<> builder(insertion_point);

  llvm::Type* r_type = llvm::FunctionType::getVoidTy(context);

  llvm::SmallVector<llvm::Type*, 3> a_types;
  llvm::SmallVector<llvm::Value*, 3> a_vals;

  a_types.push_back(builder.getInt8PtrTy());
  llvm::StringRef function_name = insertion_point->getFunction()->getName();
  a_vals.push_back(builder.CreateGlobalStringPtr(function_name, "str"));

  a_types.push_back(inst->getType());
  a_vals.push_back(inst);

  a_types.push_back(builder.getInt32Ty());
  a_vals.push_back(builder.getInt32(this->reverseSTEdges.size()));

  llvm::FunctionType* f_type = llvm::FunctionType::get(r_type, a_types, false);
  llvm::Module* mod = insertion_point->getFunction()->getParent();
  llvm::FunctionCallee f_call = mod->getOrInsertFunction("print_data", f_type);
  builder.CreateCall(f_call, a_vals);
}

PreservedAnalyses BallPass::run(Function &F, FunctionAnalysisManager &FAM) {
  auto &edges = FAM.getResult<BallAnalysis>(F);
  this->STEdges = edges;
  this->reverseSTEdges.clear();

  for (auto i = F.begin(); i != F.end(); ++i) {
    for (auto j = i; ++j != F.end(); /**/) {
      this->addToReverse(&*i, &*j);
    }
  }

  auto inst = this->insertEntryFn(F, *F.getParent());

  int i = 0;
  for (auto p : this->reverseSTEdges) {
    this->insertIncrFn(p.first, p.second, i++, inst);
  }

  for (BasicBlock &BB : F) {
    auto term = BB.getTerminator();
    if (isa<ReturnInst>(term)) {
      this->insertExitFn(&BB, F, *F.getParent(), inst);
    }
  }

  return PreservedAnalyses::all();
}
} // namespace ball
