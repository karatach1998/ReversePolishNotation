cmake_minimum_required(VERSION 3.5)
project(Rpn)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(HEADER Rpn.h)
set(SOURCE Rpn.cpp)
set(SOURCE_FILES_RPN ${HEADER} ${SOURCE})
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES_RPN})