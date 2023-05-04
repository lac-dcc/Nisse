#===============================================================================
# See: https://llvm.org/docs/CMake.html#developing-llvm-passes-out-of-source
#===============================================================================
add_library(Ball MODULE
    BallPass.cpp
    BallAnalysis.cpp
    BallPlugin.cpp)

target_include_directories(Ball PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
