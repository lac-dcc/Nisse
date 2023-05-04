//===-- BallAnalysis.cpp --------------------------------------------------===//
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
/// This file contains the implementation of the BallAnalysis
///
//===----------------------------------------------------------------------===//

#include "Ball.h"
#include "llvm-c/Core.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include <set>
#include <utility>

using namespace llvm;
using namespace std;

namespace ball {

// Initialize the analysis key.
AnalysisKey BallAnalysis::Key;

void BallAnalysis::insertBlock(BlockPtr BB, BlockPtr Next,
                               set<PairBlock> &STEdges, set<BlockPtr> &STVertex,
                               SmallVector<BlockPtr> &queue) {
  if (STVertex.insert(Next).second) {
    STEdges.insert(PairBlock(BB, Next));
    STEdges.insert(PairBlock(Next, BB));
    queue.push_back(Next);
  }
}

BallAnalysis::Result BallAnalysis::run(llvm::Function &F,
                                       llvm::FunctionAnalysisManager &FAM) {
  set<PairBlock> STEdges;
  set<BlockPtr> STVertex;
  SmallVector<BlockPtr> queue;

  BlockPtr FB = &F.getEntryBlock();
  queue.push_back(FB);
  STVertex.insert(FB);
  while (!queue.empty()) {
    const auto BB = queue.pop_back_val();
    for (auto Succ : successors(BB)) {
      this->insertBlock(BB, Succ, STEdges, STVertex, queue);
    }
    // for (auto Pred : predecessors(BB)) {
    //   this->insertBlock(BB, Pred, STEdges, STVertex, queue);
    // }
  }

  return STEdges;
}
} // namespace ball