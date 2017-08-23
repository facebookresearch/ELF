add_rts_command_gen(${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific td_specific)

# don't build python stuff together with the game, so we list the sources manually
set(SOURCES ai.cc cmd_specific.cc gamedef.cc td_rule_actor.cc)
prepend_each(SOURCES ${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES})

add_library(minirts-game INTERFACE)
target_sources(minirts-game INTERFACE ${SOURCES})
target_link_libraries(minirts-game INTERFACE elf minirts-engine)

add_custom_target(minirts-game-headers DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific.gen.h)
add_dependencies(minirts-game minirts-game-headers)

# python libraries
pybind11_add_module(minirts python_wrapper.cc wrapper_callback.cc)
target_link_libraries(minirts PRIVATE minirts-game)
