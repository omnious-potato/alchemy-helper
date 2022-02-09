cmake_minimum_required(VERSION 3.15)

project(AlchemyTESV 
	VERSION 
		0.0.1 
	DESCRIPTION 
		"Alchemy Helper for Skyrim, extracting metadata from savefiles"
	LANGUAGES 
		CXX
	)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
add_executable(save_interpreter src/save_interpreter.cpp)

target_link_libraries(save_interpreter lz4)