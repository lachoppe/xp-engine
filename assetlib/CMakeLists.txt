
set(CMAKE_CXX_STANDARD 17)
# Add source to this project's executable.

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(assetlib ${SOURCE_FILES})


target_include_directories(assetlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(assetlib PRIVATE json lz4)
