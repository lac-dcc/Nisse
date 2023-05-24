//===-- Edge.cpp --------------------------------------------------===//
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
/// This file contains the implementation of the Edge
///
//===----------------------------------------------------------------------===//

#include "Nisse.h"

using namespace llvm;
using namespace std;

namespace nisse {

BlockPtr Edge::getOrigin() const { return this->origin; }

BlockPtr Edge::getDest() const { return this->dest; }

int Edge::getWeight() { return this->weight; }

void Edge::setWeight(int weight) { this->weight = weight; }

void Edge::setIndex(int index) { this->index = index; }

void Edge::setWellFoundedValues(llvm::Value *indVar, llvm::Value *initValue,
                                const llvm::APInt *incrValue,
                                llvm::SmallVector<BlockPtr> &exitBlocks) {
  this->indVar = indVar;
  this->initValue = initValue;
  this->incrValue = incrValue->signedRoundToDouble();
  this->exitBlocks = exitBlocks;
  this->isBackEdge = true;
}

Instruction *Edge::getInstrumentationPoint() const {
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

void Edge::insertLoopIncrFn(int i, Value *inst) {
  for (auto block : this->exitBlocks) {
    auto instruction = &*block->getFirstInsertionPt();
    IRBuilder<> builder(instruction);
    auto *I = builder.getInt32Ty();
    Value *indexList[] = {builder.getInt32(i)};
    auto incrValueCst = builder.getInt32(this->incrValue);
    auto inst1 = builder.CreateGEP(I, inst, indexList);
    auto inst = builder.CreateLoad(I, inst1);
    auto inst3 = builder.CreateSub(this->indVar, this->initValue);
    if (inst3->getType() != builder.getInt32Ty()) {
      inst3 = builder.CreateIntCast(inst3, builder.getInt32Ty(), true);
    }
    auto incr0 = builder.CreateSDiv(inst3, incrValueCst);
    auto incr = builder.CreateIntCast(incr0, builder.getInt32Ty(), true);
    auto inst4 = builder.CreateAdd(inst, incr);
    builder.CreateStore(inst4, inst1);
  }
}

void Edge::insertIncrFn(int i, Value *inst) {
  if (this->isBackEdge) {
    this->insertLoopIncrFn(i, inst);
  } else {
    this->insertSimpleIncrFn(i, inst);
  }
}

int Edge::getIndex() const { return this->index; }

bool Edge::getIsBackEdge() const { return this->isBackEdge; }

string Edge::getName() const { return to_string(this->index); }

bool Edge::operator==(const Edge &e) const {
  return this->origin->getName() == e.origin->getName() &&
         this->dest->getName() == e.dest->getName();
}

bool Edge::operator<(const Edge &e) const {
  if (this->weight != e.weight)
    return this->weight < e.weight;
  return this->index < e.index;
}

bool Edge::operator>(const Edge &e) const {
  if (this->weight != e.weight)
    return this->weight > e.weight;
  return this->index > e.index;
}

bool Edge::compareWeights(const Edge &a, const Edge &b) { return b < a; }

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Edge &e) {
  os << e.getName() << " : " << e.getOrigin()->getName() << " -> "
     << e.getDest()->getName();
  return os;
}

std::ostream &operator<<(std::ostream &os, const Edge &e) {
  os << to_string(e.getIndex()) << string(" ")
     << NisseAnalysis::removebb(e.getOrigin()->getName().str()) << string(" ")
     << NisseAnalysis::removebb(e.getDest()->getName().str());
  return os;
}

} // namespace nisse