# ===============================================================================
# See: https://llvm.org/docs/CMake.html#developing-llvm-passes-out-of-source
# ===============================================================================
add_executable(propagation
    NissePropagation.cpp)

target_include_directories(propagation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_link_libraries(propagation LLVMSupport)