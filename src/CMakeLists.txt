
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
    )


set_property(TARGET vulkanTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkanTest>")

target_include_directories(vulkanTest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkanTest vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(vulkanTest Vulkan::Vulkan sdl2)

add_dependencies(vulkanTest Shaders)
