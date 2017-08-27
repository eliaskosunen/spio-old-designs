if(NOT SPIO_HEADER_ONLY)
    file(GLOB sources_spio *.cpp)
    add_library(spio ${sources_spio})
endif()
