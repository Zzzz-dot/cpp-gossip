# generate library genmsg
add_library(genmsg msgtype.pb.cc genmsg.cpp)

# add protobuf dir to the include dir
target_include_directories(genmsg PRIVATE "/usr/local/include" PUBLIC "${CMAKE_SOURCE_DIR}/include")

# add protobuf dir to the link dir
target_link_directories(genmsg PRIVATE "/usr/local/usr")

# add protobuf linkfile to the link
target_link_libraries(genmsg "protobuf")