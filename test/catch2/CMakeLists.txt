FIND_PACKAGE(Catch2 REQUIRED)
FIND_PACKAGE(spdlog REQUIRED)


INCLUDE(CTest)
INCLUDE(Catch)

# Add a file containing a set of constexpr tests
ADD_EXECUTABLE(tests tests.cpp)
TARGET_LINK_LIBRARIES(tests PRIVATE project_options project_warnings Catch2::Catch2WithMain spdlog::spdlog_header_only)
TARGET_INCLUDE_DIRECTORIES(tests PRIVATE ${PROJECT_SOURCE_DIR}/src/include)

CATCH_DISCOVER_TESTS(
        tests
        TEST_PREFIX
        "tests."
        REPORTER
        JUnit
        OUTPUT_DIR
        .
        OUTPUT_PREFIX
        "tests."
        OUTPUT_SUFFIX
        .xml)
