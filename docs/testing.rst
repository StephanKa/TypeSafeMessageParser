Testing
=======

The project uses `Catch2 <https://github.com/catchorg/Catch2>`_ v3 for unit testing,
with a strong emphasis on compile-time verification via ``STATIC_REQUIRE``.

Test Categories
---------------

The test suite is organized into the following categories:

.. mermaid::

   pie title Test Coverage Distribution
       "getSize calculations" : 4
       "MinMaxRange validation" : 7
       "SpecificRange validation" : 6
       "CustomRange validation" : 2
       "Multi-byte BE parsing" : 5
       "Multi-byte LE parsing" : 4
       "Signed integer parsing" : 4
       "Float/double parsing" : 4
       "Bit-field parsing" : 4
       "getField tests" : 4
       "Encode/serialize" : 10
       "CRC/Checksum" : 6
       "Message framing" : 4
       "Message dispatch" : 2
       "Cross-field validation" : 3
       "Conditional fields" : 3
       "std::span support" : 4
       "Named fields" : 2
       "Reflection/iteration" : 3
       "std::format / operator<<" : 4
       "validateMessage" : 2
       "convertAll" : 2
       "Utility functions" : 4
       "End-to-end" : 1

Running Tests
-------------

.. code-block:: bash

   # Configure
   cmake --preset unixlike-clang-20-debug

   # Build
   cmake --build --preset build-unixlike-clang-20-debug

   # Run all tests
   ctest --preset test-unixlike-clang-20-debug

   # Run with verbose output
   ctest --preset test-unixlike-clang-20-debug --output-on-failure

Running Benchmarks
------------------

The benchmark suite uses Catch2's built-in benchmarking support:

.. code-block:: bash

   # Build benchmarks
   cmake --build --preset build-unixlike-clang-20-release --target benchmarks

   # Run with 100 samples
   ./benchmarks --benchmark-samples 100

   # Run specific benchmark
   ./benchmarks "[benchmark]" --benchmark-samples 100

Benchmark categories:

- Single-field parsing (uint8_t, uint16_t, uint32_t, both endians)
- Multi-field batch operations (``convertAll``, ``validateMessage``)
- Range validation types (MinMaxRange, SpecificRange, CustomRange)
- Serialization (encode with/without validation)
- CRC computation (CRC-8, CRC-16, CRC-32 over various sizes)
- Frame parsing (with and without CRC)
- ``std::span`` vs ``std::array`` performance comparison
- Bit-field and float parsing

Test Design Principles
----------------------

1. **Compile-time where possible** — ``STATIC_REQUIRE`` is preferred over ``REQUIRE``
   to ensure parsing works at compile time.

2. **Boundary testing** — Every range is tested at:
   - Exact minimum (should pass)
   - Exact maximum (should pass)
   - One below minimum (should fail with ``BelowRange``)
   - One above maximum (should fail with ``AboveRange``)

3. **Error variant verification** — Tests check not just that errors occur, but that
   the *correct* ``ParseError`` variant is returned.

4. **Byte order coverage** — Both big-endian and little-endian are tested, including
   verification that the same bytes produce different values under different orders.

5. **Roundtrip testing** — Encode/decode roundtrips verify that serialization is the
   exact inverse of parsing.

6. **End-to-end tests** — Complete message parsing scenarios that exercise multiple
   features together.

Adding New Tests
----------------

Follow this pattern for new test cases:

.. code-block:: cpp

   TEST_CASE("Descriptive name of what is being tested")
   {
       // 1. Define the field configuration
       static constexpr auto field = FieldConfiguration<0, uint8_t,
           FieldRanges::MinMaxRange{.min = uint8_t{10}, .max = uint8_t{200}}>{};

       // 2. Create test message
       constexpr std::array<uint8_t, 1> msg = { 0x42 };

       // 3. Parse and verify (prefer STATIC_REQUIRE for constexpr)
       constexpr auto result = MessageParser::convertByteType(msg, field);
       STATIC_REQUIRE(result.has_value());
       STATIC_REQUIRE(*result == 0x42);
   }

Coverage Targets
----------------

The test suite aims for:

- 100% function coverage of the ``MessageParser`` namespace
- 100% branch coverage of ``RangeChecker`` (MinMaxRange, SpecificRange, CustomRange)
- Both byte orders tested for all multi-byte sizes
- All ``ParseError`` variants verified (including ``ChecksumMismatch`` and ``CustomValidationFailed``)
- All public concepts verified with positive and negative cases
- Encode/decode roundtrip for every field type
- CRC correctness against known test vectors
- Frame build/parse roundtrip with and without CRC
