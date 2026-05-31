Architecture
============

This page describes the internal architecture of TypeSafeMessageParser and how messages
are structured, parsed, and validated.

Design Philosophy
-----------------

The library follows these core principles:

1. **Zero runtime cost** — all validation that can happen at compile time does
2. **Type safety** — raw bytes are never exposed without explicit casting
3. **Composability** — fields are independent units that compose into messages
4. **Explicit error handling** — ``std::expected`` forces callers to handle failures

High-Level Architecture
-----------------------

.. mermaid::

   graph TB
       subgraph "User Code"
           A[Define Enums/Types]
           B[Declare FieldConfiguration]
           C[Create Message Array]
       end

       subgraph "MessageParser Library"
           D[parseMessage - Size Validation]
           E[convertByteType - Parse & Validate]
           F[RangeChecker - Range Validation]
           G[getField - Raw Byte Access]
       end

       subgraph "Results"
           H["std::expected&lt;T, ParseError&gt;"]
           I[Compile-time static_assert]
       end

       A --> B
       B --> D
       C --> D
       C --> E
       E --> F
       F --> H
       D --> I
       C --> G

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

       class MinMaxRange~T~ {
           +T min
           +T max
       }

       class SpecificRange~Entries~ {
           +array possibleValues
       }

       class ParseError {
           <<enumeration>>
           ValueNotExist
           BelowRange
           AboveRange
           InvalidSize
       }

       class ByteOrder {
           <<enumeration>>
           BigEndian
           LittleEndian
       }

       class MessageParser {
           +getField~Field~(msg)
           +getSize~Fields~()
           +getMessageSize(msg)
           +parseMessage(msg, fields...)
           +convertByteType(msg, field)
           +convertAll(msg, fields...)
           +validateMessage(msg, fields...)
           +RangeChecker~Field~(value)
           +toUnderlying(value)
       }

       FieldConfiguration --> MinMaxRange : uses
       FieldConfiguration --> SpecificRange : uses
       FieldConfiguration --> ByteOrder : uses
       MessageParser --> FieldConfiguration : operates on
       MessageParser --> ParseError : returns

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
       end
       RangeChecker-->>convertByteType: expected<T, ParseError>
       convertByteType-->>User: result

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

Concept Constraints
-------------------

The library uses C++20/23 concepts to constrain template parameters:

- ``IsSpecificRange`` — matches types with a ``possibleValues`` member
- ``IsMinMaxRange`` — matches types with ``min`` and ``max`` members
- ``IsFieldConfiguration`` — matches types with ``byteIndex``, ``byteLength``, ``type``, and ``range``

These concepts enable clear compile-time error messages when incorrect types are used.
