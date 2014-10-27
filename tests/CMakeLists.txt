link_directories(../src)

add_executable(sfstest sfstest.c)
target_link_libraries(sfstest sfs)

add_executable(tests tests.c)
target_link_libraries(tests sfs)