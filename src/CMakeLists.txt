
# Add source to this project's executable.
add_executable(vulkanTest
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_initializers.cpp
    vk_initializers.h
    vk_mesh.cpp
    vk_mesh.h
    vk_textures.cpp
    vk_textures.h
    )


set_property(TARGET vulkanTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkanTest>")

target_include_directories(vulkanTest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkanTest vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(vulkanTest Vulkan::Vulkan sdl2)

add_dependencies(vulkanTest Shaders)


find_library(
    FREETYPE_LIB
    NAMES freetype
    HINTS "$ENV{FREETYPE_SDK_PATH}\\release dll\\win64"
    )
SET(FREETYPE_INCLUDES "$ENV{FREETYPE_SDK_PATH}/include" "$ENV{FREETYPE_SDK_PATH}/include/freetype")
target_include_directories(vulkanTest PUBLIC ${FREETYPE_INCLUDES})
target_include_directories(imgui PUBLIC ${FREETYPE_INCLUDES})
target_link_libraries(vulkanTest ${FREETYPE_LIB})