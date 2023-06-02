#===============================================================================
# See: https://llvm.org/docs/CMake.html#developing-llvm-passes-out-of-source
#===============================================================================
add_library(Nisse MODULE
    NissePass.cpp
    NisseAnalysis.cpp
    NissePlugin.cpp
    Edge.cpp
    UnionFind.cpp)

target_include_directories(Nisse PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
    
target_link_libraries(Nisse LLVMSupport)