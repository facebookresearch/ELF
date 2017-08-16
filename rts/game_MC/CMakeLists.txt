add_rts_command_gen(${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific minirts_specific)


set(SOURCES ai.cc cmd_specific.cc gamedef.cc mc_rule_actor.cc)
# add the generated file to be an explicit dependency
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific.gen.h)

# don't build python stuff together with the game
add_library(minirts-game SHARED ${SOURCES})
target_link_libraries(minirts-game PUBLIC elf rts_engine)

# python libraries
pybind11_add_module(minirts python_wrapper.cc wrapper_callback.cc)
target_link_libraries(minirts PRIVATE minirts-game)
