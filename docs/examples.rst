Examples
========

This page shows practical examples of using TypeSafeMessageParser in real-world scenarios.

CAN Bus Message Parsing
------------------------

A typical CAN bus message has an 8-byte data payload. Here's how to define and parse one:

.. code-block:: cpp

   #include "MessageParser.h"
   #include <array>
   #include <cstdint>

   // Engine status message (CAN ID 0x100)
   enum class EngineState : uint8_t {
       Off = 0, Cranking = 1, Running = 2, Stalling = 3
   };

   struct EngineMsg {
       static constexpr auto state = FieldConfiguration<0, EngineState,
           FieldRanges::MinMaxRange{.min = EngineState::Off, .max = EngineState::Stalling}>{};
       static constexpr auto rpm = FieldConfiguration<1, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{0}, .max = uint16_t{8000}}>{};
       static constexpr auto temp = FieldConfiguration<3, uint8_t,
           FieldRanges::MinMaxRange{.min = uint8_t{0}, .max = uint8_t{150}}>{};
       static constexpr auto load = FieldConfiguration<4, uint8_t,
           FieldRanges::MinMaxRange{.min = uint8_t{0}, .max = uint8_t{100}}>{};
       static constexpr auto throttle = FieldConfiguration<5, uint8_t>{};
       static constexpr auto fuelPressure = FieldConfiguration<6, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{200}, .max = uint16_t{5000}}>{};
   };

   void processEngineMessage(const std::array<uint8_t, 8>& data) {
       // Validate all fields at once
       MessageParser::parseMessage(data, EngineMsg::state, EngineMsg::rpm,
           EngineMsg::temp, EngineMsg::load, EngineMsg::throttle, EngineMsg::fuelPressure);

       auto engineState = MessageParser::convertByteType(data, EngineMsg::state);
       auto rpmValue = MessageParser::convertByteType(data, EngineMsg::rpm);

       if (engineState && rpmValue) {
           // Both parsed successfully with valid ranges
       }
   }

Sensor Protocol with Little-Endian
-----------------------------------

Some sensor protocols use little-endian byte order:

.. code-block:: cpp

   #include "MessageParser.h"

   // Sensor data packet (little-endian protocol)
   struct SensorPacket {
       // Temperature in 0.01°C units, LE
       static constexpr auto temperature = FieldConfiguration<0, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{0}, .max = uint16_t{12000}},
           ByteOrder::LittleEndian>{};

       // Humidity in 0.1% units, LE
       static constexpr auto humidity = FieldConfiguration<2, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{0}, .max = uint16_t{1000}},
           ByteOrder::LittleEndian>{};

       // Pressure in Pa, LE 32-bit
       static constexpr auto pressure = FieldConfiguration<4, uint32_t,
           FieldRanges::MinMaxRange{.min = uint32_t{80000}, .max = uint32_t{120000}},
           ByteOrder::LittleEndian>{};
   };

   void readSensor(const std::array<uint8_t, 8>& raw) {
       auto [temp, hum, pres] = MessageParser::convertAll(raw,
           SensorPacket::temperature, SensorPacket::humidity, SensorPacket::pressure);

       if (MessageParser::validateMessage(raw,
           SensorPacket::temperature, SensorPacket::humidity, SensorPacket::pressure)) {
           // All readings are within expected ranges
       }
   }

Command/Response Protocol
-------------------------

Define both command and response message formats:

.. code-block:: cpp

   #include "MessageParser.h"

   enum class CommandId : uint8_t {
       Ping = 0x01, SetMode = 0x02, GetStatus = 0x03, Reset = 0xFF
   };

   enum class ResponseCode : uint8_t {
       Ok = 0x00, InvalidCmd = 0x01, Busy = 0x02, Error = 0xFF
   };

   // Command message format
   struct CommandMsg {
       static constexpr auto cmdId = FieldConfiguration<0, CommandId,
           FieldRanges::SpecificRange{CommandId::Ping, CommandId::SetMode,
                                      CommandId::GetStatus, CommandId::Reset}>{};
       static constexpr auto payload = FieldConfiguration<1, uint8_t>{};
   };

   // Response message format
   struct ResponseMsg {
       static constexpr auto code = FieldConfiguration<0, ResponseCode,
           FieldRanges::MinMaxRange{.min = ResponseCode::Ok, .max = ResponseCode::Error}>{};
       static constexpr auto data = FieldConfiguration<1, uint16_t>{};
   };

Compile-Time Validation
-----------------------

Leverage ``constexpr`` to validate protocol definitions at compile time:

.. code-block:: cpp

   #include "MessageParser.h"

   struct Protocol {
       static constexpr auto header = FieldConfiguration<0, uint8_t,
           FieldRanges::SpecificRange{uint8_t{0xAA}, uint8_t{0x55}}>{};
       static constexpr auto length = FieldConfiguration<1, uint8_t,
           FieldRanges::MinMaxRange{.min = uint8_t{1}, .max = uint8_t{64}}>{};
       static constexpr auto payload = FieldConfiguration<2, uint16_t>{};
       static constexpr auto checksum = FieldConfiguration<4, uint8_t>{};
   };

   // Verify message structure at compile time
   constexpr std::array<uint8_t, 5> validMsg = {0xAA, 0x03, 0x01, 0x02, 0xA6};
   static_assert(MessageParser::getSize<
       decltype(Protocol::header),
       decltype(Protocol::length),
       decltype(Protocol::payload),
       decltype(Protocol::checksum)>() == 5);

   // Verify a known-good message parses correctly at compile time
   static_assert(MessageParser::convertByteType(validMsg, Protocol::header).has_value());
   static_assert(*MessageParser::convertByteType(validMsg, Protocol::header) == 0xAA);

Error Handling
--------------

Properly handle all error cases:

.. code-block:: cpp

   #include "MessageParser.h"
   #include <format>
   #include <iostream>

   void handleField(const std::array<uint8_t, 2>& msg) {
       static constexpr auto field = FieldConfiguration<0, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{100}, .max = uint16_t{5000}}>{};

       auto result = MessageParser::convertByteType(msg, field);
       if (result.has_value()) {
           std::cout << "Value: " << *result << '\n';
       } else {
           // Using std::format integration
           std::cout << std::format("Error: {}\n", result.error());
           // Or using operator<<
           std::cout << "Error: " << result.error() << '\n';

           switch (result.error()) {
               case FieldRanges::ParseError::BelowRange:
                   std::cout << "Value too low\n"; break;
               case FieldRanges::ParseError::AboveRange:
                   std::cout << "Value too high\n"; break;
               case FieldRanges::ParseError::ValueNotExist:
                   std::cout << "Value not in allowed set\n"; break;
               case FieldRanges::ParseError::InvalidSize:
                   std::cout << "Message size mismatch\n"; break;
               case FieldRanges::ParseError::ChecksumMismatch:
                   std::cout << "CRC/checksum failed\n"; break;
               case FieldRanges::ParseError::CustomValidationFailed:
                   std::cout << "Custom predicate failed\n"; break;
           }
       }
   }

Custom Validator Predicates
---------------------------

Use lambda predicates for complex validation rules:

.. code-block:: cpp

   #include "MessageParser.h"

   // Only accept values that are multiples of 4 (alignment constraint)
   constexpr auto isAligned = [](auto val) constexpr { return val % 4 == 0; };

   // Only accept power-of-two values
   constexpr auto isPowerOf2 = [](auto val) constexpr {
       return val > 0 && (val & (val - 1)) == 0;
   };

   struct AlignedFields {
       static constexpr auto offset = FieldConfiguration<0, uint16_t,
           FieldRanges::CustomRange<uint16_t, decltype(isAligned)>{isAligned}>{};
       static constexpr auto blockSize = FieldConfiguration<2, uint8_t,
           FieldRanges::CustomRange<uint8_t, decltype(isPowerOf2)>{isPowerOf2}>{};
   };

   void validatePacket(const std::array<uint8_t, 3>& data) {
       auto offset = MessageParser::convertByteType(data, AlignedFields::offset);
       // offset succeeds only if the uint16_t value is divisible by 4

       auto block = MessageParser::convertByteType(data, AlignedFields::blockSize);
       // block succeeds only if it's a power of 2 (1, 2, 4, 8, 16, ...)
   }

Bit-Field Parsing (CAN Bus)
----------------------------

CAN bus protocols often pack multiple fields into single bytes:

.. code-block:: cpp

   #include "MessageParser.h"

   // CAN message with bit-level fields:
   // Byte 0: [priority:3][reserved:1][direction:2][mode:2]
   struct CanControlMsg {
       static constexpr auto priority = BitFieldConfiguration<0, 0, 3, uint8_t,
           FieldRanges::MinMaxRange{.min = uint8_t{0}, .max = uint8_t{7}}>{};
       static constexpr auto direction = BitFieldConfiguration<0, 4, 2, uint8_t,
           FieldRanges::SpecificRange{uint8_t{0}, uint8_t{1}, uint8_t{2}}>{};
       static constexpr auto mode = BitFieldConfiguration<0, 6, 2, uint8_t>{};
   };

   void processCanFrame(const std::array<uint8_t, 8>& data) {
       auto prio = MessageParser::convertBitField(data, CanControlMsg::priority);
       auto dir  = MessageParser::convertBitField(data, CanControlMsg::direction);
       auto mode = MessageParser::convertBitField(data, CanControlMsg::mode);

       if (prio && dir && mode) {
           // All bit-fields parsed and validated
       }
   }

Signed Integer and Float Parsing
---------------------------------

Parse signed integers and IEEE 754 floating-point values:

.. code-block:: cpp

   #include "MessageParser.h"

   struct SensorData {
       // Signed temperature: -40°C to +125°C
       static constexpr auto temperature = FieldConfiguration<0, int16_t,
           FieldRanges::MinMaxRange{.min = int16_t{-4000}, .max = int16_t{12500}},
           ByteOrder::LittleEndian>{};

       // Signed acceleration: -2g to +2g (in milli-g)
       static constexpr auto accelX = FieldConfiguration<2, int16_t,
           FieldRanges::MinMaxRange{.min = int16_t{-2000}, .max = int16_t{2000}},
           ByteOrder::LittleEndian>{};
   };

   void readImuData(const std::array<uint8_t, 8>& raw) {
       // Parse signed integers
       auto temp = MessageParser::convertByteType(raw, SensorData::temperature);

       // Parse IEEE 754 float (bytes 4-7)
       auto pressure = MessageParser::getFloatField<4, ByteOrder::LittleEndian>(raw);
   }

Message Serialization (Encode)
------------------------------

Build messages from typed values:

.. code-block:: cpp

   #include "MessageParser.h"

   enum class Command : uint8_t { Start = 1, Stop = 2, Reset = 3 };

   struct CmdMsg {
       static constexpr auto cmd = FieldConfiguration<0, Command,
           FieldRanges::MinMaxRange{.min = Command::Start, .max = Command::Reset}>{};
       static constexpr auto param = FieldConfiguration<1, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{0}, .max = uint16_t{10000}}>{};
       static constexpr auto flags = FieldConfiguration<3, uint8_t>{};
   };

   auto buildCommand(Command cmd, uint16_t param, uint8_t flags) {
       std::array<uint8_t, 4> msg{};

       // Encode with validation
       auto r1 = MessageParser::encodeFieldChecked(msg, CmdMsg::cmd, cmd);
       auto r2 = MessageParser::encodeFieldChecked(msg, CmdMsg::param, param);
       if (!r1 || !r2) {
           // Handle validation error
       }

       // Encode without validation (when you trust the input)
       MessageParser::encodeField(msg, CmdMsg::flags, flags);

       return msg;
   }

   // Roundtrip: encode then decode
   void roundtrip() {
       auto msg = buildCommand(Command::Start, 5000, 0xFF);
       auto cmd = MessageParser::convertByteType(msg, CmdMsg::cmd);
       // *cmd == Command::Start
   }

Cross-Field Validation
----------------------

Validate constraints spanning multiple fields:

.. code-block:: cpp

   #include "MessageParser.h"

   struct MotorMsg {
       static constexpr auto status = FieldConfiguration<0, uint8_t>{};
       static constexpr auto rpm = FieldConfiguration<1, uint16_t>{};
       static constexpr auto current = FieldConfiguration<3, uint16_t>{};
   };

   void validateMotorMessage(const std::array<uint8_t, 5>& msg) {
       // Single cross-field rule: if motor is off (status=0), rpm must be 0
       bool valid = MessageParser::validateCrossField(msg, [](const auto& m) {
           auto status = MessageParser::convertByteType(m, MotorMsg::status);
           auto rpm = MessageParser::convertByteType(m, MotorMsg::rpm);
           if (status.has_value() && *status == 0)
               return rpm.has_value() && *rpm == 0;
           return true;
       });

       // Multiple rules: all must pass
       bool allValid = MessageParser::validateCrossFields(msg,
           // Rule 1: if off, rpm must be 0
           [](const auto& m) {
               auto s = MessageParser::convertByteType(m, MotorMsg::status);
               auto r = MessageParser::convertByteType(m, MotorMsg::rpm);
               return !(s && *s == 0) || (r && *r == 0);
           },
           // Rule 2: current must be less than rpm
           [](const auto& m) {
               auto r = MessageParser::convertByteType(m, MotorMsg::rpm);
               auto c = MessageParser::convertByteType(m, MotorMsg::current);
               return !(r && c) || *c <= *r;
           }
       );
   }

CRC-Protected Protocol
----------------------

Parse messages with integrity verification:

.. code-block:: cpp

   #include "MessageParser.h"

   // Message format: [data: 4 bytes] [crc8: 1 byte]
   void parseWithCrc(const std::array<uint8_t, 5>& msg) {
       // Verify CRC-8 of bytes [0..3] against byte[4]
       auto crcOk = MessageParser::verifyCrc8<0, 4, 4>(msg);
       if (!crcOk) {
           // CRC mismatch - corrupted message
           return;
       }
       // Now safe to parse the data fields
       static constexpr auto value = FieldConfiguration<0, uint32_t>{};
       auto result = MessageParser::convertByteType(msg, value);
   }

   // Build a CRC-protected message
   void buildWithCrc() {
       std::array<uint8_t, 5> msg{};
       static constexpr auto value = FieldConfiguration<0, uint32_t>{};
       MessageParser::encodeField(msg, value, uint32_t{0x12345678});

       // Compute and append CRC
       auto crc = MessageParser::computeCrc8(std::span<const uint8_t>(msg.data(), 4));
       msg[4] = crc;
   }

Message Framing
---------------

Use the framing system for serial/UART protocols:

.. code-block:: cpp

   #include "MessageParser.h"

   // Define frame format: header=0xAA, trailer=0x55, max 32 bytes, CRC enabled
   using MyFrame = MessageParser::FrameDefinition<0xAA, 0x55, 32, true>;

   // Build a framed message
   void sendMessage(const std::array<uint8_t, 4>& payload) {
       auto frame = MessageParser::buildFrame<MyFrame>(payload);
       // frame = [0xAA][0x04][payload...][CRC-8][0x55]
       // transmit(frame);
   }

   // Parse a received frame
   void receiveMessage(std::span<const uint8_t> rawData) {
       auto result = MessageParser::parseFrame<MyFrame>(rawData);
       if (result.has_value()) {
           std::span<const uint8_t> payload = *result;
           // Parse fields from payload
           static constexpr auto cmd = FieldConfiguration<0, uint8_t>{};
           auto command = MessageParser::convertByteType(payload, cmd);
       } else {
           // Frame error: wrong header, bad CRC, size mismatch, etc.
       }
   }

Message ID Dispatch
-------------------

Route messages to type-safe handlers based on ID:

.. code-block:: cpp

   #include "MessageParser.h"
   #include <functional>
   #include <span>

   // Define message handlers
   void handlePing(std::span<const uint8_t> payload) {
       // Process ping message
   }

   void handleStatus(std::span<const uint8_t> payload) {
       static constexpr auto temp = FieldConfiguration<0, uint16_t>{};
       auto temperature = MessageParser::convertByteType(payload, temp);
   }

   // Build the registry
   auto registry = MessageParser::makeRegistry<uint8_t>(
       MessageParser::MessageEntry<uint8_t, 0x01,
           std::function<void(std::span<const uint8_t>)>>{.parser = handlePing},
       MessageParser::MessageEntry<uint8_t, 0x02,
           std::function<void(std::span<const uint8_t>)>>{.parser = handleStatus}
   );

   // Dispatch incoming messages
   void onMessageReceived(uint8_t id, std::span<const uint8_t> payload) {
       if (!registry.dispatch(id, payload)) {
           // Unknown message ID
       }
   }

Using std::span
---------------

All parsing functions work with runtime-sized buffers via ``std::span``:

.. code-block:: cpp

   #include "MessageParser.h"
   #include <span>
   #include <vector>

   struct Fields {
       static constexpr auto header = FieldConfiguration<0, uint8_t>{};
       static constexpr auto value = FieldConfiguration<1, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{0}, .max = uint16_t{10000}}>{};
   };

   void processBuffer(const std::vector<uint8_t>& buffer) {
       std::span<const uint8_t> msg(buffer.data(), buffer.size());

       auto hdr = MessageParser::convertByteType(msg, Fields::header);
       auto val = MessageParser::convertByteType(msg, Fields::value);

       // Batch operations also work
       auto all = MessageParser::convertAll(msg, Fields::header, Fields::value);
       bool valid = MessageParser::validateMessage(msg, Fields::header, Fields::value);
   }

Named Fields and Reflection
----------------------------

Use named fields and iteration for debugging/logging:

.. code-block:: cpp

   #include "MessageParser.h"
   #include <format>
   #include <iostream>

   static constexpr auto tempField = NamedFieldConfiguration<0, uint16_t,
       FieldRanges::MinMaxRange{.min = uint16_t{0}, .max = uint16_t{1000}}>{.name = "temperature"};
   static constexpr auto pressField = NamedFieldConfiguration<2, uint16_t,
       FieldRanges::MinMaxRange{.min = uint16_t{900}, .max = uint16_t{1100}}>{.name = "pressure"};

   void logFields() {
       // Access field names at compile time
       std::cout << std::format("Field: {}, offset: {}\n", tempField.name, tempField.byteIndex);

       // Iterate over fields
       MessageParser::forEachFieldIndexed([](std::size_t idx, const auto& field) {
           std::cout << std::format("  [{}] byteIndex={}, size={}\n",
               idx, field.byteIndex, field.byteLength);
       }, tempField, pressField);

       std::cout << std::format("Total fields: {}\n",
           MessageParser::fieldCount<decltype(tempField), decltype(pressField)>());
   }

Conditional/Optional Fields
----------------------------

Parse fields that may or may not be present:

.. code-block:: cpp

   #include "MessageParser.h"

   struct VariableMsg {
       static constexpr auto header = FieldConfiguration<0, uint8_t>{};
       static constexpr auto length = FieldConfiguration<1, uint8_t>{};
       static constexpr auto payload1 = FieldConfiguration<2, uint16_t>{};
       static constexpr auto payload2 = FieldConfiguration<4, uint16_t>{}; // optional
   };

   void parseVariable(const std::array<uint8_t, 6>& msg) {
       auto len = MessageParser::convertByteType(msg, VariableMsg::length);
       bool hasSecondPayload = len.has_value() && *len > 2;

       // Parse only if the message indicates this field is present
       auto opt = MessageParser::convertByteTypeIf(msg, VariableMsg::payload2, hasSecondPayload);
       if (opt.has_value() && opt->has_value()) {
           uint16_t value = **opt; // The optional field was present and valid
       }
   }
