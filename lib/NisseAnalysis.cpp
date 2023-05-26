//===-- NisseAnalysis.cpp
//--------------------------------------------------===//
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
/// This file contains the implementation of the NisseAnalysis
///
//===----------------------------------------------------------------------===//

#include "Nisse.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include <fstream>
#include <regex>

using namespace llvm;
using namespace std;

namespace nisse {

BlockPtr NisseAnalysis::findReturnBlock(llvm::Function &F) {
  for (llvm::BasicBlock &BB : F) {
    auto term = BB.getTerminator();
    if (llvm::isa<llvm::ReturnInst>(term)) {
      return &BB;
    }
  }
  return nullptr;
}

string NisseAnalysis::removebb(const std::string &s) {
  string sub = regex_replace(s, regex(R"([\D])"), "");
  if (sub.size() > 0) {
    if (regex_match(s, regex(".*crit.*"))) {
      return "-" + sub;
    }
    return sub;
  }
  return "0";
}

// Initialize the analysis key.
AnalysisKey NisseAnalysis::Key;

multiset<Edge> NisseAnalysis::generateEdges(Function &F) {
  multiset<Edge> edges;
  int index = 0;
  for (auto &BB : F) {
    for (auto Succ : successors(&BB)) {
      edges.insert(Edge(&BB, Succ, index++));
    }
  }
  edges.insert(Edge(findReturnBlock(F), &F.getEntryBlock(), index++, 0));
  return edges;
}

pair<multiset<Edge>, multiset<Edge>>
NisseAnalysis::generateSTrev(Function &F, multiset<Edge> &edges) {
  multiset<Edge> ST;
  multiset<Edge> rev;
  UnionFind uf;
  for (auto &BB : F) {
    uf.init(&BB);
  }
  multiset<Edge, greater<Edge>> revEdges(edges.begin(), edges.end());
  for (auto e : revEdges) {
    auto BB1 = e.getOrigin();
    auto BB2 = e.getDest();
    if (!uf.connected(BB1, BB2)) {
      ST.insert(e);
      uf.merge(BB1, BB2);
    } else {
      rev.insert(e);
    }
  }
  return pair(ST, rev);
}

void NisseAnalysis::identifyInductionVariables(Loop *L, ScalarEvolution &SE,
                                               multiset<Edge> &edges) {
  BlockPtr incomingBlock, backBlock;
  L->getIncomingAndBackEdge(incomingBlock, backBlock);
  SmallVector<BlockPtr> exitBlocks;
  L->getExitBlocks(exitBlocks);
  auto firstBlock = incomingBlock->getSingleSuccessor();
  Edge backEdge(backBlock, firstBlock, -1);

  for (auto &Inst : *firstBlock) {
    if (auto *PHI = dyn_cast<llvm::PHINode>(&Inst)) {
      const llvm::SCEV *SCEV = SE.getSCEV(PHI);
      if (SE.containsAddRecurrence(SCEV)) {
        const llvm::SCEVAddRecExpr *AddRecExpr =
            cast<llvm::SCEVAddRecExpr>(SCEV);
        if (AddRecExpr->isAffine()) {
          // Affine induction variable found
          llvm::Value *IndVar = PHI;
          const llvm::SCEV *IncrementSCEV = AddRecExpr->getStepRecurrence(SE);
          const llvm::SCEVConstant *IncrementSCEVCst =
              dyn_cast<llvm::SCEVConstant>(IncrementSCEV);
          if (!IncrementSCEVCst)
            break;
          const llvm::APInt *IncrementValue =
              &IncrementSCEVCst->getValue()->getValue();
          auto val = PHI->getIncomingValueForBlock(incomingBlock);
          for (auto &e : edges) {
            if (e == backEdge) {
              edges.erase(e);
              Edge new_e = e;
              new_e.setWellFoundedValues(IndVar, val, IncrementValue,
                                         exitBlocks);
              edges.insert(new_e);
              return;
            }
          }
        }
      }
    }
  }
}

void NisseAnalysis::printGraph(
    llvm::Function &F, multiset<Edge> &edges,
    std::pair<std::multiset<Edge>, std::multiset<Edge>> &STrev) {

  string fileName = F.getName().str() + ".graph";

  errs() << "Writing '" << fileName << "'...\n";

  int blockCount = distance(F.begin(), F.end());

  ofstream file;
  file.open(fileName, ios::out | ios::trunc);

  file << blockCount;
  for (auto &BB : F) {
    file << ' ' << NisseAnalysis::removebb(BB.getName().str());
  }
  file << endl;

  file << edges.size();
  for (auto e : edges) {
    file << '\t' << e << '\n';
  }

  file << STrev.first.size();
  for (auto e : STrev.first) {
    file << ' ' << e.getIndex();
  }
  file << endl;

  file << STrev.second.size();
  for (auto e : STrev.second) {
    file << ' ' << e.getIndex();
  }

  file.close();
}

NisseAnalysis::Result NisseAnalysis::run(llvm::Function &F,
                                         llvm::FunctionAnalysisManager &FAM) {

  auto edges = this->generateEdges(F);

  llvm::ScalarEvolution &SE = FAM.getResult<llvm::ScalarEvolutionAnalysis>(F);

  DominatorTree DT(F);
  LoopInfo LI(DT);
  auto loops = LI.getLoopsInPreorder();
  for (auto loop : loops) {
    identifyInductionVariables(loop, SE, edges);
  }

  auto STrev = this->generateSTrev(F, edges);

  printGraph(F, edges, STrev);

  return make_tuple(edges, STrev.first, STrev.second);
}
} // namespace nisse