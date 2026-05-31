# Type Safe Message Parser

A **header-only, compile-time type-safe message parser library for C++23**. It parses raw byte arrays — such as those received from CAN bus, serial lines, or other embedded communication protocols — into strongly-typed C++ values with validated ranges, all evaluated at compile time where possible.

## Features

- **Header-only** — just include `MessageParser.h`, no compilation required
- **Compile-time parsing** — all parsing and validation can be performed via `constexpr`/`consteval`, catching errors at build time instead of runtime
- **Type-safe field mapping** — fields are declared with an explicit type (`uint8_t`, `uint16_t`, `enum`, …) and a byte index; the parser reads and casts accordingly
- **Range validation** — every field can carry a `MinMaxRange<T>` (inclusive min/max) or a `SpecificRange<V...>` (exact allowed values); validation returns `std::expected<T, ParseError>` so errors are always handled explicitly
- **Enum support** — enum values are first-class citizens; a field typed as an enum is read from raw bytes and validated through the same range mechanism
- **Multi-byte fields** — fields wider than one byte (e.g., `uint16_t`, `uint32_t`) are supported
- **Byte order control** — fields can be parsed as **big-endian** (default) or **little-endian** via the `ByteOrder` template parameter
- **Static size checks** — `parseMessage()` `static_assert`s at compile time that the sum of all field sizes equals the actual array size, preventing buffer over/under-reads
- **Rich error information** — `ParseError` distinguishes `ValueNotExist`, `BelowRange`, `AboveRange`, and `InvalidSize`
- **Batch operations** — `convertAll()` parses all fields at once returning a tuple; `validateMessage()` checks all fields in one call
- **`to_string` for errors** — `FieldRanges::to_string(ParseError)` provides human-readable error descriptions
- **C++23 utilities** — uses `std::to_underlying`, `consteval`, concepts, and `std::expected`

## Usage

```cpp
#include "MessageParser.h"

using namespace MessageParser;
using namespace FieldRanges;

// 1. Define the types your message fields carry
enum class Status : uint8_t { Ok = 0, Warning = 1, Error = 2 };
enum class Motor  : uint8_t { Off = 0, On = 1 };

// 2. Declare field configurations (byte index, type, optional range, optional byte order)
struct Fields {
    static constexpr auto status = FieldConfiguration<0, Status>{};
    static constexpr auto motor  = FieldConfiguration<1, Motor>{};
    static constexpr auto rpm    = FieldConfiguration<2, uint16_t, MinMaxRange<uint16_t>{1000, 65000}>{};
    // Little-endian sensor reading
    static constexpr auto sensor = FieldConfiguration<4, uint16_t,
        MinMaxRange<uint16_t>{0, 4095}, ByteOrder::LittleEndian>{};
};

// 3. Parse a raw message (compile-time or runtime)
constexpr std::array<uint8_t, 6> msg = { 0x01, 0x01, 0x10, 0xE8, 0x34, 0x12 };

// Parse a single field with range validation
auto result = convertByteType(msg, Fields::rpm);
if (result) {
    uint16_t rpmValue = *result; // 4328, validated to be in [1000, 65000]
}

// Validate entire message at compile time
static_assert(validateMessage(msg, Fields::status, Fields::motor, Fields::rpm, Fields::sensor));

// Parse all fields at once
auto [status, motor, rpm, sensor] = convertAll(msg, Fields::status, Fields::motor, Fields::rpm, Fields::sensor);
```

## API Overview

| Function / Type | Description |
|---|---|
| `FieldConfiguration<Index, T, Range, Order>` | Declares a field: byte index, value type, optional range constraint, optional byte order |
| `MinMaxRange<T>{min, max}` | Inclusive minimum/maximum range |
| `SpecificRange<V...>{}` | Exact set of allowed values |
| `ByteOrder::BigEndian` / `ByteOrder::LittleEndian` | Byte order for multi-byte fields (default: BigEndian) |
| `ParseError` | Enum: `ValueNotExist`, `BelowRange`, `AboveRange`, `InvalidSize` |
| `to_string(ParseError)` | Returns a `std::string_view` description of the error |
| `convertByteType(msg, field)` | Reads the field from `msg`, validates its range, returns `std::expected<T, ParseError>` |
| `getField<Field>(msg)` | Reads a single byte and casts it to the field's type (no range check) |
| `parseMessage(msg, fields...)` | Validates that all field sizes add up to the message size at compile time |
| `getSize<Fields...>()` | Returns the total byte size of all given fields at compile time |
| `convertAll(msg, fields...)` | Parses all fields, returns `std::tuple` of `std::expected` results |
| `validateMessage(msg, fields...)` | Returns `true` if all fields pass range validation |
| `toUnderlying(value)` | Converts enum to underlying type using `std::to_underlying`, pass-through for non-enums |
| `IsFieldConfiguration<T>` | Concept constraining valid field configuration types |

## Requirements

- **C++23** compiler:
  - GCC 14 / 15
  - Clang 18 / 19 / 20 / 21
  - MSVC 2019 / 2022
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
    "unixlike-gcc-15-debug"       - GCC 15 Debug
    "unixlike-gcc-15-release"     - GCC 15 Release
    "unixlike-clang-18-debug"     - Clang 18 Debug
    "unixlike-clang-18-release"   - Clang 18 Release
    "unixlike-clang-19-debug"     - Clang 19 Debug
    "unixlike-clang-19-release"   - Clang 19 Release
    "unixlike-clang-20-debug"     - Clang 20 Debug
    "unixlike-clang-20-release"   - Clang 20 Release
    "unixlike-clang-21-debug"     - Clang 21 Debug
    "unixlike-clang-21-release"   - Clang 21 Release
    "windows-2022-msvc-debug"     - Visual Studio 17 2022 Debug
    "windows-2022-msvc-release"   - Visual Studio 17 2022 Release

Choose a configuration which is suitable and use following command for example.

    cmake --preset unixlike-clang-20-debug

### Build

Once you have selected all the options you would like to use, you can build the
project (all targets):

    cmake --build --preset <PRESET_NAME>

For example:

    cmake --build --preset build-unixlike-clang-20-debug

### Test

Run all tests using a test preset and ctest:

    ctest --preset <PRESET_NAME>

For example:

    ctest --preset test-unixlike-clang-20-debug

## Compiler Warnings

The project enables an extensive set of compiler warnings for all supported compilers:

- **MSVC**: `/W4` + 20+ specific warnings + `/permissive-` + `/analyze:external-`
- **Clang**: `-Wall -Wextra -Wshadow -Wconversion -Wpedantic -Wlifetime -Wextra-semi` and more
- **GCC**: All Clang warnings + `-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast -Wcast-qual -Wredundant-decls`

All warnings are treated as errors by default (`WARNINGS_AS_ERRORS=ON`).

## Static Analysis & Sanitizers

Enable static analysis:

    cmake --preset unixlike-clang-19-debug-static-analysis

This enables `cppcheck`, `clang-tidy`, and `include-what-you-use` in a single configuration.

Available sanitizers (via CMake options):
- `ENABLE_SANITIZER_ADDRESS` — Address Sanitizer (ASan)
- `ENABLE_SANITIZER_UNDEFINED_BEHAVIOR` — UBSan
- `ENABLE_SANITIZER_THREAD` — Thread Sanitizer (TSan)
- `ENABLE_SANITIZER_MEMORY` — Memory Sanitizer (MSan, Clang only)
- `ENABLE_SANITIZER_LEAK` — Leak Sanitizer (LSan)

## Testing

The project uses [Catch2](https://github.com/catchorg/Catch2) v3 with 60+ test cases covering:

- Field size calculations
- MinMaxRange boundary validation (exact min, exact max, below, above)
- SpecificRange validation (single value, enum values, boundary values)
- Multi-byte big-endian and little-endian parsing
- Enum field parsing with underlying type conversion
- `validateMessage` and `convertAll` batch operations
- `ParseError` string conversion
- `IsFieldConfiguration` concept validation
- End-to-end multi-field message parsing

All parsing tests use `STATIC_REQUIRE` to verify compile-time evaluation.

## Documentation

Full Sphinx documentation with diagrams is available under `docs/`. To build:

```bash
cd docs
pip install -r requirements.txt
make html
```

Open `docs/_build/html/index.html` in your browser.

## License

MIT — see [LICENSE](LICENSE)
