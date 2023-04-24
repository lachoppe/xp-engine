set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
file(GLOB ENGINE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(vulkanTest ${ENGINE_FILES})

add_dependencies(vulkanTest Shaders)

set_property(TARGET vulkanTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkanTest>")

target_include_directories(vulkanTest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkanTest vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(vulkanTest Vulkan::Vulkan sdl2 freetype assetlib)

