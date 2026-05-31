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
   #include <iostream>

   void handleField(const std::array<uint8_t, 2>& msg) {
       static constexpr auto field = FieldConfiguration<0, uint16_t,
           FieldRanges::MinMaxRange{.min = uint16_t{100}, .max = uint16_t{5000}}>{};

       auto result = MessageParser::convertByteType(msg, field);
       if (result.has_value()) {
           std::cout << "Value: " << *result << '\n';
       } else {
           std::cout << "Error: " << FieldRanges::to_string(result.error()) << '\n';
           switch (result.error()) {
               case FieldRanges::ParseError::BelowRange:
                   std::cout << "Value too low\n"; break;
               case FieldRanges::ParseError::AboveRange:
                   std::cout << "Value too high\n"; break;
               case FieldRanges::ParseError::ValueNotExist:
                   std::cout << "Value not in allowed set\n"; break;
               case FieldRanges::ParseError::InvalidSize:
                   std::cout << "Message size mismatch\n"; break;
           }
       }
   }
