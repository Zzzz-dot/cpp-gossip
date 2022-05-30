# cmake minimum version required
cmake_minimum_required(VERSION 3.16)

# project: cpp-gossip, version: 1.0.0
project(cpp-gossip VERSION 1.0.0)

configure_file(cmakeConfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/cmakeConfig.h)

#execute_process(COMMAND protoc -I=${CMAKE_SOURCE_DIR}/include/type/ --cpp_out=${CMAKE_SOURCE_DIR}/src/type/ ${CMAKE_SOURCE_DIR}/include/type/msgtype.proto)
#execute_process(COMMAND mv ${CMAKE_SOURCE_DIR}/src/type/msgtype.pb.h ${CMAKE_SOURCE_DIR}/include/type/msgtype.pb.h)

set(CMAKE_CXX_FLAGS "-lpthread")

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

# test
enable_testing()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(temp)

add_executable(main main.cpp)

target_link_libraries(main memberlist)