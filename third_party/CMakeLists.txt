find_package(Vulkan REQUIRED)

add_library(vkbootstrap STATIC)
add_library(glm INTERFACE)
add_library(vma INTERFACE)

add_library(stb_image INTERFACE)
add_library(json INTERFACE)
add_library(tinyobjloader STATIC)
#add_library(spirv_reflect STATIC)

#add_library(fmt_lib STATIC)

add_library(lz4 STATIC)

target_sources(vkbootstrap PRIVATE 
    vkbootstrap/VkBootstrap.h
    vkbootstrap/VkBootstrap.cpp
    )

target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

#both vma and glm and header only libs so we only need the include path
target_include_directories(vma INTERFACE vma)
target_include_directories(glm INTERFACE glm)

target_sources(tinyobjloader PRIVATE 
    tinyobjloader/tiny_obj_loader.h
    tinyobjloader/tiny_obj_loader.cc
    )

target_include_directories(tinyobjloader PUBLIC tinyobjloader)


add_library(sdl2 INTERFACE)
#set(sdl2_DIR "SDL_PATH" CACHE PATH "Path to SDL2")
set(sdl2_DIR SDL2 CACHE PATH "Path to SDL2")


if (WIN32)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include ${sdl2_image_DIR}/include)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
elseif (UNIX)
target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
endif()

target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)




add_library(freetype INTERFACE)
set(freetype_DIR freetype CACHE PATH "Path to Freetype")
find_path(freetype_INCLUDE
    NAMES "ft2build.h"
    PATHS "${freetype_DIR}/include")
find_path(freetype_LIB
    NAMES "freetype.lib"
    PATHS "${freetype_DIR}/lib")

if (WIN32)
target_include_directories(freetype SYSTEM INTERFACE ${freetype_INCLUDE} ${freetype_INCLUDE}/freetype)
target_link_directories(freetype INTERFACE ${freetype_LIB})
elseif (UNIX)
#target_include_directories(freetype INTERFACE ${freetype_DIR}/include ${freetype_DIR}/include/freetype)
#target_link_directories(freetype INTERFACE ${freetype_DIR}/lib)
endif()

target_link_libraries(freetype INTERFACE freetype.lib)




add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)
target_include_directories(imgui PUBLIC ${freetype_INCLUDE} ${freetype_INCLUDE}/freetype)

target_sources(imgui PRIVATE 
    imgui/imgui.h
    imgui/imgui.cpp

    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp

    imgui/misc/freetype/imgui_freetype.cpp

    imgui/backend/imgui_impl_vulkan.cpp
    imgui/backend/imgui_impl_sdl.cpp
    )

target_link_libraries(imgui PUBLIC Vulkan::Vulkan sdl2 freetype)

target_include_directories(stb_image INTERFACE stb_image)

target_sources(lz4 PRIVATE
    lz4/lz4.h
    lz4/lz4.c
)

target_include_directories(lz4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lz4")

target_include_directories(json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json")