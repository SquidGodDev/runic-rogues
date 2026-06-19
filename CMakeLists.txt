cmake_minimum_required(VERSION 3.21)
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
set(PLAYDATE_GAME_NAME "RunicRogues")
set(PLAYDATE_GAME_DEVICE "RunicRogues")

project(${PLAYDATE_GAME_NAME} C ASM)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.c"
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "src/*.h"
)

file(GLOB_RECURSE IMAGES CONFIGURE_DEPENDS
	"Source/images/*"
	"Source/fonts/*"
)

# Force a relink (which re-runs the pdc POST_BUILD step) whenever an asset
# changes, so editing a PNG/font/etc is picked up without touching C source.
set(ASSETS_TRIGGER "${CMAKE_CURRENT_BINARY_DIR}/assets_trigger.c")
add_custom_command(
	OUTPUT ${ASSETS_TRIGGER}
	COMMAND ${CMAKE_COMMAND} -E touch ${ASSETS_TRIGGER}
	DEPENDS ${IMAGES}
	COMMENT "Asset change detected — re-running pdc"
)

if (TOOLCHAIN STREQUAL "armgcc")
	add_executable(${PLAYDATE_GAME_DEVICE} ${SOURCES} ${HEADERS} ${ASSETS_TRIGGER})
	set(GAME_TARGET ${PLAYDATE_GAME_DEVICE})
	target_compile_options(${GAME_TARGET} PRIVATE
		-ffast-math
		-falign-functions=32
		-falign-loops=32
		-O2
	)
else()
	add_library(${PLAYDATE_GAME_NAME} SHARED ${SOURCES} ${HEADERS} ${ASSETS_TRIGGER})
	set(GAME_TARGET ${PLAYDATE_GAME_NAME})
	target_compile_options(${GAME_TARGET} PRIVATE
		-Wall
		-Werror
		-Wno-declaration-after-statement
		-Wdouble-promotion
		-Wno-unused-parameter
		-Wno-unsafe-buffer-usage
	)
endif()

target_include_directories(${GAME_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(${SDK}/C_API/buildsupport/playdate_game.cmake)
