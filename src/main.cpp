#include <MessageParser.h>
#include <array>
#include <cstdint>
#include <spdlog/spdlog.h>

// example enums for message fields
enum class Status : std::uint8_t
{
    OK,
    FAILED
};

enum class Motor : std::uint8_t
{
    OK,
    FAILED,
    UNKNOWN,
    ERROR,
    SUCCESS
};

enum class Fan : std::uint8_t
{
    OK,
    FAILED
};

enum class Timer : std::uint8_t
{
    OK,
    FAILED,
    STARTED,
    STOPPED,
    RESET,
    EXIT
};

// example message definition using BigEndian (default) and LittleEndian fields
struct DataFields
{
    static constexpr auto A = FieldConfiguration<0, Status>{};
    static constexpr auto B = FieldConfiguration<1, Motor, FieldRanges::MinMaxRange{.min=Motor::OK, .max=Motor::SUCCESS}>{};
    static constexpr auto C = FieldConfiguration<2, Fan>{};
    static constexpr auto D = FieldConfiguration<3, std::uint8_t>{};
    static constexpr auto E = FieldConfiguration<4, std::uint8_t>{};
    static constexpr auto F = FieldConfiguration<5, std::uint8_t>{};
    static constexpr auto G = FieldConfiguration<6, std::uint8_t>{};
    static constexpr auto H = FieldConfiguration<7, Timer, FieldRanges::SpecificRange{Timer::OK, Timer::EXIT}>{};
    static constexpr auto I = FieldConfiguration<8, uint16_t, FieldRanges::MinMaxRange{.min=1000, .max=65000}>{};
};

// example with little-endian field
struct LittleEndianFields
{
    static constexpr auto sensor = FieldConfiguration<0, uint16_t, FieldRanges::MinMaxRange{.min=uint16_t{0}, .max=uint16_t{4095}}, ByteOrder::LittleEndian>{};
};

// Message type
struct Message
{
    uint16_t id{};
    std::array<std::uint8_t, 10> data{};
};

int main()
{
    constexpr Message msg{.id=42, .data={0x1, 0x3, 0x1, 0x0, 0x4, 0x5, 0xA, 0x5, 0xCF, 0xFF}};
    MessageParser::parseMessage(msg.data, DataFields::A, DataFields::B, DataFields::C, DataFields::D, DataFields::E, DataFields::F, DataFields::G, DataFields::H, DataFields::I);

    // Validate entire message at compile time
    constexpr bool allValid = MessageParser::validateMessage(msg.data, DataFields::A, DataFields::B, DataFields::C, DataFields::D, DataFields::E, DataFields::F, DataFields::G, DataFields::H, DataFields::I);
    spdlog::info("All fields valid: {}", allValid);

    constexpr auto type = MessageParser::convertByteType(msg.data, DataFields::B);
    if constexpr (type.has_value())
    {
        spdlog::info("type: {}", std::to_underlying(type.value()));
    }

    constexpr auto typeH = MessageParser::convertByteType(msg.data, DataFields::H);
    if constexpr (typeH.has_value())
    {
        spdlog::info("typeH: {}", std::to_underlying(typeH.value()));
    }
    else
    {
        spdlog::error("typeH error: {}", FieldRanges::to_string(typeH.error()));
    }

    constexpr auto typeI = MessageParser::convertByteType(msg.data, DataFields::I);
    if constexpr (typeI.has_value())
    {
        spdlog::info("typeI: {}", typeI.value());
    }

    // Little-endian example
    constexpr std::array<uint8_t, 2> sensorMsg = { 0x34, 0x12 }; // 0x1234 in little-endian
    constexpr auto sensorVal = MessageParser::convertByteType(sensorMsg, LittleEndianFields::sensor);
    if constexpr (sensorVal.has_value())
    {
        spdlog::info("Sensor (LE): {}", sensorVal.value());
    }

    return 0;
}
