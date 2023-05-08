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
/// Ball-Larus edge profiling algorithm.
//
//===----------------------------------------------------------------------===//

#ifndef BALL_H
#define BALL_H

#include "llvm/IR/PassManager.h"
#include <map>

namespace nisse {

/// @brief Pointer to a Basic Block
using BlockPtr = llvm::BasicBlock *;

/// \struct Edge
///
/// \brief Representation of a CFG edge connecting two basic blocks.
///
struct Edge {
private:
  BlockPtr origin;                 ///< The origin of the edge.
  BlockPtr dest;                   ///< The destination of the edge.
  llvm::Instruction *BBInstrument; ///< Hook to insert Ball-Larus counter.
  int weight;       ///< An expectation of how often this edge will be executed.
  std::string name; ///< User-defined edge name, for debugging purposes.

  /// \brief Adds a name to the edge, for debugging purposes.
  /// \param name The name of the edge.
  void setName(std::string name);

public:
  /// \brief Default constructor for Edge.
  /// \param origin The origin of the edge.
  /// \param dest The destination of the edge.
  /// \param weight (optional) An expectation of how often the edge will be
  /// executed.
  Edge(BlockPtr origin, BlockPtr dest, int weight = 1, std::string = "");

  /// \brief Getter for the destination of the edge.
  /// \return The destination of the edge.
  BlockPtr getOrigin();

  /// \brief Getter for the destination of the edge.
  /// \return The destination of the edge.
  BlockPtr getDest();

  /// \brief Computes the hook to insert the Ball-Larus counter.
  /// If the source block terminates with an absolute jump, the counter is
  /// placed at the end of that block. If not, it is placed at the start of the
  /// destination block. If the hooked block is the entry block, the counter
  /// will always be placed at the end of the block.
  /// \return The pointer to the hook for the counter.
  llvm::Instruction *getInstrument();

  /// \brief Getter for the name of the edge.
  /// If there is no user-defined name, will return a default name in the form
  /// of "Origin -> Dest".
  /// \return Returns the name of the edge.
  llvm::Twine getName();

  /// \brief Checks if two edges have the same origins and destinations.
  /// \param e Edge to compare the current edge to.
  /// \return true if the current edge and e have the same origins and
  /// destinations.
  bool equals(const Edge &e) const;

  /// \brief Compares the weight of two edges.
  /// \param e Edge to compare the current edge to.
  /// \return true if the current edge has lower weight than e.
  bool operator<(const Edge &e) const;

  /// \brief Compares the weight of two edges.
  /// \param a First edge to compare.
  /// \param b Second edge to compare.
  /// \return true if a has higher weight than b.
  static bool compareWeights(Edge a, Edge b);
};
/// \struct UnionFind
///
/// \brief Implements union-find structure for Kruskal's Maximal Spanning Tree
/// algorithm
/// \see NisseAnalysis
struct UnionFind {
private:
  int cnt;                     ///< Number of elements in the structure.
  std::map<void *, void *> id; ///< Maps elements to their root.
  std::map<void *, int> sz;    ///< Maps roots to the size of their group.

public:
  /// \brief Initialises a new element in the structure.
  /// \param x Element to initialise.
  void init(void *x);

  /// \brief Finds the root of x, and collapses the path from x to its root.
  /// \param x Element to find the root of.
  /// \return The root of x.
  void *find(void *x);

  /// \brief Merges the sets of x and y, updates the weights, and makes the
  /// smaller root point to larger one. \param x First element to merge. \param
  /// y Second element to merge.
  void merge(void *x, void *y);

  /// \brief Checks if x and y belong to the same set.
  /// \param x First element to check.
  /// \param y Second element to check.
  /// \return true is x and y belong to the same set.
  bool connected(void *x, void *y);
};

/// \struct NisseAnalysis
///
/// \brief Computes the maximum spanning tree of a function's CFG
/// \see Edge, UnionFind
struct NisseAnalysis : public llvm::AnalysisInfoMixin<NisseAnalysis> {

  /// @brief The return type of the analysis pass.
  using Result = std::tuple<llvm::SmallVector<Edge>, std::multiset<Edge>,
                            std::multiset<Edge>>;

private:
  /// \brief Generates the set of edges of a function's CFG.
  /// \param F The function to compute the edges of.
  /// \return a vector containing the edges of F.
  llvm::SmallVector<Edge> generateEdges(llvm::Function &F);

  /// \brief Generates the maximum spanning tree of a set of F's edges.
  /// \param F The function the edges belong to.
  /// \param edges The set of edges to generate the maximum spanning tree of.
  /// \return A pair of the set of edges in the spanning tree and the set of
  /// edges in its complementary.
  std::pair<std::multiset<Edge>, std::multiset<Edge>>
  generateSTrev(llvm::Function &F, llvm::SmallVector<Edge> &edges);

public:
  static llvm::AnalysisKey
      Key; ///< A special type used by analysis passes to provide an address
           ///< that identifies that particular analysis pass type.

  /// \brief Find the return block of a function.
  /// \param F The function to find the return block of.
  /// \return The return block.
  static BlockPtr findReturnBlock(llvm::Function &F);

  /// \brief The analysis pass' run function.
  /// \param F The function to analyse.
  /// \param FAM The current FunctionAnalysisManager.
  /// \return A triple with the set of edges in F, a maximum spanning tree, and
  /// its complementary.
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
};

/// \struct NissePass
///
/// \brief Instruments a function for Ball-Larus edge instrumentation.
struct NissePass : public llvm::PassInfoMixin<NissePass> {
protected:
  int size; ///< The number of edges to instrument.

  /// \brief Inserts the initialization code, which creates a
  /// 0-initialized array of ints of size size.
  /// \param F The function to instrument.
  /// \return The instruction pointer to the assignment of the array.
  llvm::Value *insertEntryFn(llvm::Function &F);

  /// \brief Inserts an increment to the counter array.
  /// \param instruction The instruction above which to insert the increment.
  /// \param i The index of the array to increment.
  /// \param inst The instruction pointer to the counter array.
  void insertIncrFn(llvm::Instruction *instruction, int i, llvm::Value *inst);

  /// \brief Inserts a call to the function that prints the results of the
  /// counter.
  /// \param F The function begin instrumented.
  /// \param inst The instruction pointer to the counter array.
  void insertExitFn(llvm::Function &F, llvm::Value *inst);

public:
  /// \brief The transformation pass' run function. Instruments the function
  /// given as argument for Ball-Larus edge instrumentation.
  /// \param F The function to transform.
  /// \param FAM The current FunctionAnalysisManager.
  /// \return ¯\_ (ツ)_/¯
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};

/// \struct NissePassPrint
///
/// \brief Instruments a function for Ball-Larus edge instrumentation and prints
/// its edges, maximum spanning tree, and the instrumented edges.
struct NissePassPrint : public NissePass {
private:
  llvm::raw_ostream &OS; ///< Output stream for the pass.

public:
  /// \brief Constructor for the pass.
  /// \param OS Output stream for the pass.
  explicit NissePassPrint(llvm::raw_ostream &OS) : OS(OS) {}

  /// \brief The transformation pass' run function. Instruments the function
  /// give as argument for Ball-Larus edge instrumentation and prints the
  /// functions edges, spanning tree, and instrumented edges.
  /// \param F The function to transform.
  /// \param FAM The current FunctionAnalysisManager.
  /// \return ¯\_ (ツ)_/¯
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};

} // namespace nisse

#endif
