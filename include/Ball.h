//===-- AddConst.h ----------------------------------------------*- C++ -*-===//
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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//===----------------------------------------------------------------------===//
///
/// \file
///
//===----------------------------------------------------------------------===//

#ifndef BALL_H
#define BALL_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include <set>
#include <utility>

namespace ball {
using BlockPtr = llvm::BasicBlock *;
using PairBlock = std::pair<BlockPtr, BlockPtr>;

struct BallAnalysis : public llvm::AnalysisInfoMixin<BallAnalysis> {

  using Result = std::set<PairBlock>;

private:
  void insertBlock(BlockPtr BB, BlockPtr Next, std::set<PairBlock> &STEdges,
                   std::set<BlockPtr> &STVertex,
                   llvm::SmallVector<BlockPtr> &queue);

public:
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
  // A special type used by analysis passes to provide an address that
  // identifies that particular analysis pass type.
  static llvm::AnalysisKey Key;
};

struct BallPass : public llvm::PassInfoMixin<BallPass> {

private:
  std::set<PairBlock> STEdges;
  std::set<PairBlock> reverseSTEdges;

  void addToReverse(BlockPtr BB1, BlockPtr BB2);
  bool inST(BlockPtr BB1, BlockPtr BB2);

  llvm::AllocaInst *insertEntryFn(llvm::Function &F, llvm::Module &M);
  void insertIncrFn(BlockPtr BB1, BlockPtr BB2, int i, llvm::AllocaInst *inst);
  void insertExitFn(BlockPtr BB, llvm::Function &F, llvm::Module &M,
                    llvm::AllocaInst *inst);

public:
  BallPass();
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};

} // namespace ball

#endif