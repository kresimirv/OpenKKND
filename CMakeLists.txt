cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)

set(VCPKG_ROOT "$ENV{VCPKG_ROOT}")
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# vcpkg appends debug dlls with -d 
set(CMAKE_DEBUG_POSTFIX, "d")

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

message("-------------------------")
message("vcpkg:")
message("VCPKG_ROOT=${VCPKG_ROOT}")
message("VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
message("CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

project("OpenKKND")
set(PROJECT_VERSION "0.2.0")




if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    #debug include files
    set(CMAKE_CXX_FLAGS " /showIncludes ${CMAKE_CXX_FLAGS} ")

    #disable pesky warnings
    set(CMAKE_CXX_FLAGS " /D_CRT_SECURE_NO_WARNINGS ${CMAKE_CXX_FLAGS} ")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")


message("-------------------------")
message("SDL2:")
find_path(SDL2_INCLUDE_DIR SDL2/SDL.h)

if (CMAKE_BUILD_TYPE MATCHES "^Debug$" OR NOT DEFINED CMAKE_BUILD_TYPE)
    message("SDL2 Debug")
    find_library(SDL2_LIBRARY SDL2d)
    find_library(SDL2_MAIN SDL2maind)
else()
    message("SDL2 Release")
    find_library(SDL2_LIBRARY SDL2)
    find_library(SDL2_MAIN SDL2main)
endif()
message("SDL2_INCLUDE_DIR=${SDL2_INCLUDE_DIR}")
message("SDL2_LIBRARY=${SDL2_LIBRARY}")
message("SDL2_MAIN=${SDL2_MAIN}")

message("-------------------------")
message("Include directories:")
include_directories(${INCLUDE_DIRECTORIES} ./)
include_directories(${INCLUDE_DIRECTORIES} ./src)
include_directories(${INCLUDE_DIRECTORIES} ${SDL2_INCLUDE_DIR})

# list include directories
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "Include dir='${dir}'")
endforeach()

add_subdirectory(src)

message("-------------------------")



add_executable(OpenKKND ./src/Application/Game.cpp)

target_link_libraries(OpenKKND
    Application
    Engine
    Infrastructure
    Audio
    Video
    Unsorted
    ${SDL2_LIBRARY} ${SDL2_MAIN}
)


set(COMPILE_OPTIONS_DEBUG "")
set(COMPILE_OPTIONS_RELEASE "")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMPILE_OPTIONS_DEBUG "-std=gnu++11 ${COMPILE_OPTIONS_DEBUG}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(COMPILE_OPTIONS_DEBUG "-std=c++11 -stdlib=libc++ ${COMPILE_OPTIONS_DEBUG}")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
endif()

target_compile_options(OpenKKND PUBLIC "$<$<CONFIG:DEBUG>:${COMPILE_OPTIONS_DEBUG}>")
target_compile_options(OpenKKND PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_OPTIONS_RELEASE}>")


# ----------------------
# copy assets
file(TO_NATIVE_PATH "${PROJECT_SOURCE_DIR}/assets" ASSETS_SRC_DIR)
file(TO_NATIVE_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets" ASSETS_OUT_DIR)
add_custom_command(TARGET OpenKKND
    POST_BUILD
    COMMAND xcopy ${ASSETS_SRC_DIR} ${ASSETS_OUT_DIR} /S /E /Y /I
)
