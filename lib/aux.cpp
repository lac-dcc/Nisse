//===-- aux.cpp --------------------------------------------------===//
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
/// This file contains the implementation of the Edge and UnionFind
/// (Auxilliary classes for the pass)
///
//===----------------------------------------------------------------------===//

#include "Nisse.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/CFG.h"

using namespace llvm;
using namespace std;

namespace nisse {

// Implementation of Edge

Edge::Edge(BlockPtr BB1, BlockPtr BB2, int weight) {
  this->weight = weight;
  this->BBInstrument = nullptr;
  this->BB1 = BB1;
  this->BB2 = BB2;
}

BlockPtr Edge::getFirst() { return this->BB1; }

BlockPtr Edge::getSecond() { return this->BB2; }

// Instrument the source block if it ends with an absolute jump
// Instrument the destination block otherwise (no critical edges)
Instruction *Edge::getInstrument() {
  if (this->BBInstrument != nullptr)
    return this->BBInstrument;
  if (this->BB1->getUniqueSuccessor() == this->BB2) {
    this->BBInstrument = this->BB1->getTerminator();
  } else {
    this->BBInstrument = &*this->BB2->getFirstInsertionPt();
  }
  auto parent = this->BBInstrument->getParent();
  if (parent == &parent->getParent()->getEntryBlock()) {
    this->BBInstrument = parent->getTerminator();
  }
  return this->BBInstrument;
}

Twine Edge::getName() {
  return this->BB1->getName() + " to " + this->BB2->getName();
}

bool Edge::equals(const Edge &e) const {
  return this->BB1 == e.BB1 && this->BB2 == e.BB2;
}

bool Edge::operator<(const Edge &e) const { return this->weight < e.weight; }

bool Edge::compareWeights(Edge a, Edge b) { return a.weight < b.weight; }

// Implementation of UnionFind

void UnionFind::init(BlockPtr BB) {
  this->id[BB] = BB;
  this->sz[BB] = 1;
  this->cnt++;
}

BlockPtr UnionFind::find(BlockPtr BB) {
  BlockPtr root = BB;
  while (root != this->id[root])
    root = this->id[root];
  while (BB != root) {
    BlockPtr newp = this->id[BB];
    this->id[BB] = root;
    BB = newp;
  }
  return root;
}

void UnionFind::merge(BlockPtr x, BlockPtr y) {
  BlockPtr i = this->find(x);
  BlockPtr j = this->find(y);
  if (i == j)
    return;
  // make smaller root point to larger one
  if (this->sz[i] < this->sz[j]) {
    this->id[i] = j, this->sz[j] += this->sz[i];
  } else {
    this->id[j] = i, this->sz[i] += this->sz[j];
  }
  this->cnt--;
}

bool UnionFind::connected(BlockPtr x, BlockPtr y) {
  return this->find(x) == this->find(y);
}

} // namespace nisse