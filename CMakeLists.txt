cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
	# Convert path from Windows
	file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
	execute_process(
			COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
			COMMAND head -n 1
			COMMAND cut -c9-
			OUTPUT_VARIABLE SDK
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (NOT EXISTS ${SDK})
	message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
	return()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Game Name Customization
set(PLAYDATE_GAME_NAME Game)
set(PLAYDATE_GAME_DEVICE Game)

project(${PLAYDATE_GAME_NAME} C ASM)

file(GLOB_RECURSE SOURCES
    "src/*.c"
)

file(GLOB_RECURSE HEADERS
    "src/*.h"
)

file(GLOB IMAGES
	"Source/images/*"
	"Source/fonts/*"
)

if (TOOLCHAIN STREQUAL "armgcc")
    add_executable(${PLAYDATE_GAME_DEVICE} ${SOURCES})
	target_include_directories(${PLAYDATE_GAME_DEVICE} PRIVATE 
							   ${CMAKE_SOURCE_DIR}/src
							   ${CMAKE_SOURCE_DIR}/globals
							   ${CMAKE_SOURCE_DIR}/game
							   ${CMAKE_SOURCE_DIR}/game/enemies
							   ${CMAKE_SOURCE_DIR}/game/entities
							   ${CMAKE_SOURCE_DIR}/game/map
							   ${CMAKE_SOURCE_DIR}/game/player
							   ${CMAKE_SOURCE_DIR}/game/rendering
							   ${CMAKE_SOURCE_DIR}/game/settings
							   ${CMAKE_SOURCE_DIR}/game/systems
	)
	target_compile_options(${PLAYDATE_GAME_DEVICE} PRIVATE 
		-ffast-math
		-falign-functions=32
		-falign-loops=32
	)
else()
	add_library(${PLAYDATE_GAME_NAME} SHARED ${SOURCES} ${HEADERS})
	target_include_directories(${PLAYDATE_GAME_NAME} PRIVATE 
							   ${CMAKE_SOURCE_DIR}/src
							   ${CMAKE_SOURCE_DIR}/globals
							   ${CMAKE_SOURCE_DIR}/game
							   ${CMAKE_SOURCE_DIR}/game/enemies
							   ${CMAKE_SOURCE_DIR}/game/entities
							   ${CMAKE_SOURCE_DIR}/game/map
							   ${CMAKE_SOURCE_DIR}/game/player
							   ${CMAKE_SOURCE_DIR}/game/rendering
							   ${CMAKE_SOURCE_DIR}/game/settings
							   ${CMAKE_SOURCE_DIR}/game/systems
	)
	target_compile_options(${PLAYDATE_GAME_NAME} PRIVATE 
		-Wall
		-Werror
		-Wno-declaration-after-statement
		-Wno-unsafe-buffer-usage
		-Wdouble-promotion
		-Wno-unused-parameter
		-Wno-implicit-function-declaration
	)
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)
