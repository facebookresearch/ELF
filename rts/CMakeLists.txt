CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
project(rts)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../elf/ ${CMAKE_BINARY_DIR}/elf/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vendor/ ${CMAKE_BINARY_DIR}/vendor/)

# include engine
add_subdirectory(engine)

# compile the game itself
set(GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game_MC" CACHE PATH "Directory to the game implementation, e.g. ../game_MC")
add_subdirectory(${GAME_DIR} ${CMAKE_BINARY_DIR}/minirts)
set_target_properties(minirts PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# compile the backend with this game
add_subdirectory(backend)
set_target_properties(minirts-backend PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
