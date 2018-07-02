file(GLOB sources_bench *.cpp)
add_executable(bench ${sources_bench})
target_link_libraries(bench benchmark)
