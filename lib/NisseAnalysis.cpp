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
#include "llvm/ADT/Statistic.h"
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
#include <queue>
#include <regex>

#define DEBUG_TYPE "nisse" // This goes after any #includes.
STATISTIC(NumCounters, "The # of counters");
STATISTIC(SESECounters, "The # of SESE counters found");
STATISTIC(SESEUsed, "The # of SESE counters used");

using namespace llvm;
using namespace std;

namespace nisse {

BlockPtr AnalysisUtil::findReturnBlock(Function &F) {
  UnreachableInst *unreach = nullptr;
  for (BasicBlock &BB : F) {
    auto term = BB.getTerminator();
    if (isa<ReturnInst>(term)) {
      return &BB;
    }
    if (auto *RI = dyn_cast<UnreachableInst>(term))
      unreach = RI;
  }
  return unreach->getParent();
}

string AnalysisUtil::removebb(const string &s) {
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

multiset<Edge> AnalysisUtil::generateEdges(Function &F) {
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
AnalysisUtil::generateSTrev(Function &F, multiset<Edge> &edges) {
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
      NumCounters++;
      if (e.isSESE()) {
        SESEUsed++;
      }
      rev.insert(e);
    }
  }
  return pair(ST, rev);
}

bool NisseAnalysis::IsSESERegion(const BlockPtr &B1, const BlockPtr &B2) {
  if (!((DT.dominates(B1, B2) && PDT.dominates(B2, B1)) ||
        (PDT.dominates(B1, B2) && DT.dominates(B2, B1))))
    return false;
  if (CI.getCycle(B1) != CI.getCycle(B2))
    return false;
  return true;
}

void NisseAnalysis::initFunctionInfo(Function &F,
                                     FunctionAnalysisManager &FAM) {
  this->SE = &FAM.getResult<ScalarEvolutionAnalysis>(F);
  this->DT.recalculate(F);
  this->PDT.recalculate(F);
  this->LI.analyze(DT);
  this->CI.compute(F);
}

bool NisseAnalysis::identifyInductionVariable(
    ScalarEvolution &SE, multiset<Edge> &edges, PHINode *PHI,
    BlockPtr incomingBlock, BlockPtr backBlock, Edge &backEdge,
    SmallVector<BlockPtr> &exitBlocks) {
  if (!SE.isSCEVable(PHI->getType()))
    return false;
  const SCEV *SCEV_PHI = SE.getSCEV(PHI);
  if (!SE.containsAddRecurrence(SCEV_PHI))
    return false;
  const SCEVAddRecExpr *AddRecExpr = cast<SCEVAddRecExpr>(SCEV_PHI);
  if (!AddRecExpr->isAffine())
    return false;
  // Affine induction variable found
  Value *IndVar = PHI;
  const SCEV *IncrementSCEV = AddRecExpr->getStepRecurrence(SE);
  const SCEVConstant *IncrementSCEVCst = dyn_cast<SCEVConstant>(IncrementSCEV);
  if (!IncrementSCEVCst)
    return false;
  const APInt *IncrementValue = &IncrementSCEVCst->getValue()->getValue();
  auto val = PHI->getIncomingValueForBlock(incomingBlock);
  for (auto &e : edges) {
    if (e == backEdge) {
      edges.erase(e);
      Edge new_e = e;
      new_e.setSESE(IndVar, val, IncrementValue, exitBlocks);
      edges.insert(new_e);
      return true;
    }
  }
  return false;
}

bool NisseAnalysis::identifyBranchVariable(ScalarEvolution &SE,
                                           multiset<Edge> &edges, PHINode *PHI,
                                           BlockPtr incomingBlock,
                                           BlockPtr backBlock,
                                           SmallVector<BlockPtr> &exitBlocks) {
  long value = 0;
  set<Value *> definitions;
  queue<Value *> queue;
  BlockPtr opBlock = nullptr;
  Edge *edge = nullptr;
  definitions.insert(PHI);
  queue.push(PHI->getIncomingValueForBlock(backBlock));
  while (!queue.empty()) {
    auto val = queue.front();
    queue.pop();
    if (definitions.count(val) == 0) {
      definitions.insert(val);
      bool flag = true;
      if (auto *PHI = dyn_cast<PHINode>(val)) {
        flag = false;
        for (unsigned int i = 0; i < PHI->getNumIncomingValues(); i++) {
          queue.push(PHI->getIncomingValue(i));
        }
      }
      if (auto *BIN = dyn_cast<BinaryOperator>(val)) {
        if (!(BIN->getOpcode() == Instruction::Add ||
              BIN->getOpcode() == Instruction::Sub)) {
          return false;
        }
        flag = false;
        int constantOp = -1;
        int constantVal = 0;
        for (auto i = 0; i < 2; i++) {
          Value *operand = BIN->getOperand(i);
          if (ConstantInt *constant = dyn_cast<ConstantInt>(operand)) {
            if (constantOp != -1) {
              return false;
            }
            constantOp = i;
            constantVal = constant->getValue().getSExtValue();
          }
        }
        if (constantOp != -1) {
          if (opBlock != nullptr) {
            if (!IsSESERegion(opBlock, BIN->getParent())) {
              return false;
            }
          } else
            opBlock = BIN->getParent();
          value += constantVal;
          queue.push(BIN->getOperand(1 - constantOp));
          if (edge == nullptr) {
            BlockPtr block = BIN->getParent();
            if (BlockPtr pred = block->getUniquePredecessor()) {
              edge = new Edge(pred, block, -1);
            } else {
              if (BlockPtr succ = block->getUniqueSuccessor()) {
                edge = new Edge(block, succ, -1);
              }
            }
          }
        }
      }
      if (flag) {
        return false;
      }
    }
  }
  if (edge == nullptr || value == 0) {
    return false;
  }
  for (auto &e : edges) {
    if (e == *edge) {
      edges.erase(e);
      Edge new_e = e;
      new_e.setSESE(PHI, PHI->getIncomingValueForBlock(incomingBlock),
                    new APInt(64, value, true), exitBlocks);
      edges.insert(new_e);
      return true;
    }
  }
  return false;
}

void NisseAnalysis::identifyWellFoundedEdges(Loop *L, ScalarEvolution &SE,
                                             multiset<Edge> &edges) {
  BlockPtr incomingBlock, backBlock;
  L->getIncomingAndBackEdge(incomingBlock, backBlock);
  SmallVector<BlockPtr> exitBlocks;
  L->getExitBlocks(exitBlocks);
  auto firstBlock = incomingBlock->getSingleSuccessor();
  Edge backEdge(backBlock, firstBlock, -1);
  for (auto &PHI : firstBlock->phis()) {
    if (identifyInductionVariable(SE, edges, &PHI, incomingBlock, backBlock,
                                  backEdge, exitBlocks)) {
      SESECounters++;
      continue;
    }
    if (identifyBranchVariable(SE, edges, &PHI, incomingBlock, backBlock,
                               exitBlocks)) {
      SESECounters++;
      continue;
    }
  }
}

void AnalysisUtil::printGraph(Function &F, multiset<Edge> &edges,
                              pair<multiset<Edge>, multiset<Edge>> &STrev) {

  string fileName = F.getName().str() + ".graph";

  errs() << "Writing '" << fileName << "'...\n";

  int blockCount = distance(F.begin(), F.end());

  ofstream file;
  file.open(fileName, ios::out | ios::trunc);

  file << blockCount;
  for (auto &BB : F) {
    file << ' ' << AnalysisUtil::removebb(BB.getName().str());
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

Result NisseAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {

  auto edges = AnalysisUtil::generateEdges(F);

  initFunctionInfo(F, FAM);
  auto loops = LI.getLoopsInPreorder();
  for (auto loop : loops) {
    identifyWellFoundedEdges(loop, *SE, edges);
  }

  auto STrev = AnalysisUtil::generateSTrev(F, edges);

  errs() << "NisseAnalysis\n";
  AnalysisUtil::printGraph(F, edges, STrev);

  return make_tuple(edges, STrev.first, STrev.second);
}

Result BallAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {

  auto edges = AnalysisUtil::generateEdges(F);

  auto STrev = AnalysisUtil::generateSTrev(F, edges);

  errs() << "BallAnalysis\n";
  AnalysisUtil::printGraph(F, edges, STrev);

  return make_tuple(edges, STrev.first, STrev.second);
}

} // namespace nisse