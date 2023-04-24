
set(CMAKE_CXX_STANDARD 17)
# Add source to this project's executable.

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(cooker ${SOURCE_FILES})

# set_property(TARGET cooker PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:out>")

set_property(TARGET cooker PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "../assets")

target_include_directories(cooker PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(cooker PRIVATE json lz4 tinyobjloader stb_image assetlib glm)
