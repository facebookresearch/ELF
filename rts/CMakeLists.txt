CMAKE_MINIMUM_REQUIRED(VERSION 3.3)
# 3.3 is required. See https://cmake.org/Bug/view.php?id=15414
project(rts)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-g -march=native")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../elf/ ${CMAKE_BINARY_DIR}/elf/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vendor/ ${CMAKE_BINARY_DIR}/vendor/)

# include engine
add_subdirectory(engine)

# compile the game itself
# possibly need -DLUA_INCLUDE_DIR=/usr/local/include/ -DLUA_LIBRARY=/usr/local/lib/liblua.a\;/usr/lib64/libdl.so to find correct lua
set(GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game_MC" CACHE PATH "Directory to the game implementation, e.g. ../game_MC")
add_subdirectory(${GAME_DIR} ${CMAKE_BINARY_DIR}/minirts)
set_target_properties(minirts PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY "${GAME_DIR}"
)

# compile the backend with this game
add_subdirectory(backend)
set_target_properties(minirts-backend PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/engine/test2.lua ${CMAKE_BINARY_DIR}/test2.lua)
