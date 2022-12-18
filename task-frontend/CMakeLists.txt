cmake_minimum_required(VERSION 3.0)

project(Frontend)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS} -O2 -fno-rtti")

set(SRC_LIST Driver.cc AST.cc)

find_package(BISON)
BISON_TARGET(Parser Grammar.yy ${CMAKE_CURRENT_BINARY_DIR}/Grammar.tab.cc VERBOSE COMPILE_FLAGS "-Wall -Wcex")

find_package(FLEX)
FLEX_TARGET(Scanner Lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)

ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)
include_directories(${PROJECT_BINARY_DIR} ".")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(driver.out ${SRC_LIST} ${BISON_Parser_OUTPUTS} ${FLEX_Scanner_OUTPUTS})
llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(driver.out ${llvm_libs})
