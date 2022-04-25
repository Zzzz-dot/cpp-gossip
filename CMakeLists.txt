# cmake minimum version required
cmake_minimum_required(VERSION 3.16)

# project: cpp-gossip, version: 1.0.0
project(cpp-gossip VERSION 1.0.0)

include_directories(include)

# test
enable_testing()

# add directory ./memberlist/type to the cmake build list
add_subdirectory(src)
# add directory ./test to the cmake build list
add_subdirectory(test)


# temp directory
add_subdirectory(temp)