CMAKE_MINIMUM_REQUIRED(VERSION 3.21)

INCLUDE(cmake/Conan.cmake)

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
SET(CMAKE_CXX_EXTENSIONS OFF)

INCLUDE(cmake/Options.cmake)

# Set the project name to your project name, my project isn't very descriptive
PROJECT(TypeSafeMessageParser
        LANGUAGES CXX
        VERSION 0.0.1)

IF(NOT DEFINED CXX_STANDARD)
    SET(CXX_STANDARD 23)
ENDIF()

SET(CMAKE_CXX_STANDARD ${CXX_STANDARD})

INCLUDE(cmake/BuildingConfig.cmake)
SETUP_MULTI_CONFIG()

INCLUDE(cmake/StandardProjectSettings.cmake)
INCLUDE(cmake/PreventInSourceBuilds.cmake)
INCLUDE(cmake/CodeFormat.cmake)
INCLUDE(cmake/InterproceduralOptimization.cmake)
ENABLE_IPO()

# fetch git hash information for configure version template file
INCLUDE(cmake/GitInformation.cmake)
GET_GIT_HASH()

CONFIGURE_FILE("templates/version.hpp.in" "${CMAKE_BINARY_DIR}/generated/include/version.hpp" ESCAPE_QUOTES)

# Link this 'library' to set the c++ standard / compile-time options requested
ADD_LIBRARY(project_options INTERFACE)
TARGET_COMPILE_FEATURES(project_options INTERFACE cxx_std_${CXX_STANDARD})

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
ADD_LIBRARY(project_warnings INTERFACE)

# enable cache system
INCLUDE(cmake/Cache.cmake)

# standard compiler warnings
INCLUDE(cmake/CompilerWarnings.cmake)
SET_PROJECT_WARNINGS(project_warnings)

# sanitizer options if supported by compiler
INCLUDE(cmake/Sanitizers.cmake)
ENABLE_SANITIZERS(project_options)

# enable doxygen
INCLUDE(cmake/Doxygen.cmake)
ENABLE_DOXYGEN()

# allow for static analysis options
INCLUDE(cmake/StaticAnalyzers.cmake)

# enabled precompiled headers
INCLUDE(cmake/PrecompiledHeader.cmake)
ENABLE_PCH()

ADD_SUBDIRECTORY(src)

IF(ENABLE_TESTING)
    ENABLE_TESTING()
    MESSAGE("Building Tests. Be sure to check out test/constexpr_tests for constexpr testing")
    ADD_SUBDIRECTORY(test)
ENDIF()

# Documentation and tooling targets
FIND_PACKAGE(Python3 COMPONENTS Interpreter QUIET)
IF(ENABLE_SPHINX)
    IF(Python3_FOUND)
        ADD_CUSTOM_TARGET(sphinx-docs
            COMMAND ${Python3_EXECUTABLE} -m sphinx -M html ${CMAKE_SOURCE_DIR}/docs ${CMAKE_SOURCE_DIR}/docs/_build
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating Sphinx documentation in docs/_build/html"
            VERBATIM
        )
    ELSE()
        MESSAGE(STATUS "Python3 was not found. Target `sphinx-docs` is unavailable.")
    ENDIF()
ENDIF()

ADD_CUSTOM_TARGET(docs
    COMMENT "Build all available documentation targets"
)

SET(_HAS_DOC_TARGET FALSE)
IF(TARGET sphinx-docs)
    ADD_DEPENDENCIES(docs sphinx-docs)
    SET(_HAS_DOC_TARGET TRUE)
ENDIF()
IF(TARGET doxygen-docs)
    ADD_DEPENDENCIES(docs doxygen-docs)
    SET(_HAS_DOC_TARGET TRUE)
ENDIF()
IF(NOT _HAS_DOC_TARGET)
    ADD_CUSTOM_COMMAND(TARGET docs
        COMMAND ${CMAKE_COMMAND} -E echo "No documentation targets available. Enable ENABLE_SPHINX and/or ENABLE_DOXYGEN."
    )
ENDIF()

# Single-header amalgamation target
IF(Python3_FOUND)
    ADD_CUSTOM_TARGET(single_header
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/amalgamate.py
            --input ${CMAKE_SOURCE_DIR}/src/include/MessageParser.h
            --output ${CMAKE_SOURCE_DIR}/single_include/MessageParser.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating single-header amalgamation"
        VERBATIM
    )
ENDIF()
