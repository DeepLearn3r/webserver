set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

cmake_minimum_required(VERSION 3.4.1)#版本
project(webserver)#project 名字

include_directories(
    ${PROJECT_SOURCE_DIR}/include/base_include
    ${PROJECT_SOURCE_DIR}/include/src_include
    ${PROJECT_SOURCE_DIR}/include/logger_include
)#include文件目录

# add_subdirectory(src)#子目录中有CMakeLists.txt的有哪些

aux_source_directory(./src SRC_LIST)
aux_source_directory(./src/base BASE_LIST)
aux_source_directory(./src/logger LOGGER_LIST)

add_executable(webserver ${SRC_LIST} ${BASE_LIST} ${LOGGER_LIST})
target_link_libraries(${PROJECT_NAME} pthread)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)