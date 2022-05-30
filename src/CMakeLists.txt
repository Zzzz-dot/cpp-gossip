# TYPE:

aux_source_directory(type typefile)

add_library(type ${typefile})

target_link_libraries(type protobuf)

# MYNET:
aux_source_directory(mynet mynetfile)

add_library(mynet ${mynetfile})

add_dependencies(mynet type)

target_link_libraries(mynet type)

find_package (glog 0.6.0 REQUIRED)

target_link_libraries (mynet glog::glog)

# MISC:
aux_source_directory(misc miscfile)

add_library(misc ${miscfile})

add_dependencies(misc mynet type)

target_link_libraries(misc mynet type)

target_link_libraries(misc pthread)

# MEMBERLIST:

aux_source_directory(memberlist memberlistfile)

add_library(memberlist ${memberlistfile})

add_dependencies(memberlist type mynet misc)

target_link_libraries(memberlist type mynet misc)

target_link_libraries(memberlist pthread)