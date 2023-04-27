
set(CMAKE_CXX_STANDARD 17)
# Add source to this project's executable.

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(cooker ${SOURCE_FILES})

# set_property(TARGET cooker PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:out>")

set_property(TARGET cooker PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "../assets")

target_include_directories(cooker PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${nvtt_DIR}")

add_library(nvtt SHARED IMPORTED)
find_path(nvtt_DIR
    NAMES "nvtt.h"
    PATHS "../third_party/nvtt")
set_target_properties(nvtt PROPERTIES IMPORTED_LOCATION "${nvtt_DIR}/bin/nvtt30201.dll")
set_target_properties(nvtt PROPERTIES IMPORTED_IMPLIB "${nvtt_DIR}/lib/nvtt30201.lib")

target_link_directories(nvtt INTERFACE "${nvtt_DIR}/lib")
target_link_libraries(nvtt INTERFACE "${nvtt_DIR}/lib/nvtt30201.lib")

target_link_libraries(cooker json lz4 tinyobjloader stb_image assetlib glm nvtt)

add_custom_command(TARGET cooker POST_BUILD
  # COMMAND ${CMAKE_COMMAND} -E echo "COOKER DLLs: $<TARGET_RUNTIME_DLLS:cooker>"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:cooker> $<TARGET_FILE_DIR:cooker>
  COMMAND_EXPAND_LISTS
)