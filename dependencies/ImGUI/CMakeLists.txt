find_package(OpenGL REQUIRED)

set(SRCS
        src/imgui.cpp
        src/imgui_draw.cpp
        src/imgui_impl_opengl3.cpp
        src/imgui_impl_sdl.cpp
        src/imgui_tables.cpp
        src/imgui_widgets.cpp)

add_library(IMGUILib ${SRCS})
target_include_directories(IMGUILib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(IMGUILib PUBLIC SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(IMGUILib PUBLIC opengl32)