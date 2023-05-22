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
#include "llvm/IR/Dominators.h"
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
  if (sub.size() > 0)
    return sub;
  return "0";
}

// Initialize the analysis key.
AnalysisKey NisseAnalysis::Key;

SmallVector<Edge> NisseAnalysis::generateEdges(Function &F) {
  SmallVector<Edge> edges;
  int index = 0;
  for (auto &BB : F) {
    for (auto Succ : successors(&BB)) {
      edges.push_back(Edge(&BB, Succ, index++));
    }
  }
  edges.push_back(Edge(findReturnBlock(F), &F.getEntryBlock(), index++, 0));
  return edges;
}

pair<multiset<Edge>, multiset<Edge>>
NisseAnalysis::generateSTrev(Function &F, SmallVector<Edge> &edges) {
  multiset<Edge> ST;
  multiset<Edge> rev;
  UnionFind uf;
  for (auto &BB : F) {
    uf.init(&BB);
  }

  llvm::sort(edges.begin(), edges.end(), Edge::compareWeights);
  for (auto e : edges) {
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
                                               SmallVector<Edge> edges) const {
  for (auto *BB : L->getBlocks()) {
    for (auto &Inst : *BB) {
      if (auto *PHI = dyn_cast<llvm::PHINode>(&Inst)) {
        const llvm::SCEV *SCEV = SE.getSCEV(PHI);
        if (SE.containsAddRecurrence(SCEV)) {
          const llvm::SCEVAddRecExpr *AddRecExpr =
              cast<llvm::SCEVAddRecExpr>(SCEV);
          if (AddRecExpr->isAffine()) {
            // Affine induction variable found
            llvm::Value *IndVar = PHI;
            const llvm::SCEV *IncrementSCEV = AddRecExpr->getStepRecurrence(SE);
            const llvm::APInt *IncrementValue =
                &cast<llvm::SCEVConstant>(IncrementSCEV)
                     ->getValue()
                     ->getValue();
            BlockPtr incomingBlock, backBlock;
            L->getIncomingAndBackEdge(incomingBlock, backBlock);
            BlockPtr exitBlock = L->getExitBlock();
            Edge tmpEdge(backBlock, incomingBlock, -1);
            for (auto e : edges) {
              if (e == tmpEdge) {
                errs() << "loop instrumenting\n";
                e.setWeight(0);
                e.setWellFoundedValues(
                    IndVar, &*exitBlock->getFirstInsertionPt(), IncrementValue);
                break;
              }
            }
            llvm::errs() << "Induction Variable: " << *IndVar << "\n";
            llvm::errs() << "Increment Value: " << IncrementValue << "\n";
          }
        }
      }
    }
  }
}

NisseAnalysis::Result NisseAnalysis::run(llvm::Function &F,
                                         llvm::FunctionAnalysisManager &FAM) {

  auto edges = this->generateEdges(F);

  DominatorTree DT(F);
  LoopInfo LI(DT);
  auto loops = LI.getLoopsInPreorder();

  llvm::ScalarEvolution &SE = FAM.getResult<llvm::ScalarEvolutionAnalysis>(F);
  // auto &SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  errs() << loops.size() << "\n";

  for (auto loop : loops) {
    identifyInductionVariables(loop, SE, edges);
  }

  auto STrev = this->generateSTrev(F, edges);

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

  return make_tuple(edges, STrev.first, STrev.second);
}
} // namespace nisse