add_rts_command_gen(${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific cf_specific)

file(GLOB SOURCES *.cc)
add_library(minirts SHARED ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific.gen.h)
target_link_libraries(minirts elf pybind11 rts_engine)
set_target_properties(minirts PROPERTIES PREFIX "")
