set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.
file(GLOB ENGINE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(vulkanTest ${ENGINE_FILES})

add_dependencies(vulkanTest Shaders)

set_property(TARGET vulkanTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkanTest>")

target_include_directories(vulkanTest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(vulkanTest INTERFACE assetlib)

target_link_libraries(vulkanTest vkbootstrap vma glm tinyobjloader imgui stb_image Vulkan::Vulkan sdl2 freetype assetlib)

add_custom_command(TARGET vulkanTest POST_BUILD
  # COMMAND ${CMAKE_COMMAND} -E echo "DLLs: $<TARGET_RUNTIME_DLLS:vulkanTest>"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:vulkanTest> $<TARGET_FILE_DIR:vulkanTest>
  COMMAND_EXPAND_LISTS
)

# install(FILES $<TARGET_RUNTIME_DLLS:vulkanTest> TYPE BIN)
