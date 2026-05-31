Message Structure
=================

This page describes how messages are defined, structured, and parsed by the library.

Message Layout
--------------

A message is a contiguous ``std::array<uint8_t, N>`` of raw bytes. Each field in the message
is described by a ``FieldConfiguration`` that specifies:

- **Byte index** — where in the array the field starts
- **Type** — what C++ type the bytes represent
- **Range** — what values are considered valid
- **Byte order** — how multi-byte values are assembled

.. mermaid::

   graph LR
       subgraph "Raw Message Array (10 bytes)"
           B0[Byte 0]
           B1[Byte 1]
           B2[Byte 2]
           B3[Byte 3]
           B4[Byte 4]
           B5[Byte 5]
           B6[Byte 6]
           B7[Byte 7]
           B8[Byte 8]
           B9[Byte 9]
       end

       subgraph "Field Mapping"
           F1["Field A (Status enum, 1 byte)"]
           F2["Field B (Motor enum, 1 byte)"]
           F3["Field C (Fan enum, 1 byte)"]
           F4["Field D-G (uint8_t, 1 byte each)"]
           F5["Field H (Timer enum, 1 byte)"]
           F6["Field I (uint16_t, 2 bytes, BE)"]
       end

       B0 --> F1
       B1 --> F2
       B2 --> F3
       B3 --> F4
       B4 --> F4
       B5 --> F4
       B6 --> F4
       B7 --> F5
       B8 --> F6
       B9 --> F6

Defining a Message Schema
-------------------------

A message schema is a struct containing ``static constexpr`` field configurations:

.. code-block:: cpp

   enum class Status : uint8_t { OK, FAILED };
   enum class Motor : uint8_t { OK, FAILED, UNKNOWN, ERROR, SUCCESS };

   struct DataFields {
       // Simple enum field at byte 0
       static constexpr auto A = FieldConfiguration<0, Status>{};

       // Enum with range validation at byte 1
       static constexpr auto B = FieldConfiguration<1, Motor,
           FieldRanges::MinMaxRange{.min=Motor::OK, .max=Motor::SUCCESS}>{};

       // Multi-byte field (big-endian) at bytes 8-9
       static constexpr auto I = FieldConfiguration<8, uint16_t,
           FieldRanges::MinMaxRange{.min=1000, .max=65000}>{};
   };

Field Types
-----------

Single-Byte Fields
^^^^^^^^^^^^^^^^^^

Single-byte fields read one byte and cast it to the target type:

.. mermaid::

   graph LR
       Raw["Raw byte: 0x02"] --> Cast["static_cast<Motor>(0x02)"]
       Cast --> Result["Motor::UNKNOWN"]

Multi-Byte Fields (Big-Endian)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Multi-byte fields in big-endian order place the most significant byte first:

.. mermaid::

   graph LR
       subgraph "Bytes in array"
           MSB["Byte[n]: 0xAA (MSB)"]
           LSB["Byte[n+1]: 0xFF (LSB)"]
       end
       subgraph "Assembly"
           Shift["0xAA << 8 | 0xFF"]
       end
       subgraph "Result"
           Val["uint16_t: 0xAAFF = 43775"]
       end
       MSB --> Shift
       LSB --> Shift
       Shift --> Val

Multi-Byte Fields (Little-Endian)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Multi-byte fields in little-endian order place the least significant byte first:

.. mermaid::

   graph LR
       subgraph "Bytes in array"
           LSB["Byte[n]: 0x34 (LSB)"]
           MSB["Byte[n+1]: 0x12 (MSB)"]
       end
       subgraph "Assembly"
           Shift["0x34 | (0x12 << 8)"]
       end
       subgraph "Result"
           Val["uint16_t: 0x1234 = 4660"]
       end
       LSB --> Shift
       MSB --> Shift
       Shift --> Val

Range Validation
----------------

Every field can optionally specify a range constraint. The range is checked after byte
assembly and type casting.

MinMaxRange
^^^^^^^^^^^

Validates that ``min ≤ value ≤ max``:

.. mermaid::

   graph TD
       V[Parsed Value] --> CheckMin{"value < min?"}
       CheckMin -->|Yes| BelowRange["Error: BelowRange"]
       CheckMin -->|No| CheckMax{"value > max?"}
       CheckMax -->|Yes| AboveRange["Error: AboveRange"]
       CheckMax -->|No| Success["OK: return value"]

SpecificRange
^^^^^^^^^^^^^

Validates that the value exists in an explicit set of allowed values:

.. mermaid::

   graph TD
       V[Parsed Value] --> Search{"value ∈ {v1, v2, ...}?"}
       Search -->|Yes| Success["OK: return value"]
       Search -->|No| NotExist["Error: ValueNotExist"]

Compile-Time Size Validation
----------------------------

The ``parseMessage`` function enforces at compile time that the sum of all declared field
sizes exactly matches the message array size:

.. mermaid::

   graph TD
       Fields["Fields: A(1) + B(1) + C(2) + D(4) = 8"] --> Compare{"Sum == array.size()?"}
       Array["Array: std::array<uint8_t, 8>"] --> Compare
       Compare -->|Yes| OK["Compilation succeeds"]
       Compare -->|No| Error["static_assert failure!"]

This prevents:

- **Buffer overread** — reading past the end of the array
- **Buffer underread** — missing bytes that should be parsed
- **Misaligned fields** — field sizes not matching the protocol specification

Full Message Parsing Flow
-------------------------

.. mermaid::

   flowchart TD
       Start[Receive raw bytes] --> Define[Define FieldConfiguration schema]
       Define --> Validate[parseMessage: compile-time size check]
       Validate --> Parse[convertByteType for each field]
       Parse --> Single{Single byte?}
       Single -->|Yes| Cast[Direct cast to type]
       Single -->|No| Order{Byte order?}
       Order -->|Big-Endian| BE[MSB-first assembly]
       Order -->|Little-Endian| LE[LSB-first assembly]
       Cast --> Range[RangeChecker]
       BE --> Range
       LE --> Range
       Range --> Valid{Range valid?}
       Valid -->|Yes| Success["std::expected(value)"]
       Valid -->|No| Error["std::unexpected(ParseError)"]
