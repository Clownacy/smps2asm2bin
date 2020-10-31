cmake_minimum_required(VERSION 3.7.2)

if((${CMAKE_VERSION} VERSION_EQUAL 3.9) OR (${CMAKE_VERSION} VERSION_GREATER 3.9))
	cmake_policy(SET CMP0069 NEW)
endif()

project(smps2asm2bin LANGUAGES C)

add_executable(smps2asm2bin
	"common.c"
	"common.h"
	"dictionary.c"
	"dictionary.h"
	"error.c"
	"error.h"
	"instruction.c"
	"instruction.h"
	"main.c"
	"memory_stream.c"
	"memory_stream.h"
	"smps2asm2bin.c"
	"smps2asm2bin.h"
)

set_target_properties(smps2asm2bin PROPERTIES
	C_STANDARD 99
	C_EXTENSIONS OFF
)

# MSVC tweak
if(MSVC)
	target_compile_definitions(smps2asm2bin PRIVATE _CRT_SECURE_NO_WARNINGS)	# Shut up those stupid warnings
endif()
