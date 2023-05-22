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
/// (Auxiliary classes for the pass)
///
//===----------------------------------------------------------------------===//

#include "Nisse.h"

using namespace llvm;
using namespace std;

namespace nisse {

// Implementation of Edge

BlockPtr Edge::getOrigin() const { return this->origin; }

BlockPtr Edge::getDest() const { return this->dest; }

void Edge::setWeight(int weight) { this->weight = weight; }

void Edge::setWellFoundedValues(llvm::Value *indVar, llvm::Instruction *instr,
                                const llvm::APInt *incrVal) {
  this->indVar = indVar;
  this->instr = instr;
  this->incrVal = incrVal;
}

Instruction *Edge::getInstrumentationPoint() const {
  if (this->instr != nullptr)
    return this->instr;
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

llvm::Value *Edge::getInductionVariable(llvm::IRBuilder<> &builder) const {
  if (this->indVar != nullptr)
    return this->indVar;
  return builder.getInt32(1);
}

void Edge::insertSimpleIncrFn(int i, Value *inst) {
  auto instruction = this->getInstrumentationPoint();
  IRBuilder<> builder(instruction);
  auto *I = builder.getInt32Ty();
  Value *indexList[] = {builder.getInt32(i)};
  Value *incr = builder.getInt32(1);
  auto inst1 = builder.CreateGEP(I, inst, indexList);
  auto inst2 = builder.CreateLoad(I, inst1);
  auto inst3 = builder.CreateAdd(inst2, incr);
  builder.CreateStore(inst3, inst1);
}

void Edge::insertIncrFn(int i, Value *inst) {
  if (this->indVar != nullptr)
    this->insertLoopIncrFn(i, inst);
  else
    this->insertLoopIncrFn(i, inst);
}

int Edge::getIndex() const { return this->index; }

string Edge::getName() const { return to_string(this->index); }

bool Edge::operator==(const Edge &e) const {
  return this->origin == e.origin && this->dest == e.dest;
}

bool Edge::operator<(const Edge &e) const { return this->weight < e.weight; }

bool Edge::compareWeights(const Edge &a, const Edge &b) { return b < a; }

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Edge &e) {
  os << e.getName() << " : " << e.getOrigin()->getName() << " -> "
     << e.getDest()->getName();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Edge &e) {
  os << NisseAnalysis::removebb(e.getOrigin()->getName().str()) << string(" ")
     << NisseAnalysis::removebb(e.getDest()->getName().str());
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