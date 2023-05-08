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

Edge::Edge(BlockPtr origin, BlockPtr dest, int weight, std::string name) {
  this->weight = weight;
  this->BBInstrument = nullptr;
  this->origin = origin;
  this->dest = dest;
  this->name = name;
}

BlockPtr Edge::getOrigin() { return this->origin; }

BlockPtr Edge::getDest() { return this->dest; }

// Instrument the source block if it ends with an absolute jump
// Instrument the destination block otherwise (no critical edges)
Instruction *Edge::getInstrument() {
  if (this->BBInstrument != nullptr)
    return this->BBInstrument;
  if (this->origin->getUniqueSuccessor() == this->dest) {
    this->BBInstrument = this->origin->getTerminator();
  } else {
    this->BBInstrument = &*this->dest->getFirstInsertionPt();
  }
  auto parent = this->BBInstrument->getParent();
  if (parent == &parent->getParent()->getEntryBlock()) {
    this->BBInstrument = parent->getTerminator();
  }
  return this->BBInstrument;
}

Twine Edge::getName() {
  return this->origin->getName() + " to " + this->dest->getName();
}

bool Edge::equals(const Edge &e) const {
  return this->origin == e.origin && this->dest == e.dest;
}

bool Edge::operator<(const Edge &e) const { return this->weight < e.weight; }

bool Edge::compareWeights(Edge a, Edge b) { return a.weight < b.weight; }

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