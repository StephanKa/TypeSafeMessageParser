# Type Safe Message Parser

A **header-only, compile-time type-safe message parser library for C++23**. It parses raw byte arrays — such as those received from CAN bus, serial lines, or other embedded communication protocols — into strongly-typed C++ values with validated ranges, all evaluated at compile time where possible.

## Features

- **Header-only** — just include `MessageParser.h`, no compilation required
- **Compile-time parsing** — all parsing and validation can be performed via `constexpr`/`consteval`, catching errors at build time instead of runtime
- **Type-safe field mapping** — fields are declared with an explicit type (`uint8_t`, `uint16_t`, `enum`, …) and a byte index; the parser reads and casts accordingly
- **Range validation** — every field can carry a `MinMaxRange<T>` (inclusive min/max) or a `SpecificRange<V...>` (exact allowed values); validation returns `std::expected<T, ParseError>` so errors are always handled explicitly
- **Custom validator predicates** — supply a `constexpr` lambda as a `CustomRange` to implement arbitrary validation logic (e.g., "must be a multiple of 4")
- **Cross-field validation** — validate constraints that depend on multiple fields using `validateCrossField()` / `validateCrossFields()`
- **Optional/conditional fields** — `convertByteTypeIf()` parses a field only when a runtime condition is met
- **Enum support** — enum values are first-class citizens; a field typed as an enum is read from raw bytes and validated through the same range mechanism
- **Signed integer support** — `int8_t`, `int16_t`, `int32_t` fields are parsed correctly via two's complement
- **Floating-point fields** — `getFloatField()` / `getDoubleField()` parse IEEE 754 `float`/`double` from raw bytes
- **Bit-field support** — `BitFieldConfiguration` allows parsing fields that span partial bytes (e.g., 3-bit flag at bit offset 5)
- **Multi-byte fields** — fields wider than one byte (e.g., `uint16_t`, `uint32_t`) are supported
- **Byte order control** — fields can be parsed as **big-endian** (default) or **little-endian** via the `ByteOrder` template parameter
- **Message serialization (encode)** — `encodeField()` and `encodeFieldChecked()` write typed values back into byte arrays with the same compile-time safety
- **CRC/checksum verification** — built-in `computeCrc8()`, `computeCrc16()`, `computeCrc32()`, and `verifyCrc8()`/`verifyCrc16()` for integrity checking
- **Message framing** — `FrameDefinition` + `buildFrame()` / `parseFrame()` handle header/length/payload/CRC/trailer structure
- **Message ID dispatch** — compile-time `MessageRegistry` maps message IDs to type-safe parse handlers
- **Named fields** — `NamedFieldConfiguration` attaches a `string_view` name for debug printing and logging
- **Reflection / field iteration** — `forEachField()`, `forEachFieldIndexed()`, `fieldCount()` for iterating over field definitions
- **`std::span` support** — all parsing functions accept `std::span<const uint8_t>` in addition to `std::array`
- **`std::format` integration** — `ParseError` has a `std::formatter` specialization for use with `std::format`
- **`operator<<` overload** — stream `ParseError` to any `std::ostream`
- **Static size checks** — `parseMessage()` `static_assert`s at compile time that the sum of all field sizes equals the actual array size, preventing buffer over/under-reads
- **Rich error information** — `ParseError` distinguishes `ValueNotExist`, `BelowRange`, `AboveRange`, `InvalidSize`, `ChecksumMismatch`, and `CustomValidationFailed`
- **Batch operations** — `convertAll()` parses all fields at once returning a tuple; `validateMessage()` checks all fields in one call
- **`to_string` for errors** — `FieldRanges::to_string(ParseError)` provides human-readable error descriptions
- **Single-header packaging** — CMake target `single_header` produces a single amalgamated header for easy distribution
- **Benchmark suite** — Catch2 benchmarks comparing parsing overhead across all operations
- **C++23 utilities** — uses `std::to_underlying`, `consteval`, concepts, `std::expected`, and `std::bit_cast`

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

// 4. Custom validator predicate
constexpr auto isMultOf4 = [](auto val) constexpr { return val % 4 == 0; };
static constexpr auto aligned = FieldConfiguration<0, uint8_t,
    CustomRange<uint8_t, decltype(isMultOf4)>{ isMultOf4 }>{};

// 5. Encode values back into a byte array
std::array<uint8_t, 6> outMsg{};
encodeField(outMsg, Fields::rpm, uint16_t{5000});
auto encResult = encodeFieldChecked(outMsg, Fields::rpm, uint16_t{5000}); // with range check

// 6. Bit-field parsing (3 bits starting at bit offset 2 in byte 0)
static constexpr auto flags = BitFieldConfiguration<0, 2, 3, uint8_t>{};
auto bitResult = convertBitField(msg, flags);

// 7. Float/double parsing
constexpr std::array<uint8_t, 4> floatMsg = { 0x40, 0x48, 0xF5, 0xC3 };
auto temp = getFloatField<0, ByteOrder::BigEndian>(floatMsg); // 3.14f

// 8. CRC verification
auto crcOk = verifyCrc8<0, 4, 4>(msg); // verify CRC-8 of bytes [0..3] against byte[4]

// 9. Message framing
using MyFrame = FrameDefinition<0xAA, 0x55, 32, true>; // header=0xAA, trailer=0x55, CRC enabled
std::array<uint8_t, 3> payload = { 0x01, 0x02, 0x03 };
auto frame = buildFrame<MyFrame>(payload);
auto parsed = parseFrame<MyFrame>(std::span<const uint8_t>(frame));

// 10. Cross-field validation
auto crossValid = validateCrossField(msg, [](const auto &m) {
    auto s = convertByteType(m, Fields::status);
    auto r = convertByteType(m, Fields::rpm);
    if (s.has_value() && *s == Status::Error) return r.has_value() && *r == 0;
    return true;
});

// 11. std::span support — all functions also accept span
std::span<const uint8_t> spanMsg(msg);
auto spanResult = convertByteType(spanMsg, Fields::rpm);

// 12. std::format integration
auto errStr = std::format("Error: {}", ParseError::BelowRange); // "Error: BelowRange"

// 13. Named fields for debugging
static constexpr auto namedRpm = NamedFieldConfiguration<2, uint16_t,
    MinMaxRange<uint16_t>{1000, 65000}>{ .name = "engine_rpm" };

// 14. Field iteration / reflection
forEachField([](const auto &f) { /* process each field */ },
    Fields::status, Fields::motor, Fields::rpm, Fields::sensor);
```

## API Overview

### Core Types & Parsing

| Function / Type | Description |
|---|---|
| `FieldConfiguration<Index, T, Range, Order>` | Declares a field: byte index, value type, optional range constraint, optional byte order |
| `BitFieldConfiguration<ByteIdx, BitOff, BitWidth, T, Range>` | Declares a bit-level field within a byte |
| `NamedFieldConfiguration<Index, T, Range, Order>` | Field with an attached `string_view` name for debugging |
| `MinMaxRange<T>{min, max}` | Inclusive minimum/maximum range |
| `SpecificRange<V...>{}` | Exact set of allowed values |
| `CustomRange<T, Pred>{predicate}` | Custom validator — any `constexpr` callable returning `bool` |
| `ByteOrder::BigEndian` / `ByteOrder::LittleEndian` | Byte order for multi-byte fields (default: BigEndian) |
| `ParseError` | Enum: `ValueNotExist`, `BelowRange`, `AboveRange`, `InvalidSize`, `ChecksumMismatch`, `CustomValidationFailed` |
| `to_string(ParseError)` | Returns a `std::string_view` description of the error |
| `convertByteType(msg, field)` | Reads the field from `msg`, validates its range, returns `std::expected<T, ParseError>` |
| `convertBitField(msg, field)` | Reads a bit-field from `msg` with range validation |
| `getField<Field>(msg)` | Reads a single byte and casts it to the field's type (no range check) |
| `getFloatField<Index, Order>(msg)` | Parses an IEEE 754 `float` from raw bytes |
| `getDoubleField<Index, Order>(msg)` | Parses an IEEE 754 `double` from raw bytes |
| `parseMessage(msg, fields...)` | Validates that all field sizes add up to the message size at compile time |
| `getSize<Fields...>()` | Returns the total byte size of all given fields at compile time |
| `convertAll(msg, fields...)` | Parses all fields, returns `std::tuple` of `std::expected` results |
| `validateMessage(msg, fields...)` | Returns `true` if all fields pass range validation |
| `convertByteTypeIf(msg, field, cond)` | Conditionally parse a field; returns `nullopt` if condition is false |
| `toUnderlying(value)` | Converts enum to underlying type using `std::to_underlying`, pass-through for non-enums |
| `IsFieldConfiguration<T>` | Concept constraining valid field configuration types |
| `IsBitFieldConfiguration<T>` | Concept constraining valid bit-field configuration types |

### Serialization (Encode)

| Function | Description |
|---|---|
| `encodeField(msg, field, value)` | Write a typed value into a byte array at the field's position |
| `encodeFieldChecked(msg, field, value)` | Same as above with range validation, returns `std::expected<void, ParseError>` |
| `encodeFloatField<Index, Order>(msg, value)` | Encode a `float` into message bytes |
| `encodeDoubleField<Index, Order>(msg, value)` | Encode a `double` into message bytes |

### Cross-field & Conditional Validation

| Function | Description |
|---|---|
| `validateCrossField(msg, predicate)` | Validate with a predicate that receives the full message |
| `validateCrossFields(msg, preds...)` | Multiple cross-field predicates; all must pass |

### CRC / Checksum

| Function | Description |
|---|---|
| `computeCrc8(data)` | CRC-8 (polynomial 0x07) |
| `computeCrc16(data)` | CRC-16 CCITT (polynomial 0x1021, init 0xFFFF) |
| `computeCrc32(data)` | CRC-32 (reflected, polynomial 0xEDB88320) |
| `computeChecksum8(data)` | Simple 8-bit sum checksum |
| `verifyCrc8<start, len, crcIdx>(msg)` | Verify CRC-8 at a given position |
| `verifyCrc16<start, len, crcIdx, Order>(msg)` | Verify CRC-16 at a given position |

### Message Framing

| Function / Type | Description |
|---|---|
| `FrameDefinition<Header, Trailer, MaxPayload, HasCrc>` | Define a frame format |
| `buildFrame<Frame>(payload)` | Construct a framed message from payload bytes |
| `parseFrame<Frame>(data)` | Validate frame and extract payload `std::span` |

### Message ID Dispatch

| Function / Type | Description |
|---|---|
| `MessageEntry<IdType, Id, ParseFunc>` | Maps a message ID to a parse handler |
| `MessageRegistry<IdType, Entries...>` | Compile-time registry; call `.dispatch(id, payload)` |
| `makeRegistry<IdType>(entries...)` | Helper to construct a registry |

### Reflection & Ergonomics

| Function | Description |
|---|---|
| `forEachField(func, fields...)` | Invoke callable for each field |
| `forEachFieldIndexed(func, fields...)` | Invoke callable with index for each field |
| `fieldCount<Fields...>()` | Returns the number of fields |
| `operator<<(os, ParseError)` | Stream a `ParseError` to `std::ostream` |
| `std::format("{}", error)` | Format `ParseError` via `std::formatter` specialization |

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

The project uses [Catch2](https://github.com/catchorg/Catch2) v3 with 100+ test cases covering:

- Field size calculations
- MinMaxRange boundary validation (exact min, exact max, below, above)
- SpecificRange validation (single value, enum values, boundary values)
- Multi-byte big-endian and little-endian parsing
- Signed integer parsing (int8_t, int16_t)
- Floating-point field parsing (float, double)
- Bit-field parsing (partial byte fields)
- Custom validator predicates (`CustomRange`)
- Cross-field validation
- Optional/conditional field parsing
- Message serialization (encode) and roundtrip tests
- CRC-8, CRC-16, CRC-32, and checksum computation
- Message framing (build and parse, with/without CRC)
- Message ID dispatch registry
- `std::span` support for all operations
- Named fields
- Reflection / field iteration
- `std::format` and `operator<<` for `ParseError`
- Enum field parsing with underlying type conversion
- `validateMessage` and `convertAll` batch operations
- `IsFieldConfiguration` / `IsBitFieldConfiguration` concept validation
- End-to-end multi-field message parsing

All parsing tests use `STATIC_REQUIRE` to verify compile-time evaluation where possible.

## Benchmarks

Catch2 benchmarks are included in `test/benchmark/` covering:

- Single-field parsing (uint8_t, uint16_t, uint32_t, both endians)
- Multi-field batch operations (`convertAll`, `validateMessage`)
- Range validation types (MinMaxRange, SpecificRange, CustomRange)
- Serialization (encode with/without validation)
- CRC computation (CRC-8, CRC-16, CRC-32 over 8 and 64 bytes)
- Frame parsing (with and without CRC)
- `std::span` vs `std::array` performance comparison
- Bit-field and float parsing

Run benchmarks:
```bash
cmake --build --preset <PRESET_NAME> --target benchmarks
./benchmarks --benchmark-samples 100
```

## Single-Header Distribution

Generate a single amalgamated header for easy integration:

```bash
cmake --build --preset <PRESET_NAME> --target single_header
```

This produces `single_include/MessageParser.h` — a single file you can drop into any project.

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
