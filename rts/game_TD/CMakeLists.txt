add_rts_command_gen(${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific td_specific)

set(SOURCES ai.cc cmd_specific.cc gamedef.cc td_rule_actor.cc)
# add the generated file to be an explicit dependency
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific.gen.h)

# don't build python stuff together with the game
add_library(minirts-game SHARED ${SOURCES} ${RTS_ENGINE_SOURCES})
target_link_libraries(minirts-game PUBLIC elf)

# this is a hack. engine and game depend on each other. We let game include engine sources
target_include_directories(minirts-game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

# python libraries
pybind11_add_module(minirts python_wrapper.cc wrapper_callback.cc)
target_link_libraries(minirts PRIVATE minirts-game)
