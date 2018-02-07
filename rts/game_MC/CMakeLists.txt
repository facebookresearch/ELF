add_rts_command_gen(${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific minirts_specific)

# don't build python stuff together with the game, so we list the sources manually
set(SOURCES ai.cc game_action.cc trainable_ai.cc cmd_specific.cc gamedef.cc mc_rule_actor.cc state_feature.cc lua/lua_interface.cc lua/cpp_interface.cc)
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


execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/lua/map_generator.lua ${CMAKE_BINARY_DIR}/map_generator.lua)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/lua/unit_generator.lua ${CMAKE_BINARY_DIR}/unit_generator.lua)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/lua/unit_factory.lua ${CMAKE_BINARY_DIR}/unit_factory.lua)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/lua/attack_rule_book.lua ${CMAKE_BINARY_DIR}/attack_rule_book.lua)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/lua/move_rule_book.lua ${CMAKE_BINARY_DIR}/move_rule_book.lua)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/lua/map_parser.lua ${CMAKE_BINARY_DIR}/map_parser.lua)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/lua/m1.map ${CMAKE_BINARY_DIR}/m1.map)
