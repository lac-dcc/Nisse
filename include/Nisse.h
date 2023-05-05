//===-- Nisse.h ----------------------------------------------*- C++ -*-===//
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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the analysis used to apply the
/// Ball Larus edge profiling algorithm.
//
//===----------------------------------------------------------------------===//

#ifndef BALL_H
#define BALL_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace nisse {

using BlockPtr = llvm::BasicBlock *;

struct Edge {
private:
  BlockPtr BB1;
  BlockPtr BB2;
  llvm::Instruction *BBInstrument;
  int weight;

  void setName(std::string name);

public:
  Edge(BlockPtr BB1, BlockPtr BB2, int weight);
  BlockPtr getFirst();
  BlockPtr getSecond();
  llvm::Instruction *getInstrument();
  llvm::Twine getName();
  bool equals(const Edge &e) const;
  bool operator<(const Edge &e) const;
  static bool compareWeights(Edge a, Edge b);
};

class UnionFind {
  int cnt;
  std::map<BlockPtr, BlockPtr> id;
  std::map<BlockPtr, int> sz;

public:
  void init(BlockPtr BB);
  BlockPtr find(BlockPtr BB);
  void merge(BlockPtr x, BlockPtr y);
  bool connected(BlockPtr x, BlockPtr y);
};

struct NisseAnalysis : public llvm::AnalysisInfoMixin<NisseAnalysis> {

  using Result = std::pair<llvm::SmallVector<Edge>,
                           std::pair<std::multiset<Edge>, std::multiset<Edge>>>;

private:
  llvm::SmallVector<Edge> generateEdges(llvm::Function &F);
  std::pair<std::multiset<Edge>, std::multiset<Edge>>
  generateSTrev(llvm::Function &F, llvm::SmallVector<Edge> &edges);
  // std::multiset<Edge> reverseST(llvm::Function &F,
  //                               llvm::SmallVector<Edge> &edges,
  //                               std::multiset<Edge> &ST);

public:
  // A special type used by analysis passes to provide an address that
  // identifies that particular analysis pass type.
  static llvm::AnalysisKey Key;
  static BlockPtr findExitBlock(llvm::Function &F);
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
};

struct NissePass : public llvm::PassInfoMixin<NissePass> {
protected:
  // std::multiset<Edge> STEdges;
  std::multiset<Edge> reverseSTEdges;

  // void addToReverse(BlockPtr BB1, BlockPtr BB2);
  // bool inST(BlockPtr BB1, BlockPtr BB2);

  llvm::Value *insertEntryFn(llvm::Function &F, llvm::Module &M);
  void insertIncrFn(llvm::Module &M, llvm::Instruction *instruction, int i,
                    llvm::Value *inst);
  void insertExitFn(BlockPtr BB, llvm::Function &F, llvm::Module &M,
                    llvm::Value *inst);

public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};

struct NissePassPrint : public NissePass {
private:
  llvm::raw_ostream &OS;

public:
  explicit NissePassPrint(llvm::raw_ostream &OS) : OS(OS) {}
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};

} // namespace nisse

#endif