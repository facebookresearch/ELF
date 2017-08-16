set(CMD_COMPILE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/compile_cmds.py"
	CACHE STRING "Path to the python script to compile commands.")

function(add_rts_command_gen DEF NAME)
	add_custom_command(
		OUTPUT ${DEF}.gen.h
		COMMAND python ${CMD_COMPILE_SCRIPT} --def_file ${DEF} --name ${NAME}
		DEPENDS ${DEF}.def
		)
endfunction()
add_rts_command_gen(${CMAKE_CURRENT_LIST_DIR}/cmd engine)
add_rts_command_gen(${CMAKE_CURRENT_LIST_DIR}/cmd_specific engine_specific)

# XXX this is a hack
# Because engine and games depend on each other..
# We let game include sources from engine
file(GLOB RTS_ENGINE_SOURCES *.cc)
list(APPEND RTS_ENGINE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/cmd.gen.h ${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific.gen.h)
set(RTS_ENGINE_SOURCES ${RTS_ENGINE_SOURCES} PARENT_SCOPE)
