//===-- NissePropagation.cpp
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
/// This file contains the implementation of the NissePropagation
///
//===----------------------------------------------------------------------===//

#include "llvm/Support/CommandLine.h"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <utility>

using namespace std;
using namespace llvm;

/// \brief Shorthand for vector of int.
using vi = vector<int>;
/// \brief Shorthand for vector of vector of int.
using vvi = vector<vi>;
/// \brief Shorthand for vector of pairs of int.
using vpi = vector<pair<int, int>>;
/// \brief Shorthand for set of int.
using si = set<int>;
/// \brief Shorthand for map from int to set of int.
using msi = map<int, si>;

/// \brief Initialises the variables given as input with the graph described by
/// the file input.
/// \param input Path to the input file.
/// \param vertex The graph's vertices.
/// \param edges The graph's edges.
/// \param ST A spanning tree of the graph.
/// \param revST The edges not in the spanning tree.
/// \param in in[x] contains the edges towards x.
/// \param out out[x] contains the edges from x.
/// \param debug Flag for the debug messages.
void initGraph(string input, vi &vertex, vpi &edges, si &ST, si &revST, msi &in,
               msi &out, bool debug) {
  int count;
  ifstream graph;
  graph.open(input + ".graph");
  graph >> count;

  if (debug)
    cout << count << endl;

  for (int i = 0; i < count; i++) {
    int tmp;
    graph >> tmp;
    vertex.push_back(tmp);
    in[tmp] = si();
    out[tmp] = si();
  }

  if (debug) {
    for (auto i : vertex) {
      cout << i << " ";
    }
    cout << endl;
  }

  graph >> count;
  edges = vpi(count);
  for (int i = 0; i < count; i++) {
    int j, a, b;
    graph >> j >> a >> b;
    edges.at(j) = pair(a, b);
    out.at(a).insert(j);
    in.at(b).insert(j);
  }

  if (debug) {
    for (auto p : edges) {
      cout << p.first << ' ' << p.second << '\n';
    }
    cout << endl;
  }

  graph >> count;
  for (int i = 0; i < count; i++) {
    int tmp;
    graph >> tmp;
    ST.insert(tmp);
  }

  if (debug) {
    for (auto i : ST) {
      cout << i << " ";
    }
    cout << endl;
  }

  graph >> count;
  for (int i = 0; i < count; i++) {
    int tmp;
    graph >> tmp;
    revST.insert(tmp);
  }

  if (debug) {
    for (auto i : revST) {
      cout << i << " ";
    }
    cout << endl;
  }

  graph.close();
}

/// \brief Initialises the edge weights based on the input file. If there are
/// multiple profilings, will sum each of them to get the total profile.
/// \param input Path to the input file.
/// \param edgeCount Number of edges in the graph.
/// \param instCount Number of instrumented edges.
/// \param debug Flag for the debug messages.
/// \return The weights of the edges, initialized at 0, or at the total value
/// given in the input file.
vi initWeights(string input, int edgeCount, int instCount, bool debug) {
  vi weights(edgeCount, 0);
  ifstream prof;
  string buf;
  prof.open(input + ".prof");

  while (prof >> buf) {
    for (auto i = 0; i < instCount; i++) {
      int edge, weight;
      prof >> edge >> weight;
      weights.at(edge) += weight;
    }
  }

  if (debug) {
    for (auto i : weights) {
      cout << i << " ";
    }
    cout << endl;
  }

  prof.close();
  return weights;
}

/// \brief Initialises the edge weights based on the input file. If there are
/// multiple profilings, will sum create a separate set of weights for each.
/// \param input Path to the input file.
/// \param edgeCount Number of edges in the graph.
/// \param instCount Number of instrumented edges.
/// \param debug Flag for the debug messages.
/// \return The weights of the edges, initialized at 0, or at the values given
/// in the input file.
vvi initWeightsSeparate(string input, int edgeCount, int instCount,
                        bool debug) {
  vvi weights;
  ifstream prof;
  string buf;
  prof.open(input + ".prof");

  while (prof >> buf) {
    vi w(edgeCount, 0);
    for (auto i = 0; i < instCount; i++) {
      int edge, weight;
      prof >> edge >> weight;
      w.at(edge) = weight;
    }
    weights.push_back(w);
  }

  if (debug) {
    for (auto w : weights) {
      for (auto i : w) {
        cout << i << " ";
      }
      cout << endl;
    }
  }

  prof.close();
  return weights;
}

/// \brief Propagates the weights across the entire graph.
/// \param edges The graph's edges.
/// \param ST The graph's spanning tree (edges that have not been instrumented).
/// \param in in[x] contains the edges towards x.
/// \param out out[x] contains the edges from x.
/// \param weights The edge's weights.
/// \param v The vertex to propagate from.
/// \param e The edge to propagate from (not used on the initial call to the
/// function).
void propagation(vpi &edges, si &ST, msi &in, msi &out, vi &weights, int v,
                 int e = -1) {
  int in_sum = 0;
  for (auto ep : in.at(v)) {
    if (ep != e && ST.count(ep) == 1) {
      propagation(edges, ST, in, out, weights, edges.at(ep).first, ep);
    }
    in_sum += weights.at(ep);
  }

  int out_sum = 0;
  for (auto ep : out.at(v)) {
    if (ep != e && ST.count(ep) == 1) {
      propagation(edges, ST, in, out, weights, edges.at(ep).second, ep);
    }
    out_sum += weights.at(ep);
  }

  if (e != -1) {
    weights.at(e) = max(in_sum, out_sum) - min(in_sum, out_sum);
  }
}

/// \brief Outputs the weights of the edges to the standard output.
/// \param edges The graph's edges.
/// \param weights The edge's weights.
void outputCout(vpi &edges, vi &weights) {
  int size = edges.size();
  for (int i = 0; i < size; i++) {
    cout << edges.at(i).first << " -> " << edges.at(i).second << " : "
         << weights.at(i) << '\n';
  }
  cout << endl;
}

/// \brief Outputs the weights of the edges to the file given as input.
/// \param filename The path to the file to write the results.
/// \param edges The graph's edges.
/// \param weights The edge's weights.
void outputFile(string filename, vpi &edges, vi &weights) {
  ofstream file;
  file.open(filename, ios::out | ios::app);

  if (file.bad()) {
    cout << "Could not open file " << filename << endl;
    outputCout(edges, weights);
  }

  int size = edges.size();
  for (int i = 0; i < size; i++) {
    file << edges.at(i).first << " -> " << edges.at(i).second << " : "
         << weights.at(i) << '\n';
  }

  file << endl;
  file.close();
}

/// \brief Propagates the weights given by edge instrumentation. If the graph is
/// described in x.graph and the profiling in x.prof, call with x as the input
/// file.
/// \param argc (⊙ˍ⊙)
/// \param argv (⊙ˍ⊙)
/// \return 0
int main(int argc, char **argv) {
  cl::opt<string> InputFilename(cl::Positional, cl::desc("<input file>"),
                                cl::Required);
  cl::opt<string> OutputFilename("o", cl::desc("Specify output filename"),
                                 cl::value_desc("filename"));
  cl::opt<bool> Debug("d", cl::desc("Enable debug messages"));
  cl::opt<bool> Separate(
      "s", cl::desc("Do separate profilings for each function execution"));

  cl::ParseCommandLineOptions(argc, argv);
  vi vertex;
  vpi edges;
  si ST, revST;
  msi in, out;
  vvi weights;

  int period = InputFilename.find_last_of('.');
  int slash = InputFilename.find_last_of('/');
  string input;
  if (period > slash) {
    input = InputFilename.substr(0, period);
  } else {
    input = InputFilename.getValue();
  }

  if (Debug) {
    cout << "\nComputing the graph of " << input << "\n\n";
  }

  initGraph(input, vertex, edges, ST, revST, in, out, Debug);

  if (Debug) {
    cout << "\nComputing the input weights\n\n";
  }

  if (Separate) {
    weights = initWeightsSeparate(input, edges.size(), revST.size(), Debug);
  } else {
    weights.push_back(initWeights(input, edges.size(), revST.size(), Debug));
  }

  if (Debug) {
    cout << "\nPropagating the weights\n\n";
  }
  bool to_print = true;
  for (auto w : weights) {
    propagation(edges, ST, in, out, w, 0);

    if (OutputFilename.size() > 0) {
      if (to_print) {
        cout << "Writing '" << OutputFilename << "'...\n";
        to_print = false;
      }
      outputFile(OutputFilename, edges, w);
    } else {
      if (to_print) {
        cout << "Printing the weights of '" << input << "'...\n";
        to_print = false;
      }
      outputCout(edges, w);
    }
  }

  return 0;
}