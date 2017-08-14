
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

file(GLOB SOURCES *.cc)
add_library(rts_engine SHARED ${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/cmd.gen.h ${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific.gen.h)
target_include_directories(rts_engine PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
