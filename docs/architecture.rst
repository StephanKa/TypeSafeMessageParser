Architecture
============

This page describes the internal architecture of TypeSafeMessageParser and how messages
are structured, parsed, validated, and serialized.

Design Philosophy
-----------------

The library follows these core principles:

1. **Zero runtime cost** — all validation that can happen at compile time does
2. **Type safety** — raw bytes are never exposed without explicit casting
3. **Composability** — fields are independent units that compose into messages
4. **Explicit error handling** — ``std::expected`` forces callers to handle failures
5. **Bidirectional** — parse and serialize with the same field definitions
6. **Protocol-aware** — built-in support for framing, CRC, and message dispatch

High-Level Architecture
-----------------------

.. mermaid::

   graph TB
       subgraph "User Code"
           A[Define Enums/Types]
           B[Declare FieldConfiguration]
           C[Create Message Array]
           D[Define Frame/Registry]
       end

       subgraph "MessageParser Library"
           subgraph "Parsing"
               E[convertByteType - Parse & Validate]
               F[convertBitField - Bit-level Parse]
               G[getFloatField / getDoubleField]
               H[RangeChecker - Range Validation]
           end
           subgraph "Serialization"
               I[encodeField - Write to Array]
               J[encodeFieldChecked - Validate & Write]
               K[encodeFloatField / encodeDoubleField]
           end
           subgraph "Protocol"
               L[parseFrame - Deframe & Verify]
               M[buildFrame - Frame & CRC]
               N[MessageRegistry - ID Dispatch]
               O[CRC/Checksum - Integrity]
           end
           subgraph "Validation"
               P[validateMessage - Batch Check]
               Q[validateCrossField - Multi-field]
               R[convertByteTypeIf - Conditional]
           end
           subgraph "Utilities"
               S[forEachField - Reflection]
               T[std::format / operator<<]
           end
       end

       subgraph "Results"
           U["std::expected&lt;T, ParseError&gt;"]
           V[Compile-time static_assert]
           W["std::span&lt;payload&gt;"]
       end

       A --> B
       B --> E
       C --> E
       E --> H
       H --> U
       B --> I
       D --> L
       D --> N
       L --> W
       L --> O

Component Overview
------------------

.. mermaid::

   classDiagram
       class FieldConfiguration {
           +size_t byteIndex
           +size_t byteLength
           +FieldType type
           +Range range
           +ByteOrder byteOrder
       }

       class BitFieldConfiguration {
           +size_t byteIndex
           +size_t bitOff
           +size_t bitLen
           +size_t byteLength
           +FieldType type
           +Range range
       }

       class NamedFieldConfiguration {
           +size_t byteIndex
           +size_t byteLength
           +FieldType type
           +Range range
           +ByteOrder byteOrder
           +string_view name
       }

       class MinMaxRange~T~ {
           +T min
           +T max
       }

       class SpecificRange~Entries~ {
           +array possibleValues
       }

       class CustomRange~T_Pred~ {
           +Pred predicate
       }

       class ParseError {
           <<enumeration>>
           ValueNotExist
           BelowRange
           AboveRange
           InvalidSize
           ChecksumMismatch
           CustomValidationFailed
       }

       class ByteOrder {
           <<enumeration>>
           BigEndian
           LittleEndian
       }

       class FrameDefinition {
           +uint8_t header
           +uint8_t trailer
           +size_t maxPayload
           +bool hasCrc
           +size_t overhead
       }

       class MessageRegistry~IdType~ {
           +tuple entries
           +dispatch(id, payload) bool
       }

       class MessageEntry~IdType_Id_Func~ {
           +IdType id
           +ParseFunc parser
       }

       class MessageParser {
           +getField~Field~(msg)
           +getSize~Fields~()
           +parseMessage(msg, fields...)
           +convertByteType(msg, field)
           +convertBitField(msg, field)
           +getFloatField(msg)
           +getDoubleField(msg)
           +convertAll(msg, fields...)
           +validateMessage(msg, fields...)
           +convertByteTypeIf(msg, field, cond)
           +validateCrossField(msg, pred)
           +encodeField(msg, field, value)
           +encodeFieldChecked(msg, field, value)
           +encodeFloatField(msg, value)
           +computeCrc8(data)
           +computeCrc16(data)
           +computeCrc32(data)
           +verifyCrc8(msg)
           +verifyCrc16(msg)
           +parseFrame(data)
           +buildFrame(payload)
           +forEachField(func, fields...)
           +forEachFieldIndexed(func, fields...)
           +fieldCount()
       }

       FieldConfiguration --> MinMaxRange : uses
       FieldConfiguration --> SpecificRange : uses
       FieldConfiguration --> CustomRange : uses
       FieldConfiguration --> ByteOrder : uses
       NamedFieldConfiguration --|> FieldConfiguration : extends
       MessageParser --> FieldConfiguration : operates on
       MessageParser --> BitFieldConfiguration : operates on
       MessageParser --> ParseError : returns
       MessageParser --> FrameDefinition : uses
       MessageParser --> MessageRegistry : contains
       MessageRegistry --> MessageEntry : contains

Parsing Pipeline
----------------

When ``convertByteType`` is called, the following steps occur:

.. mermaid::

   sequenceDiagram
       participant User
       participant convertByteType
       participant ByteAssembly
       participant RangeChecker

       User->>convertByteType: msg, field
       alt Single byte field
           convertByteType->>convertByteType: static_cast<Type>(msg[index])
       else Multi-byte field
           convertByteType->>ByteAssembly: Assemble bytes
           alt BigEndian
               ByteAssembly->>ByteAssembly: MSB first assembly
           else LittleEndian
               ByteAssembly->>ByteAssembly: LSB first assembly
           end
           ByteAssembly-->>convertByteType: assembled value
       end
       convertByteType->>RangeChecker: validate(value)
       alt MinMaxRange
           RangeChecker->>RangeChecker: check min ≤ value ≤ max
       else SpecificRange
           RangeChecker->>RangeChecker: find value in allowed set
       else CustomRange
           RangeChecker->>RangeChecker: invoke predicate(value)
       end
       RangeChecker-->>convertByteType: expected<T, ParseError>
       convertByteType-->>User: result

Serialization Pipeline
----------------------

When ``encodeField`` or ``encodeFieldChecked`` is called:

.. mermaid::

   sequenceDiagram
       participant User
       participant encodeFieldChecked
       participant RangeChecker
       participant ByteWriter

       User->>encodeFieldChecked: msg, field, value
       encodeFieldChecked->>RangeChecker: validate(value)
       alt Valid
           RangeChecker-->>encodeFieldChecked: OK
           encodeFieldChecked->>ByteWriter: write bytes
           alt Enum type
               ByteWriter->>ByteWriter: std::to_underlying(value)
           end
           alt BigEndian
               ByteWriter->>ByteWriter: MSB first
           else LittleEndian
               ByteWriter->>ByteWriter: LSB first
           end
           ByteWriter-->>encodeFieldChecked: done
           encodeFieldChecked-->>User: expected<void>{}
       else Invalid
           RangeChecker-->>encodeFieldChecked: ParseError
           encodeFieldChecked-->>User: unexpected(error)
       end

Framing Pipeline
----------------

Message framing adds structure around raw payload bytes:

.. mermaid::

   sequenceDiagram
       participant Sender
       participant buildFrame
       participant CRC
       participant Receiver
       participant parseFrame

       Sender->>buildFrame: payload bytes
       buildFrame->>buildFrame: prepend header + length
       opt CRC enabled
           buildFrame->>CRC: compute CRC-8 of payload
           CRC-->>buildFrame: CRC byte
           buildFrame->>buildFrame: append CRC
       end
       buildFrame->>buildFrame: append trailer
       buildFrame-->>Sender: framed message

       Sender->>Receiver: transmit frame
       Receiver->>parseFrame: raw bytes
       parseFrame->>parseFrame: check header byte
       parseFrame->>parseFrame: read length
       parseFrame->>parseFrame: verify size
       opt CRC enabled
           parseFrame->>CRC: verify CRC-8
           CRC-->>parseFrame: pass/fail
       end
       parseFrame->>parseFrame: check trailer byte
       parseFrame-->>Receiver: span<payload> or error

Message Dispatch Flow
---------------------

.. mermaid::

   sequenceDiagram
       participant App
       participant Registry
       participant Handler1
       participant Handler2

       App->>Registry: dispatch(id=0x02, payload)
       Registry->>Registry: scan entries
       alt id matches entry 1
           Registry->>Handler1: invoke parser(payload)
       else id matches entry 2
           Registry->>Handler2: invoke parser(payload)
       else no match
           Registry-->>App: return false
       end
       Registry-->>App: return true

Template Parameter Flow
-----------------------

.. mermaid::

   graph LR
       subgraph "FieldConfiguration Template Parameters"
           P1[Index: size_t] --> FC[FieldConfiguration]
           P2[FieldType: typename] --> FC
           P3[Range: auto] --> FC
           P4[Order: ByteOrder] --> FC
       end

       subgraph "Compile-time Deduction"
           FC --> BL[byteLength = sizeof FieldType]
           FC --> BI[byteIndex = Index]
           FC --> RD[range = Range]
           FC --> BO[byteOrder = Order]
       end

.. mermaid::

   graph LR
       subgraph "BitFieldConfiguration Template Parameters"
           P1[ByteIdx: size_t] --> BF[BitFieldConfiguration]
           P2[BitOffset: size_t] --> BF
           P3[BitWidth: size_t] --> BF
           P4[FieldType: typename] --> BF
           P5[Range: auto] --> BF
       end

       subgraph "Compile-time Deduction"
           BF --> BL["byteLength = (bitOffset + bitWidth + 7) / 8"]
           BF --> BI[byteIndex = ByteIdx]
           BF --> BO[bitOff = BitOffset]
           BF --> BW[bitLen = BitWidth]
       end

Concept Constraints
-------------------

The library uses C++20/23 concepts to constrain template parameters:

- ``IsSpecificRange`` — matches types with a ``possibleValues`` member
- ``IsMinMaxRange`` — matches types with ``min`` and ``max`` members
- ``IsCustomRange`` — matches types with a ``predicate`` member
- ``IsFieldConfiguration`` — matches types with ``byteIndex``, ``byteLength``, ``type``, and ``range``
- ``IsBitFieldConfiguration`` — matches types with ``byteIndex``, ``bitOff``, ``bitLen``, ``type``, and ``range``
- ``IsNamedFieldConfiguration`` — extends ``IsFieldConfiguration`` with a ``name`` member

These concepts enable clear compile-time error messages when incorrect types are used.
