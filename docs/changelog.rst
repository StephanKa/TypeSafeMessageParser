Changelog
=========

v0.1.0 (Unreleased)
--------------------

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
