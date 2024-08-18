cmake_minimum_required(VERSION 3.0.2)
project(kaleidoscope)

find_package(LLVM REQUIRED CONFIG)

include_directories(include/
  ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(klc src/main.cpp)
llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(klc LLVMSupport ${llvm_libs})
