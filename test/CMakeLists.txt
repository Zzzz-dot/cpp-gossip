# TEST1
add_executable(queueTest broadcastQueue_test.cpp)

target_link_libraries(queueTest misc)

add_test(queuetest queueTest)

# TEST2
add_executable(timerTest timer_test.cpp)

add_dependencies(timerTest misc)

target_link_libraries(timerTest misc)

add_test(timertest timerTest)

# TEST3
# generate excutable genmsgTest
add_executable(genmsgTest genmsg_test.cpp)

# this excutable depends on genmsg library
add_dependencies(genmsgTest type)

# link genmsg library to this excutable
target_link_libraries(genmsgTest type)

# add test, name: genmsgtest, command: genmsgTest
add_test(genmsgtest genmsgTest)

# TEST4
add_executable(configTest config_test.cpp)

add_dependencies(configTest memberlist)

target_link_libraries(configTest memberlist)

add_test(configtest configTest)

# PIPE
add_executable(pipeTest pipe_test.cpp)

target_link_libraries(pipeTest pthread)

add_test(pipetest pipeTest)

# MUTEX
add_executable(mutexTest mutex_test.cpp)

target_link_libraries(mutexTest pthread)

add_test(mutextest mutexTest)

# SUSPICION
add_executable(suspicionTest suspicion_test.cpp)

target_link_libraries(suspicionTest misc)

add_test(suspiciontest suspicionTest)

# THREADLOCAL
add_executable(threadlocalTest threadlocal_test.cpp)

target_link_libraries(threadlocalTest pthread)

add_test(threadlocaltest threadlocalTest)