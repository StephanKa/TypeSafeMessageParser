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
       "Multi-byte BE parsing" : 5
       "Multi-byte LE parsing" : 4
       "getField tests" : 4
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

5. **End-to-end tests** — Complete message parsing scenarios that exercise multiple
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
- 100% branch coverage of ``RangeChecker``
- Both byte orders tested for all multi-byte sizes
- All ``ParseError`` variants verified
- All public concepts verified with positive and negative cases
