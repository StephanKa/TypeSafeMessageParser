#include <MessageParser.h>

// example
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

// example message definition
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

// Message type
struct Message
{
    uint16_t id;
    std::array<std::uint8_t, 10> msg;
};

int main()
{
    constexpr Message msg{.id=42, .msg={0x1, 0x3, 0x1, 0x0, 0x4, 0x5, 0xA, 0x5, 0xCF, 0xFF}};
    MessageParser::parseMessage(msg, DataFields::A, DataFields::B, DataFields::C, DataFields::D, DataFields::E, DataFields::F, DataFields::G, DataFields::H, DataFields::I);
    constexpr auto type = MessageParser::convertByteType(msg, DataFields::B);
    if constexpr (type.has_value())
    {
        spdlog::info("type: {}", static_cast<uint32_t>(type.value()));
    }

    constexpr auto typeH = MessageParser::convertByteType(msg, DataFields::H);
    if constexpr (typeH.has_value())
    {
        spdlog::info("typeH: {}", static_cast<uint32_t>(typeH.value()));
    }
    constexpr auto typeI = MessageParser::convertByteType(msg, DataFields::I);
    if constexpr (typeI.has_value())
    {
        spdlog::info("typeI: {}", static_cast<uint32_t>(typeI.value()));
    }
    return 0;
}
