if(NOT SPIO_HEADER_ONLY)
    add_subdirectory(fmt)

    file(GLOB sources_spio *.cpp)
    add_library(spio ${sources_spio})
    target_link_libraries(spio PRIVATE fmt)
else()
    add_subdirectory(fmt EXCLUDE_FROM_ALL)
endif()
