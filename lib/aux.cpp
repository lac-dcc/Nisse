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

using namespace llvm;
using namespace std;

namespace nisse {

// Implementation of Edge

Edge::Edge(BlockPtr origin, BlockPtr dest, int index, int weight, string name) {
  this->origin = origin;
  this->dest = dest;
  this->index = index;
  this->weight = weight;
  this->name = name;
}

BlockPtr Edge::getOrigin() const { return this->origin; }

BlockPtr Edge::getDest() const { return this->dest; }

Instruction *Edge::getInstrument() const {
  Instruction *instr;
  if (this->origin->getUniqueSuccessor() == this->dest) {
    instr = this->origin->getTerminator();
  } else {
    instr = &*this->dest->getFirstInsertionPt();
  }
  auto parent = instr->getParent();
  if (parent == &parent->getParent()->getEntryBlock()) {
    instr = parent->getTerminator();
  }
  return instr;
}

int Edge::getIndex() const { return this->index; }

string Edge::getName() const {
  if (this->name.length() > 0) {
    return this->name;
  }
  return to_string(this->index);
}

bool Edge::equals(const Edge &e) const {
  return this->origin == e.origin && this->dest == e.dest;
}

bool Edge::operator<(const Edge &e) const { return this->weight < e.weight; }

bool Edge::compareWeights(const Edge &a, const Edge &b) {
  return a.weight > b.weight;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Edge &e) {
  os << e.getName() << " : " << e.getOrigin()->getName() << " -> "
     << e.getDest()->getName();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Edge &e) {
  os << e.getName() << string(" : ") << e.getOrigin()->getName().str()
     << string(" -> ") << e.getDest()->getName().str();
  return os;
}

// Implementation of UnionFind

void UnionFind::init(void *x) {
  this->id[x] = x;
  this->sz[x] = 1;
  this->cnt++;
}

void *UnionFind::find(void *x) {
  void *root = x;
  while (root != this->id[root])
    root = this->id[root];
  while (x != root) {
    void *newp = this->id[x];
    this->id[x] = root;
    x = newp;
  }
  return root;
}

void UnionFind::merge(void *x, void *y) {
  void *i = this->find(x);
  void *j = this->find(y);
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

bool UnionFind::connected(void *x, void *y) {
  return this->find(x) == this->find(y);
}

} // namespace nisse