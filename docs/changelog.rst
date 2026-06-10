Changelog
=========

v0.2.0 (Unreleased)
--------------------

Added
^^^^^

**Parsing & Encoding:**

- **Message serialization** — ``encodeField()``, ``encodeFieldChecked()``, ``encodeFloatField()``, ``encodeDoubleField()`` write typed values back into byte arrays
- **Bit-field support** — ``BitFieldConfiguration`` and ``convertBitField()`` for sub-byte field access
- **Signed integer support** — ``int8_t``, ``int16_t``, ``int32_t`` fields with two's complement parsing
- **Floating-point fields** — ``getFloatField()`` / ``getDoubleField()`` parse IEEE 754 values via ``std::bit_cast``

**Validation & Safety:**

- **Custom validator predicates** — ``CustomRange<T, Pred>`` accepts any ``constexpr`` callable
- **Cross-field validation** — ``validateCrossField()`` / ``validateCrossFields()`` for multi-field constraints
- **Optional/conditional fields** — ``convertByteTypeIf()`` parses only when a condition is met

**Protocol Features:**

- **CRC/checksum verification** — ``computeCrc8()``, ``computeCrc16()``, ``computeCrc32()``, ``computeChecksum8()``, ``verifyCrc8()``, ``verifyCrc16()``
- **Message framing** — ``FrameDefinition``, ``buildFrame()``, ``parseFrame()`` for header/length/payload/CRC/trailer protocols
- **Message ID dispatch** — ``MessageEntry``, ``MessageRegistry``, ``makeRegistry()`` for compile-time message routing

**Ergonomics & Tooling:**

- **``std::format`` integration** — ``std::formatter<ParseError>`` specialization
- **``operator<<``** — stream ``ParseError`` to ``std::ostream``
- **Compile-time factories** — ``makeField()`` and ``makeBitField()`` for concise field declarations
- **Schema helpers** — ``MessageSchema`` and ``makeSchema()`` for grouped parse/validate/convertAll operations
- **Reflection / field iteration** — ``forEachField()``, ``forEachFieldIndexed()``, ``fieldCount()``
- **Named fields** — ``NamedFieldConfiguration`` with ``string_view`` name for debugging
- **``std::span`` support** — all parsing functions accept ``std::span<const uint8_t>``

**Testing & CI:**

- **Benchmark suite** — Catch2 benchmarks for all operations (``test/benchmark/``)
- **Single-header packaging** — CMake ``single_header`` target + ``scripts/amalgamate.py``

**New error variants:**

- ``ParseError::ChecksumMismatch`` — CRC/checksum verification failed
- ``ParseError::CustomValidationFailed`` — custom predicate returned false

**New concepts:**

- ``IsBitFieldConfiguration`` — constrains bit-field configuration types
- ``IsNamedFieldConfiguration`` — constrains named field types
- ``IsCustomRange`` — detects custom range predicates

v0.1.0
------

Added
^^^^^

- **Little-endian support** — ``ByteOrder::LittleEndian`` template parameter on ``FieldConfiguration``
- **``validateMessage()``** — batch validation returning ``bool`` for all fields
- **``convertAll()``** — parse all fields at once, returning ``std::tuple`` of results
- **``to_string(ParseError)``** — human-readable error descriptions
- **``toUnderlying()``** — C++23 ``std::to_underlying`` wrapper for enums
- **``IsFieldConfiguration`` concept** — constrains generic code to valid field types
- **``ParseError::InvalidSize``** — new error variant for size mismatches
- **GCC 15 support** — added CMake presets for GCC 15
- **Clang 21 support** — added CMake presets for Clang 21
- **Enhanced compiler warnings** — added ``-Wlifetime``, ``-Wextra-semi``, ``-Wcast-qual``, ``-Wredundant-decls``, MSVC ``/w14868``, ``/w15038``, ``/w15204``, ``/w15233``
- **Sphinx documentation** — full documentation with Mermaid diagrams
- **60+ test cases** — comprehensive test coverage including byte order, concepts, batch operations
- **``consteval`` for ``getSize()`` and ``getMessageSize()``** — stronger compile-time guarantees

Changed
^^^^^^^

- Default ``MinMaxRange`` for ``FieldConfiguration``/``BitFieldConfiguration``/``NamedFieldConfiguration`` now follows field type semantics (for enums: underlying type range)

- ``getSize()`` and ``getMessageSize()`` are now ``consteval`` (previously ``constexpr``)
- ``main.cpp`` uses ``std::to_underlying`` instead of ``static_cast<uint32_t>``
- Minimum recommended compilers updated to GCC 14+, Clang 18+

v0.0.1
------

Initial release with:

- Header-only message parser
- ``FieldConfiguration`` with compile-time type mapping
- ``MinMaxRange`` and ``SpecificRange`` validation
- Big-endian multi-byte field parsing
- ``std::expected<T, ParseError>`` error handling
- Compile-time ``static_assert`` size validation via ``parseMessage()``
- Catch2 test suite with ``STATIC_REQUIRE`` compile-time tests
- CMake presets for GCC 14, Clang 18-20, MSVC 2019/2022
- Conan 2 package management
- Sanitizer and static analysis support
