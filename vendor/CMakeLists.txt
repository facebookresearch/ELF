
add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)

add_library(websocketpp INTERFACE)
target_include_directories(websocketpp SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)
target_link_libraries(websocketpp INTERFACE asio)

find_package(PythonLibs REQUIRED)
message("-- Found Python Include Dirs: ${PYTHON_INCLUDE_DIRS}")
add_library(pybind11 INTERFACE)
target_include_directories(pybind11 SYSTEM INTERFACE
	 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
	 ${PYTHON_INCLUDE_DIRS}
)
target_link_libraries(pybind11 INTERFACE ${PYTHON_LIBRARIES})

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
