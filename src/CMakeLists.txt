FIND_PACKAGE(spdlog REQUIRED)

# Generic test that uses conan libs
ADD_EXECUTABLE(example main.cpp)
TARGET_LINK_LIBRARIES(
        example
        PRIVATE project_options
        project_warnings
        spdlog::spdlog_header_only)

TARGET_INCLUDE_DIRECTORIES(example PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR}/include)
