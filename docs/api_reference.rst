API Reference
=============

This page provides a complete reference for all types, functions, and concepts
in the TypeSafeMessageParser library.

Namespace: FieldRanges
----------------------

ParseError
^^^^^^^^^^

.. code-block:: cpp

   enum class ParseError : std::uint8_t {
       ValueNotExist,  // Value not found in SpecificRange set
       BelowRange,     // Value is below MinMaxRange minimum
       AboveRange,     // Value is above MinMaxRange maximum
       InvalidSize     // Message size does not match field layout
   };

to_string
^^^^^^^^^

.. code-block:: cpp

   constexpr std::string_view to_string(ParseError error);

Returns a human-readable string representation of a ``ParseError`` value.

**Example:**

.. code-block:: cpp

   auto str = FieldRanges::to_string(FieldRanges::ParseError::BelowRange);
   // str == "BelowRange"

MinMaxRange<T>
^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename T>
   struct MinMaxRange {
       T min{};
       T max{};
   };

Defines an inclusive range ``[min, max]``. Works with integral types and enums.

**Example:**

.. code-block:: cpp

   // Integer range
   constexpr auto range = FieldRanges::MinMaxRange<uint16_t>{.min = 1000, .max = 65000};

   // Enum range
   enum class Status : uint8_t { OK, WARN, ERR };
   constexpr auto range = FieldRanges::MinMaxRange{.min = Status::OK, .max = Status::ERR};

SpecificRange<Entries...>
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename... Entries>
   struct SpecificRange {
       constexpr explicit SpecificRange(Entries&&... entries);
       using type = std::common_type_t<Entries...>;
       std::array<type, sizeof...(Entries)> possibleValues;
   };

Defines an explicit set of allowed values. The parsed value must be one of the
entries to pass validation.

**Example:**

.. code-block:: cpp

   // Only allow specific uint8_t values
   constexpr auto range = FieldRanges::SpecificRange{uint8_t{10}, uint8_t{20}, uint8_t{30}};

   // Only allow specific enum values
   enum class Mode : uint8_t { Idle, Run, Sleep };
   constexpr auto range = FieldRanges::SpecificRange{Mode::Idle, Mode::Run};

Concepts
^^^^^^^^

.. code-block:: cpp

   template<typename Type>
   concept IsSpecificRange = requires(Type val) { val.possibleValues; };

   template<typename Type>
   concept IsMinMaxRange = requires(Type val) { val.min; val.max; };

ByteOrder Enum
--------------

.. code-block:: cpp

   enum class ByteOrder : std::uint8_t {
       BigEndian,      // Most significant byte first (default)
       LittleEndian    // Least significant byte first
   };

FieldConfiguration
------------------

.. code-block:: cpp

   template<
       std::size_t index,
       typename FieldType,
       auto Range = FieldRanges::MinMaxRange{
           .min = std::numeric_limits<uint32_t>::min(),
           .max = std::numeric_limits<uint32_t>::max()
       },
       ByteOrder Order = ByteOrder::BigEndian
   >
   struct FieldConfiguration {
       static constexpr std::size_t byteIndex{index};
       static constexpr std::size_t byteLength{sizeof(FieldType)};
       static constexpr FieldType type{};
       static constexpr decltype(Range) range{Range};
       static constexpr ByteOrder byteOrder{Order};
   };

**Template Parameters:**

- ``index`` — Starting byte position in the message array
- ``FieldType`` — The C++ type this field represents (``uint8_t``, ``uint16_t``, ``uint32_t``, or an enum)
- ``Range`` — A ``MinMaxRange`` or ``SpecificRange`` for validation (default: full uint32_t range)
- ``Order`` — Byte order for multi-byte fields (default: ``BigEndian``)

Namespace: MessageParser
------------------------

getField<Field>(msg)
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename Field>
   constexpr decltype(Field::type) getField(const auto& msg);

Reads a single byte from the message at the field's byte index and casts it to the
field's type. **No range validation is performed.**

**Parameters:**

- ``msg`` — A ``std::array<uint8_t, N>`` message

**Returns:** The field value cast to ``Field::type``

getSize<Fields...>()
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename... Fields>
   consteval auto getSize();

Returns the sum of byte lengths of all given fields. Evaluated at compile time (``consteval``).

getMessageSize(msg)
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename T, std::size_t size>
   consteval auto getMessageSize(const std::array<T, size>& value);

Returns the size of the given message array. Evaluated at compile time (``consteval``).

parseMessage(msg, fields...)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename T, std::size_t size, typename... Fields>
   constexpr void parseMessage(const std::array<T, size>& msg, const Fields&... fields);

Validates that the sum of all field sizes equals the message array size via
``static_assert`` (in ``consteval`` context). Then reads all fields.

**Compile-time error if sizes don't match.**

convertByteType(msg, field)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename Field>
   constexpr auto convertByteType(const auto& msg, const Field& field);

The primary parsing function. Reads bytes from the message, assembles multi-byte
values according to the field's byte order, and validates through ``RangeChecker``.

**Returns:** ``std::expected<FieldType, FieldRanges::ParseError>``

convertAll(msg, fields...)
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename T, std::size_t size, typename... Fields>
   constexpr auto convertAll(const std::array<T, size>& msg, const Fields&... fields);

Parses all specified fields from the message in one call.

**Returns:** ``std::tuple<std::expected<T1, ParseError>, std::expected<T2, ParseError>, ...>``

**Example:**

.. code-block:: cpp

   auto [status, counter] = MessageParser::convertAll(msg, Fields::status, Fields::counter);

validateMessage(msg, fields...)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename T, std::size_t size, typename... Fields>
   constexpr bool validateMessage(const std::array<T, size>& msg, const Fields&... fields);

Checks whether all fields in the message have valid values (pass range validation).

**Returns:** ``true`` if all fields are valid, ``false`` if any field fails.

toUnderlying(value)
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   constexpr auto toUnderlying(const auto& value);

For enum types, returns the underlying integral value using ``std::to_underlying``.
For non-enum types, returns the value unchanged.

RangeChecker<Field>(value)
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename Field>
   constexpr std::expected<decltype(Field::type), FieldRanges::ParseError> RangeChecker(const auto& value);

Internal validation function. Checks the value against the field's range constraint.

IsFieldConfiguration Concept
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename T>
   concept IsFieldConfiguration = requires {
       { T::byteIndex } -> std::convertible_to<std::size_t>;
       { T::byteLength } -> std::convertible_to<std::size_t>;
       T::type;
       T::range;
   };

Constrains template parameters to valid ``FieldConfiguration`` types. Use in your own
generic code to ensure type safety.
