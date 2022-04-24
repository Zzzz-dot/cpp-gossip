# TEST1
add_executable(queueTest safe_queue_test.cpp)

add_test(queuetest queueTest)

# TEST2
add_executable(timerTest timer_test.cpp)

add_test(timertest timerTest)

# TEST3
# generate excutable genmsgTest
add_executable(genmsgTest genmsg_test.cpp)

# this excutable depends on genmsg library
add_dependencies(genmsgTest genmsg)

# link genmsg library to this excutable
target_link_libraries(genmsgTest genmsg)

# add test, name: genmsgtest, command: genmsgTest
add_test(genmsgtest genmsgTest)