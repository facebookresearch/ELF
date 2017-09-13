# find sources
file(GLOB SOURCES lib/*.cc tar_loader.cc)

add_library(elf INTERFACE)
target_sources(elf INTERFACE ${SOURCES})
target_link_libraries(elf INTERFACE concurrentqueue tbb microtar)
target_include_directories(elf
	INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
)

target_compile_definitions(elf INTERFACE USE_TBB)
# git commit
execute_process(COMMAND git rev-parse HEAD
	OUTPUT_VARIABLE GIT_COMMIT_HASH)
execute_process(COMMAND git diff-index --quiet HEAD --
	RESULT_VARIABLE GIT_STAGED)
string(STRIP ${GIT_COMMIT_HASH} GIT_COMMIT_HASH)
target_compile_definitions(elf INTERFACE
	GIT_COMMIT_HASH=${GIT_COMMIT_HASH})
if(NOT ${GIT_STAGED})
	target_compile_definitions(elf INTERFACE
		GIT_UNSTAGED=staged)
endif()

