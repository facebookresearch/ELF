set(CMAKE_CXX_STANDARD 11)

# find sources
file(GLOB SOURCES lib/*.cc)

add_library(elf SHARED ${SOURCES})
target_link_libraries(elf concurrentqueue tbb)
target_include_directories(elf
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
)

target_compile_definitions(elf PUBLIC USE_TBB)
# git commit
execute_process(COMMAND git rev-parse HEAD
	OUTPUT_VARIABLE GIT_COMMIT_HASH)
execute_process(COMMAND git diff-index --quiet HEAD --
	RESULT_VARIABLE GIT_STAGED)
string(STRIP ${GIT_COMMIT_HASH} GIT_COMMIT_HASH)
target_compile_definitions(elf PUBLIC
	GIT_COMMIT_HASH=${GIT_COMMIT_HASH})
if(NOT ${GIT_STAGED})
	target_compile_definitions(elf PUBLIC
		GIT_UNSTAGED=staged)
endif()

