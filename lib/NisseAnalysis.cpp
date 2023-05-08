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
#include "llvm/IR/Instructions.h"
// #include <iostream>
#include <fstream>

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

NisseAnalysis::Result NisseAnalysis::run(llvm::Function &F,
                                         llvm::FunctionAnalysisManager &FAM) {

  auto edges = this->generateEdges(F);
  auto STrev = this->generateSTrev(F, edges);

  string fileName = F.getName().str() + ".graph";

  errs() << "Writing '" << fileName << "'...";

  int blockCount = distance(F.begin(), F.end());

  ofstream file;
  file.open(fileName, ios::out | ios::trunc);

  file << blockCount << " Basic Blocks\n";
  for (auto &BB: F) {
    file << '\t' << BB.getName().str() << '\n';
  }

  file << STrev.first.size() << " Edges in the Spanning Tree\n";
  for (auto e: STrev.first) {
    file << '\t' << e << '\n';
  }

  file << STrev.second.size() << " Instrumented Edges\n";
  for (auto e: STrev.second) {
    file << '\t' << e << '\n';
  }

  file.close();

  return make_tuple(edges, STrev.first, STrev.second);
}
} // namespace nisse