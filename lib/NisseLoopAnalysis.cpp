//===-- NisseLoopAnalysis.cpp
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
/// This file contains the implementation of the NisseLoopAnalysis
///
//===----------------------------------------------------------------------===//

#include "Nisse.h"

using namespace llvm;
using namespace std;

// namespace nisse {
// NisseLoopAnalysis::Result NisseLoopAnalysis::run(Loop &L,
//                                                  LoopAnalysisManager &LAM) {
//   auto P = L.getCanonicalInductionVariable();
//   BlockPtr Incoming, BackEdge;
//   if (!L.getIncomingAndBackEdge(Incoming, BackEdge)) {
//     BackEdge = nullptr;
//   }
//   SmallVector<pair<BlockPtr, BlockPtr>> ExitEdges;
//   L.getExitEdges(ExitEdges);

//   return Result(P, BackEdge, ExitEdges);
// }
// } // namespace nisse