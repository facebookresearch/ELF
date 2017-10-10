
add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)

add_library(websocketpp INTERFACE)
target_include_directories(websocketpp SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)
target_link_libraries(websocketpp INTERFACE asio)

add_subdirectory(pybind11)

find_package(Threads)
add_library(concurrentqueue INTERFACE)
target_include_directories(concurrentqueue SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)
target_link_libraries(concurrentqueue INTERFACE
	${CMAKE_THREAD_LIBS_INIT})

add_library(json INTERFACE)
target_include_directories(json SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)

find_package(Lua REQUIRED)
add_library(selene INTERFACE)
target_include_directories(selene SYSTEM INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	${LUA_INCLUDE_DIR}
)
target_link_libraries(selene INTERFACE ${LUA_LIBRARIES})

add_library(microtar INTERFACE)
target_sources(microtar INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/microtar.c)
target_include_directories(microtar SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)
