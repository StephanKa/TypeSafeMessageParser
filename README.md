# Type Safe Message Parser

A **header-only, compile-time type-safe message parser library for C++23**. It parses raw byte arrays — such as those received from CAN bus, serial lines, or other embedded communication protocols — into strongly-typed C++ values with validated ranges, all evaluated at compile time where possible.

## Features

- **Header-only** — just include `MessageParser.h`, no compilation required
- **Compile-time parsing** — all parsing and validation can be performed via `constexpr`/`consteval`, catching errors at build time instead of runtime
- **Type-safe field mapping** — fields are declared with an explicit type (`uint8_t`, `uint16_t`, `enum`, …) and a byte index; the parser reads and casts accordingly
- **Range validation** — every field can carry a `MinMaxRange<T>` (inclusive min/max) or a `SpecificRange<V...>` (exact allowed values); validation returns `std::expected<T, ParseError>` so errors are always handled explicitly
- **Enum support** — enum values are first-class citizens; a field typed as an enum is read from raw bytes and validated through the same range mechanism
- **Multi-byte fields** — fields wider than one byte (e.g., `uint16_t`) are read in big-endian order
- **Static size checks** — `parseMessage()` `static_assert`s at compile time that the sum of all field sizes equals the actual array size, preventing buffer over/under-reads
- **Rich error information** — `ParseError` distinguishes `ValueNotExist`, `BelowRange`, and `AboveRange`

## Usage

```cpp
#include "MessageParser.h"

using namespace MessageParser;
using namespace FieldRanges;

// 1. Define the types your message fields carry
enum class Status : uint8_t { Ok = 0, Warning = 1, Error = 2 };
enum class Motor  : uint8_t { Off = 0, On = 1 };

// 2. Declare field configurations (byte index, type, optional range)
struct Fields {
    static constexpr FieldConfiguration<0, Status> status{};
    static constexpr FieldConfiguration<1, Motor>  motor{};
    static constexpr FieldConfiguration<2, uint16_t, MinMaxRange<uint16_t>{1000, 65000}> rpm{};
};

// 3. Parse a raw message (compile-time or runtime)
constexpr std::array<uint8_t, 4> msg = { 0x01, 0x01, 0x10, 0xE8 };

auto result = convertByteType(msg, Fields::rpm);
if (result) {
    uint16_t rpmValue = *result; // 4328, validated to be in [1000, 65000]
}
```

## API Overview

| Function / Type | Description |
|---|---|
| `FieldConfiguration<Index, T, Range>` | Declares a field: byte index, value type, and optional range constraint |
| `MinMaxRange<T>{min, max}` | Inclusive minimum/maximum range |
| `SpecificRange<V...>{}` | Exact set of allowed values |
| `ParseError` | Enum: `ValueNotExist`, `BelowRange`, `AboveRange` |
| `convertByteType(msg, field)` | Reads the field from `msg`, validates its range, returns `std::expected<T, ParseError>` |
| `getField<Field>(msg)` | Reads a single byte and casts it to the field's type (no range check) |
| `parseMessage(msg, fields...)` | Validates that all field sizes add up to the message size at compile time |
| `getSize<Fields...>()` | Returns the total byte size of all given fields at compile time |

## Requirements

- **C++23** compiler (GCC 14, Clang 17/18/19, MSVC 2019/2022)
- **CMake** ≥ 3.21
- **Conan 2** (for dependency management)

## Build Instructions

All defined presets have the following scheme:

| Preset stage  | scheme                    | description                                                              |
|---------------|---------------------------|--------------------------------------------------------------------------|
| build         | **build-**\<PRESET_NAME\> | This stage is used for compiling the project                             |
| configuration | \<PRESET_NAME\>           | This stage is used for configure the project with defined compiler setup |
| test          | **test-**\<PRESET_NAME\>  | This stage is used to run all test registered for ctest                  |

### Configure your build

To configure the project and write makefiles, you could use `cmake` with a bunch of command line options.
The easier option is to run cmake interactively:

#### **Configure via cmake preset**:

Check the preset which can be applied to your build system by typing:

    cmake --list-presets

The output looks like this:

    Available configure presets:

    "unixlike-gcc-14-debug"       - GCC 14 Debug
    "unixlike-gcc-14-release"     - GCC 14 Release
    "unixlike-clang-17-debug"     - Clang 17 Debug
    "unixlike-clang-17-release"   - Clang 17 Release
    "unixlike-clang-18-debug"     - Clang 18 Debug
    "unixlike-clang-18-release"   - Clang 18 Release
    "unixlike-clang-19-debug"     - Clang 19 Debug
    "unixlike-clang-19-release"   - Clang 19 Release

Choose a configuration which is suitable and use following command for example.

    cmake --preset unixlike-clang-17-debug

### Build
Once you have selected all the options you would like to use, you can build the
project (all targets):

    cmake --build --preset <PRESET_NAME>

For example:

    cmake --build --preset build-unixlike-clang-17-debug

### Test
Run all tests using a test preset and ctest:

    ctest --preset <PRESET_NAME>

For example:

    ctest --preset test-unixlike-clang-17-debug

## Testing

- See [Catch2 tutorial](https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md)
