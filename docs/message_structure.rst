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

Bit-Field Layout
----------------

Bit-fields allow sub-byte field access. A ``BitFieldConfiguration`` specifies the byte
index, bit offset (from MSB), and bit width:

.. mermaid::

   graph LR
       subgraph "Single Byte: 0xAB = 10101011"
           B7["Bit 0: 1"]
           B6["Bit 1: 0"]
           B5["Bit 2: 1"]
           B4["Bit 3: 0"]
           B3["Bit 4: 1"]
           B2["Bit 5: 0"]
           B1["Bit 6: 1"]
           B0["Bit 7: 1"]
       end

       subgraph "BitField(offset=0, width=4)"
           Result1["1010 = 10"]
       end

       subgraph "BitField(offset=4, width=4)"
           Result2["1011 = 11"]
       end

       B7 --> Result1
       B6 --> Result1
       B5 --> Result1
       B4 --> Result1
       B3 --> Result2
       B2 --> Result2
       B1 --> Result2
       B0 --> Result2

For bit-fields spanning two bytes, the library reads the necessary bytes and extracts
the bit range across the boundary.

Signed Integer Fields
---------------------

Signed integers (``int8_t``, ``int16_t``, ``int32_t``) use two's complement representation,
which is the native format. The library casts raw bytes to the signed type directly:

.. mermaid::

   graph LR
       subgraph "Raw Byte"
           Raw["0xFF"]
       end
       subgraph "As uint8_t"
           U["255"]
       end
       subgraph "As int8_t"
           S["-1 (two's complement)"]
       end
       Raw --> U
       Raw --> S

Floating-Point Fields
---------------------

IEEE 754 floats (4 bytes) and doubles (8 bytes) are parsed using ``std::bit_cast``:

.. mermaid::

   graph LR
       subgraph "4 Bytes (Big-Endian)"
           B0["0x40"]
           B1["0x48"]
           B2["0xF5"]
           B3["0xC3"]
       end
       subgraph "Assembly"
           Asm["0x4048F5C3"]
       end
       subgraph "bit_cast"
           F["float: 3.14"]
       end
       B0 --> Asm
       B1 --> Asm
       B2 --> Asm
       B3 --> Asm
       Asm --> F

Frame Structure
---------------

The ``FrameDefinition`` system wraps payloads in a protocol frame:

.. mermaid::

   graph LR
       subgraph "Frame Layout"
           H["Header (1B)"]
           L["Length (1B)"]
           P["Payload (N bytes)"]
           C["CRC-8 (0 or 1B)"]
           T["Trailer (1B)"]
       end
       H --> L --> P --> C --> T

.. mermaid::

   graph TD
       subgraph "Example: FrameDefinition<0xAA, 0x55, 32, true>"
           H["0xAA"]
           L["0x03"]
           P1["0x01"]
           P2["0x02"]
           P3["0x03"]
           CRC["CRC-8"]
           T["0x55"]
       end
       H --> L
       L --> P1
       P1 --> P2
       P2 --> P3
       P3 --> CRC
       CRC --> T

The frame overhead is calculated as: ``header(1) + length(1) + [crc(1)] + trailer(1)``.

CRC Placement
-------------

CRC/checksum fields protect the integrity of preceding data:

.. mermaid::

   graph LR
       subgraph "Message with CRC-8"
           D0["Data[0]"]
           D1["Data[1]"]
           D2["Data[2]"]
           D3["Data[3]"]
           CRC["CRC-8"]
       end
       D0 --> CRC
       D1 --> CRC
       D2 --> CRC
       D3 --> CRC

The ``verifyCrc8`` and ``verifyCrc16`` functions take template parameters specifying
which bytes are covered and where the CRC is stored.

Serialization (Encode) Flow
----------------------------

The encode path mirrors the parse path in reverse:

.. mermaid::

   flowchart TD
       Start[Typed Value] --> Enum{Is Enum?}
       Enum -->|Yes| ToUnderlying[std::to_underlying]
       Enum -->|No| Raw[Use directly]
       ToUnderlying --> Order{Byte Order?}
       Raw --> Order
       Order -->|Big-Endian| BE[Write MSB first]
       Order -->|Little-Endian| LE[Write LSB first]
       BE --> Done[Bytes written to array]
       LE --> Done
