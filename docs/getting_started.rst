Getting Started
===============

Requirements
------------

- **C++23** compiler:
  - GCC 14 or 15
  - Clang 18, 19, 20, or 21
  - MSVC 2019 or 2022
- **CMake** ≥ 3.21
- **Conan 2** (for dependency management)

Installation
------------

TypeSafeMessageParser is a header-only library. Simply copy ``src/include/MessageParser.h``
into your project's include path, or use CMake's ``FetchContent``:

.. code-block:: cmake

   include(FetchContent)
   FetchContent_Declare(
       TypeSafeMessageParser
       GIT_REPOSITORY https://github.com/youruser/TypeSafeMessageParser.git
       GIT_TAG main
   )
   FetchContent_MakeAvailable(TypeSafeMessageParser)

   target_link_libraries(your_target PRIVATE TypeSafeMessageParser)

Building from Source
--------------------

1. **Install dependencies** via Conan:

   .. code-block:: bash

      conan install . --build=missing

2. **Configure** with a CMake preset:

   .. code-block:: bash

      cmake --preset unixlike-clang-20-debug

3. **Build**:

   .. code-block:: bash

      cmake --build --preset build-unixlike-clang-20-debug

4. **Test**:

   .. code-block:: bash

      ctest --preset test-unixlike-clang-20-debug

Quick Example
-------------

.. code-block:: cpp

   #include "MessageParser.h"
   #include <array>
   #include <cstdint>

   enum class Status : uint8_t { Ok = 0, Warning = 1, Error = 2 };

   struct MyFields {
       static constexpr auto status = FieldConfiguration<0, Status>{};
       static constexpr auto counter = FieldConfiguration<1, uint16_t,
           FieldRanges::MinMaxRange<uint16_t>{0, 10000}>{};
   };

   int main() {
       constexpr std::array<uint8_t, 3> msg = { 0x01, 0x03, 0xE8 };
       constexpr auto result = MessageParser::convertByteType(msg, MyFields::counter);
       static_assert(result.has_value());
       static_assert(*result == 1000);
   }
