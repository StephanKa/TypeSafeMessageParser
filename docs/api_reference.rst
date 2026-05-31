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
       ValueNotExist,            // Value not found in SpecificRange set
       BelowRange,              // Value is below MinMaxRange minimum
       AboveRange,              // Value is above MinMaxRange maximum
       InvalidSize,             // Message size does not match field layout
       ChecksumMismatch,        // CRC/checksum verification failed
       CustomValidationFailed   // Custom predicate returned false
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

Defines an inclusive range ``[min, max]``. Works with integral types (including signed), and enums.

**Example:**

.. code-block:: cpp

   // Integer range
   constexpr auto range = FieldRanges::MinMaxRange<uint16_t>{.min = 1000, .max = 65000};

   // Signed integer range
   constexpr auto signedRange = FieldRanges::MinMaxRange<int8_t>{.min = -50, .max = 50};

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

CustomRange<T, Pred>
^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename T, typename Pred>
   struct CustomRange {
       Pred predicate;
   };

Defines a custom validation predicate. The predicate must be a ``constexpr``-callable
that takes a value and returns ``bool``. Returns ``CustomValidationFailed`` on failure.

**Example:**

.. code-block:: cpp

   // Only allow even numbers
   constexpr auto isEven = [](auto val) constexpr { return val % 2 == 0; };
   static constexpr auto field = FieldConfiguration<0, uint8_t,
       FieldRanges::CustomRange<uint8_t, decltype(isEven)>{isEven}>{};

   // Only allow multiples of 4
   constexpr auto isAligned = [](auto val) constexpr { return val % 4 == 0; };
   static constexpr auto aligned = FieldConfiguration<0, uint16_t,
       FieldRanges::CustomRange<uint16_t, decltype(isAligned)>{isAligned}>{};

Concepts
^^^^^^^^

.. code-block:: cpp

   template<typename Type>
   concept IsSpecificRange = requires(Type val) { val.possibleValues; };

   template<typename Type>
   concept IsMinMaxRange = requires(Type val) { val.min; val.max; };

   template<typename Type>
   concept IsCustomRange = requires(Type val) { val.predicate; };

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
- ``FieldType`` — The C++ type this field represents (``uint8_t``, ``int8_t``, ``uint16_t``, ``int16_t``, ``uint32_t``, ``int32_t``, ``float``, ``double``, or an enum)
- ``Range`` — A ``MinMaxRange``, ``SpecificRange``, or ``CustomRange`` for validation (default: full uint32_t range)
- ``Order`` — Byte order for multi-byte fields (default: ``BigEndian``)

BitFieldConfiguration
---------------------

.. code-block:: cpp

   template<
       std::size_t byteIdx,
       std::size_t bitOffset,
       std::size_t bitWidth,
       typename FieldType,
       auto Range = FieldRanges::MinMaxRange{...}
   >
   struct BitFieldConfiguration {
       static constexpr std::size_t byteIndex{byteIdx};
       static constexpr std::size_t bitOff{bitOffset};
       static constexpr std::size_t bitLen{bitWidth};
       static constexpr std::size_t byteLength{(bitOffset + bitWidth + 7) / 8};
       static constexpr FieldType type{};
       static constexpr decltype(Range) range{Range};
       static constexpr ByteOrder byteOrder{ByteOrder::BigEndian};
   };

Declares a bit-level field within one or more bytes. Used for protocols where fields
don't align to byte boundaries (common in CAN bus).

**Template Parameters:**

- ``byteIdx`` — Starting byte position
- ``bitOffset`` — Bit offset within the starting byte (0 = MSB)
- ``bitWidth`` — Number of bits the field occupies
- ``FieldType`` — The C++ type to cast the extracted bits to
- ``Range`` — Optional range constraint

**Example:**

.. code-block:: cpp

   // 3-bit field starting at bit offset 2 in byte 0
   static constexpr auto flags = BitFieldConfiguration<0, 2, 3, uint8_t>{};

   // 4-bit field at bit offset 0 with range validation
   static constexpr auto priority = BitFieldConfiguration<0, 0, 4, uint8_t,
       FieldRanges::MinMaxRange{.min = uint8_t{0}, .max = uint8_t{7}}>{};

NamedFieldConfiguration
-----------------------

.. code-block:: cpp

   template<std::size_t index, typename FieldType, auto Range = ..., ByteOrder Order = ...>
   struct NamedFieldConfiguration {
       static constexpr std::size_t byteIndex{index};
       static constexpr std::size_t byteLength{sizeof(FieldType)};
       static constexpr FieldType type{};
       static constexpr decltype(Range) range{Range};
       static constexpr ByteOrder byteOrder{Order};
       std::string_view name{};
   };

A ``FieldConfiguration`` with an attached compile-time name for debug/logging purposes.
Works with all the same functions as ``FieldConfiguration``.

**Example:**

.. code-block:: cpp

   static constexpr auto temp = NamedFieldConfiguration<0, uint16_t,
       FieldRanges::MinMaxRange{.min = uint16_t{0}, .max = uint16_t{1000}}>{.name = "temperature"};

Namespace: MessageParser
------------------------

Core Parsing
^^^^^^^^^^^^

getField<Field>(msg)
""""""""""""""""""""

.. code-block:: cpp

   template<typename Field>
   constexpr decltype(Field::type) getField(const auto& msg);

Reads a single byte from the message at the field's byte index and casts it to the
field's type. **No range validation is performed.**

getSize<Fields...>()
""""""""""""""""""""

.. code-block:: cpp

   template<typename... Fields>
   consteval auto getSize();

Returns the sum of byte lengths of all given fields. Evaluated at compile time (``consteval``).

getMessageSize(msg)
"""""""""""""""""""

.. code-block:: cpp

   template<typename T, std::size_t size>
   consteval auto getMessageSize(const std::array<T, size>& value);

Returns the size of the given message array. Evaluated at compile time (``consteval``).

parseMessage(msg, fields...)
""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename T, std::size_t size, typename... Fields>
   constexpr void parseMessage(const std::array<T, size>& msg, const Fields&... fields);

Validates that the sum of all field sizes equals the message array size via
``static_assert`` (in ``consteval`` context). Then reads all fields.

**Compile-time error if sizes don't match.**

convertByteType(msg, field)
"""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename Field>
   constexpr auto convertByteType(const auto& msg, const Field& field);

   // std::span overload
   template<typename Field>
   constexpr auto convertByteType(std::span<const std::uint8_t> msg, const Field& field);

The primary parsing function. Reads bytes from the message, assembles multi-byte
values according to the field's byte order, and validates through ``RangeChecker``.
Accepts both ``std::array`` and ``std::span<const uint8_t>``.

**Returns:** ``std::expected<FieldType, FieldRanges::ParseError>``

convertBitField(msg, field)
"""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename BitField>
       requires IsBitFieldConfiguration<BitField>
   constexpr auto convertBitField(const auto& msg, const BitField& field);

Parses a bit-field from the message. Extracts bits at the specified offset and width,
then validates through ``RangeChecker``.

**Returns:** ``std::expected<FieldType, FieldRanges::ParseError>``

**Example:**

.. code-block:: cpp

   static constexpr auto bf = BitFieldConfiguration<0, 4, 4, uint8_t>{};
   constexpr std::array<uint8_t, 1> msg = {0xAB}; // lower 4 bits = 0xB = 11
   auto result = MessageParser::convertBitField(msg, bf); // *result == 11

Floating-Point Parsing
^^^^^^^^^^^^^^^^^^^^^^

getFloatField<index, Order>(msg)
""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian>
   constexpr auto getFloatField(const auto& msg);

Parses an IEEE 754 ``float`` (4 bytes) from the message using ``std::bit_cast``.

getDoubleField<index, Order>(msg)
"""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian>
   constexpr auto getDoubleField(const auto& msg);

Parses an IEEE 754 ``double`` (8 bytes) from the message using ``std::bit_cast``.

Batch Operations
^^^^^^^^^^^^^^^^

convertAll(msg, fields...)
""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename T, std::size_t size, typename... Fields>
   constexpr auto convertAll(const std::array<T, size>& msg, const Fields&... fields);

   // std::span overload
   template<typename... Fields>
   constexpr auto convertAll(std::span<const std::uint8_t> msg, const Fields&... fields);

Parses all specified fields from the message in one call.

**Returns:** ``std::tuple<std::expected<T1, ParseError>, std::expected<T2, ParseError>, ...>``

validateMessage(msg, fields...)
"""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename T, std::size_t size, typename... Fields>
   constexpr bool validateMessage(const std::array<T, size>& msg, const Fields&... fields);

   // std::span overload
   template<typename... Fields>
   constexpr bool validateMessage(std::span<const std::uint8_t> msg, const Fields&... fields);

Checks whether all fields in the message have valid values (pass range validation).

Conditional Parsing
^^^^^^^^^^^^^^^^^^^

convertByteTypeIf(msg, field, condition)
""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename Field>
   constexpr auto convertByteTypeIf(const auto& msg, const Field& field, bool condition)
       -> std::expected<std::optional<decltype(Field::type)>, FieldRanges::ParseError>;

Parse a field only if a condition is true. Returns ``std::nullopt`` (wrapped in
``expected``) if the condition is false, the parsed value if true and valid,
or an error if true and validation fails.

**Example:**

.. code-block:: cpp

   // Only parse optional payload if header indicates it's present
   auto header = MessageParser::convertByteType(msg, Fields::header);
   bool hasPayload = header.has_value() && *header > 0;
   auto payload = MessageParser::convertByteTypeIf(msg, Fields::payload, hasPayload);

Cross-field Validation
^^^^^^^^^^^^^^^^^^^^^^

validateCrossField(msg, predicate)
""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename T, std::size_t size, typename Predicate>
   constexpr bool validateCrossField(const std::array<T, size>& msg, Predicate&& pred);

Validates a message using a predicate that receives the full message array.
Use for constraints that depend on multiple fields.

**Example:**

.. code-block:: cpp

   auto valid = MessageParser::validateCrossField(msg, [](const auto& m) {
       auto status = MessageParser::convertByteType(m, Fields::status);
       auto rpm = MessageParser::convertByteType(m, Fields::rpm);
       if (status.has_value() && *status == Status::Error)
           return rpm.has_value() && *rpm == 0;
       return true;
   });

validateCrossFields(msg, preds...)
""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename T, std::size_t size, typename... Predicates>
   constexpr bool validateCrossFields(const std::array<T, size>& msg, Predicates&&... preds);

Multiple cross-field predicates — all must pass for the function to return ``true``.

Serialization (Encode)
^^^^^^^^^^^^^^^^^^^^^^

encodeField(msg, field, value)
""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename Field, typename T, std::size_t size>
   constexpr void encodeField(std::array<T, size>& msg, const Field& field,
                              const decltype(Field::type)& value);

Writes a typed value into the byte array at the field's position, respecting byte order.
Supports enums (automatically uses ``std::to_underlying``).

**Example:**

.. code-block:: cpp

   std::array<uint8_t, 4> msg{};
   MessageParser::encodeField(msg, Fields::rpm, uint16_t{5000});

encodeFieldChecked(msg, field, value)
"""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename Field, typename T, std::size_t size>
   constexpr std::expected<void, FieldRanges::ParseError> encodeFieldChecked(
       std::array<T, size>& msg, const Field& field, const decltype(Field::type)& value);

Same as ``encodeField`` but validates the value against the field's range first.
Returns an error if validation fails; does not write to the array on failure.

encodeFloatField<index, Order>(msg, value)
""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian, typename T, std::size_t size>
   constexpr void encodeFloatField(std::array<T, size>& msg, float value);

Encodes an IEEE 754 ``float`` into 4 bytes at the given index.

encodeDoubleField<index, Order>(msg, value)
"""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian, typename T, std::size_t size>
   constexpr void encodeDoubleField(std::array<T, size>& msg, double value);

Encodes an IEEE 754 ``double`` into 8 bytes at the given index.

CRC / Checksum
^^^^^^^^^^^^^^

computeCrc8(data)
"""""""""""""""""

.. code-block:: cpp

   constexpr std::uint8_t computeCrc8(std::span<const std::uint8_t> data);

Computes CRC-8 with polynomial 0x07, initial value 0x00.

computeCrc16(data)
""""""""""""""""""

.. code-block:: cpp

   constexpr std::uint16_t computeCrc16(std::span<const std::uint8_t> data);

Computes CRC-16 CCITT with polynomial 0x1021, initial value 0xFFFF.

computeCrc32(data)
""""""""""""""""""

.. code-block:: cpp

   constexpr std::uint32_t computeCrc32(std::span<const std::uint8_t> data);

Computes CRC-32 with reflected polynomial 0xEDB88320.

computeChecksum8(data)
""""""""""""""""""""""

.. code-block:: cpp

   constexpr std::uint8_t computeChecksum8(std::span<const std::uint8_t> data);

Computes a simple 8-bit sum checksum (sum of all bytes, truncated to 8 bits).

verifyCrc8<start, len, crcIdx>(msg)
""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<std::size_t payloadStart, std::size_t payloadLen, std::size_t crcIndex,
            typename T, std::size_t size>
   constexpr std::expected<void, FieldRanges::ParseError> verifyCrc8(const std::array<T, size>& msg);

Verifies CRC-8 of bytes ``[payloadStart, payloadStart+payloadLen)`` against the
byte at ``crcIndex``. Returns ``ChecksumMismatch`` on failure.

verifyCrc16<start, len, crcIdx, Order>(msg)
"""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<std::size_t payloadStart, std::size_t payloadLen, std::size_t crcIndex,
            ByteOrder CrcOrder = ByteOrder::BigEndian, typename T, std::size_t size>
   constexpr std::expected<void, FieldRanges::ParseError> verifyCrc16(const std::array<T, size>& msg);

Verifies CRC-16 of bytes ``[payloadStart, payloadStart+payloadLen)`` against the
two bytes at ``crcIndex``. Byte order of the stored CRC is configurable.

Message Framing
^^^^^^^^^^^^^^^

FrameDefinition
"""""""""""""""

.. code-block:: cpp

   template<std::uint8_t HeaderByte, std::uint8_t TrailerByte,
            std::size_t MaxPayloadSize, bool HasCrc8 = false>
   struct FrameDefinition {
       static constexpr std::uint8_t header{HeaderByte};
       static constexpr std::uint8_t trailer{TrailerByte};
       static constexpr std::size_t maxPayload{MaxPayloadSize};
       static constexpr bool hasCrc{HasCrc8};
       static constexpr std::size_t overhead{2 + (hasCrc ? 1 : 0) + 1};
       static constexpr std::size_t maxFrameSize{overhead + maxPayload};
   };

Defines a frame format: ``[header(1)] [length(1)] [payload(N)] [crc8(0|1)] [trailer(1)]``

buildFrame<Frame>(payload)
""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename Frame, std::size_t PayloadSize>
   constexpr auto buildFrame(const std::array<std::uint8_t, PayloadSize>& payload);

Constructs a framed message. If the frame has CRC enabled, it is computed and inserted
automatically.

**Returns:** ``std::array<uint8_t, Frame::overhead + PayloadSize>``

parseFrame<Frame>(data)
"""""""""""""""""""""""

.. code-block:: cpp

   template<typename Frame>
   constexpr std::expected<std::span<const std::uint8_t>, FieldRanges::ParseError>
   parseFrame(std::span<const std::uint8_t> data);

Validates the frame structure (header, length, trailer, optional CRC) and returns
a span over the payload bytes.

**Errors:**

- ``InvalidSize`` — frame too short or length field exceeds available data
- ``ValueNotExist`` — header or trailer byte mismatch
- ``ChecksumMismatch`` — CRC verification failed (if enabled)

Message ID Dispatch
^^^^^^^^^^^^^^^^^^^

MessageEntry<IdType, Id, ParseFunc>
"""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename IdType, IdType Id, typename ParseFunc>
   struct MessageEntry {
       static constexpr IdType id{Id};
       ParseFunc parser;
   };

Maps a message ID to a parse handler function.

MessageRegistry<IdType, Entries...>
"""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename IdType, typename... Entries>
   struct MessageRegistry {
       constexpr MessageRegistry(Entries... e);
       template<typename Payload>
       constexpr bool dispatch(IdType id, const Payload& payload) const;
   };

Compile-time registry that dispatches to the correct handler based on message ID.
Returns ``true`` if a matching handler was found, ``false`` otherwise.

makeRegistry<IdType>(entries...)
""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename IdType, typename... Entries>
   constexpr auto makeRegistry(Entries... entries);

Helper to construct a ``MessageRegistry``.

**Example:**

.. code-block:: cpp

   auto registry = MessageParser::makeRegistry<uint8_t>(
       MessageParser::MessageEntry<uint8_t, 0x01, decltype(handler1)>{.parser = handler1},
       MessageParser::MessageEntry<uint8_t, 0x02, decltype(handler2)>{.parser = handler2}
   );
   registry.dispatch(msgId, payload);

Reflection / Field Iteration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

forEachField(func, fields...)
"""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename Func, typename... Fields>
   constexpr void forEachField(Func&& func, const Fields&... fields);

Invokes ``func`` for each field in the parameter pack.

forEachFieldIndexed(func, fields...)
""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename Func, typename... Fields>
   constexpr void forEachFieldIndexed(Func&& func, const Fields&... fields);

Invokes ``func(index, field)`` for each field, where ``index`` is a ``std::size_t``
starting from 0.

fieldCount<Fields...>()
"""""""""""""""""""""""

.. code-block:: cpp

   template<typename... Fields>
   consteval std::size_t fieldCount();

Returns the number of fields at compile time.

Utility Functions
^^^^^^^^^^^^^^^^^

toUnderlying(value)
"""""""""""""""""""

.. code-block:: cpp

   constexpr auto toUnderlying(const auto& value);

For enum types, returns the underlying integral value using ``std::to_underlying``.
For non-enum types, returns the value unchanged.

operator<< for ParseError
"""""""""""""""""""""""""

.. code-block:: cpp

   std::ostream& operator<<(std::ostream& os, FieldRanges::ParseError error);

Streams the human-readable error string to an ``std::ostream``.

std::formatter<ParseError>
""""""""""""""""""""""""""

.. code-block:: cpp

   template<>
   struct std::formatter<FieldRanges::ParseError> : std::formatter<std::string_view> { ... };

Enables use with ``std::format``:

.. code-block:: cpp

   auto msg = std::format("Parse failed: {}", result.error());

Concepts
^^^^^^^^

.. code-block:: cpp

   template<typename T>
   concept IsFieldConfiguration = requires {
       { T::byteIndex } -> std::convertible_to<std::size_t>;
       { T::byteLength } -> std::convertible_to<std::size_t>;
       T::type;
       T::range;
   };

   template<typename T>
   concept IsBitFieldConfiguration = requires {
       { T::byteIndex } -> std::convertible_to<std::size_t>;
       { T::bitOff } -> std::convertible_to<std::size_t>;
       { T::bitLen } -> std::convertible_to<std::size_t>;
       T::type;
       T::range;
   };

   template<typename T>
   concept IsNamedFieldConfiguration = IsFieldConfiguration<T> && requires(T t) {
       { t.name } -> std::convertible_to<std::string_view>;
   };

Constrains template parameters to valid configuration types. Use in your own
generic code to ensure type safety.
