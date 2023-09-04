cmake_minimum_required( VERSION 3.6 )

set(SRCS
    src/main.cpp
)

set(TARGETNAME OpenGL)

project(${TARGETNAME})

add_executable(${TARGETNAME} ${SRCS})

include(${CMAKE_SOURCE_DIR}/dependencies/SDL2/cmake/sdl2-config.cmake)

# Requires SDL2
add_subdirectory(dependencies/ImGUI)

target_link_libraries(${TARGETNAME} PUBLIC SDL2::SDL2 SDL2::SDL2main)

target_link_libraries(${TARGETNAME} PUBLIC IMGUILib)

target_compile_features(${TARGETNAME} PRIVATE cxx_std_17)

target_include_directories(${TARGETNAME} PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/GLEW/include)

target_include_directories(${TARGETNAME} PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/ImGUI/include)

target_include_directories(${TARGETNAME} PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/)
