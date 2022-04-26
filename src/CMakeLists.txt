# TYPE:

# generate library genmsg
add_library(genmsg type/msgtype.pb.cc type/genmsg.cpp)

# add protobuf dir to the include dir
target_include_directories(genmsg PRIVATE "/usr/local/include")

# add protobuf dir to the link dir
target_link_directories(genmsg PRIVATE "/usr/local/usr")

# add protobuf linkfile to the link
target_link_libraries(genmsg "protobuf")

# MEMBERLIST
aux_source_directory(memberlist memberlistfile)
add_library(memberlist ${memberlistfile})

add_dependencies(memberlist genmsg)

target_link_libraries(memberlist genmsg)

aux_source_directory(mynet mynetfile)

add_library(mynet ${mynetfile})