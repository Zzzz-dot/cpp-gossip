# cmake minimum version required
cmake_minimum_required(VERSION 3.16)

# project: cpp-gossip, version: 1.0.0
project(cpp-gossip VERSION 1.0.0)


#execute_process(COMMAND protoc -I=${CMAKE_SOURCE_DIR}/include/type/ --cpp_out=${CMAKE_SOURCE_DIR}/src/type/ ${CMAKE_SOURCE_DIR}/include/type/msgtype.proto)
#execute_process(COMMAND mv ${CMAKE_SOURCE_DIR}/src/type/msgtype.pb.h ${CMAKE_SOURCE_DIR}/include/type/msgtype.pb.h)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

# test
enable_testing()

# add directory ./memberlist/type to the cmake build list
add_subdirectory(src)
# add directory ./test to the cmake build list
add_subdirectory(test)


# temp directory
add_subdirectory(temp)