set(CMD_COMPILE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/compile_cmds.py"
	CACHE STRING "Path to the python script to compile commands.")

function(add_rts_command_gen DEF NAME)
	# use the specified python, if any
	if (NOT PYTHON_EXECUTABLE)
		set(_PYTHON_EXE python)
	else()
		set(_PYTHON_EXE "${PYTHON_EXECUTABLE}")
	endif()

	add_custom_command(
		OUTPUT ${DEF}.gen.h
		COMMAND ${_PYTHON_EXE} ${CMD_COMPILE_SCRIPT} --def_file ${DEF} --name ${NAME}
		COMMENT "Running compile_cmds.py on ${DEF}"
		VERBATIM
		DEPENDS ${DEF}.def
		)
	set_source_files_properties(${DEF}.gen.h PROPERTIES GENERATED TRUE)
endfunction()

function(prepend_each OUTPUT PREFIX)
	set(ret "")
	foreach(f ${ARGN})
		list(APPEND ret "${PREFIX}/${f}")
	endforeach()
	set(${OUTPUT} "${ret}" PARENT_SCOPE)
endfunction()
add_rts_command_gen(${CMAKE_CURRENT_SOURCE_DIR}/cmd engine)
add_rts_command_gen(${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific engine_specific)

# Engine and game depend on each other
# But we're using INTERFACE target, sources will be built together in the end, so it can work
file(GLOB RTS_ENGINE_SOURCES *.cc)
file(GLOB RTS_ENGINE_LUA_SOURCES lua/*.cc)
add_library(minirts-engine INTERFACE)
target_sources(minirts-engine INTERFACE ${RTS_ENGINE_SOURCES} ${RTS_ENGINE_LUA_SOURCES})
target_include_directories(minirts-engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(minirts-engine INTERFACE selene)

add_custom_target(minirts-engine-headers DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/cmd.gen.h
	${CMAKE_CURRENT_SOURCE_DIR}/cmd_specific.gen.h)
add_dependencies(minirts-engine minirts-engine-headers)
