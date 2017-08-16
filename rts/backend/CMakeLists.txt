
file(GLOB SOURCES *.cc)
add_executable(minirts-backend ${SOURCES})
target_link_libraries(minirts-backend
	rts_engine minirts-game
	concurrentqueue json websocketpp)
target_include_directories(minirts-backend PRIVATE ${GAME_DIR})
