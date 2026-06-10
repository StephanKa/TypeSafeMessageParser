#include <MessageParser.h>
#include <array>
#include <catch2/catch_all.hpp>
#include <cstdint>
#include <span>

// ===========================================================================
// Benchmark: Single-field parsing
// ===========================================================================

TEST_CASE("Benchmark single uint8_t field parsing", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    std::array<uint8_t, 1> msg = { 0x42 };

    BENCHMARK("parse uint8_t field")
    {
        return MessageParser::convertByteType(msg, field);
    };
}

TEST_CASE("Benchmark single uint16_t big-endian parsing", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }>{};
    std::array<uint8_t, 2> msg = { 0x12, 0x34 };

    BENCHMARK("parse uint16_t BE field")
    {
        return MessageParser::convertByteType(msg, field);
    };
}

TEST_CASE("Benchmark single uint16_t little-endian parsing", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::LittleEndian>{};
    std::array<uint8_t, 2> msg = { 0x34, 0x12 };

    BENCHMARK("parse uint16_t LE field")
    {
        return MessageParser::convertByteType(msg, field);
    };
}

TEST_CASE("Benchmark uint32_t field parsing", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint32_t>{};
    std::array<uint8_t, 4> msg = { 0xDE, 0xAD, 0xBE, 0xEF };

    BENCHMARK("parse uint32_t field")
    {
        return MessageParser::convertByteType(msg, field);
    };
}

// ===========================================================================
// Benchmark: Multi-field parsing
// ===========================================================================

TEST_CASE("Benchmark multi-field message parsing", "[benchmark]")
{
    enum class Status : uint8_t
    {
        OK = 0,
        WARN = 1,
        ERR = 2
    };
    static constexpr auto f1 = FieldConfiguration<0, Status, FieldRanges::MinMaxRange{ .min = Status::OK, .max = Status::ERR }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 10000 } }>{};
    static constexpr auto f3 = FieldConfiguration<3, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    static constexpr auto f4 = FieldConfiguration<4, uint32_t>{};

    std::array<uint8_t, 8> msg = { 0x01, 0x01, 0xF4, 0x42, 0x00, 0x00, 0x10, 0x00 };

    BENCHMARK("convertAll 4 fields")
    {
        return MessageParser::convertAll(msg, f1, f2, f3, f4);
    };

    BENCHMARK("validateMessage 4 fields")
    {
        return MessageParser::validateMessage(msg, f1, f2, f3, f4);
    };
}

// ===========================================================================
// Benchmark: Range validation types
// ===========================================================================

TEST_CASE("Benchmark MinMaxRange validation", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 10 }, .max = uint8_t{ 200 } }>{};
    std::array<uint8_t, 1> msg = { 100 };

    BENCHMARK("MinMaxRange check")
    {
        return MessageParser::convertByteType(msg, field);
    };
}

TEST_CASE("Benchmark SpecificRange validation", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint8_t, FieldRanges::SpecificRange{ uint8_t{ 10 }, uint8_t{ 20 }, uint8_t{ 30 }, uint8_t{ 40 }, uint8_t{ 50 } }>{};
    std::array<uint8_t, 1> msg = { 30 };

    BENCHMARK("SpecificRange check (5 values)")
    {
        return MessageParser::convertByteType(msg, field);
    };
}

TEST_CASE("Benchmark CustomRange validation", "[benchmark]")
{
    constexpr auto isEven = [](auto val) constexpr { return val % 2 == 0; };
    static constexpr auto field = FieldConfiguration<0, uint8_t, FieldRanges::CustomRange<uint8_t, decltype(isEven)>{ isEven }>{};
    std::array<uint8_t, 1> msg = { 42 };

    BENCHMARK("CustomRange predicate check")
    {
        return MessageParser::convertByteType(msg, field);
    };
}

// ===========================================================================
// Benchmark: Serialization (encode)
// ===========================================================================

TEST_CASE("Benchmark encode single field", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint16_t>{};
    std::array<uint8_t, 2> msg{};

    BENCHMARK("encode uint16_t")
    {
        MessageParser::encodeField(msg, field, uint16_t{ 0x1234 });
        return msg;
    };
}

TEST_CASE("Benchmark encode with validation", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 200 } }>{};
    std::array<uint8_t, 1> msg{};

    BENCHMARK("encodeFieldChecked")
    {
        return MessageParser::encodeFieldChecked(msg, field, uint8_t{ 100 });
    };
}

// ===========================================================================
// Benchmark: CRC computation
// ===========================================================================

TEST_CASE("Benchmark CRC-8 computation", "[benchmark]")
{
    std::array<uint8_t, 8> data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

    BENCHMARK("CRC-8 over 8 bytes")
    {
        return MessageParser::computeCrc8(std::span<const uint8_t>(data));
    };
}

TEST_CASE("Benchmark CRC-16 computation", "[benchmark]")
{
    std::array<uint8_t, 8> data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

    BENCHMARK("CRC-16 over 8 bytes")
    {
        return MessageParser::computeCrc16(std::span<const uint8_t>(data));
    };
}

TEST_CASE("Benchmark CRC-32 computation", "[benchmark]")
{
    std::array<uint8_t, 8> data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

    BENCHMARK("CRC-32 over 8 bytes")
    {
        return MessageParser::computeCrc32(std::span<const uint8_t>(data));
    };
}

TEST_CASE("Benchmark CRC-32 over 64 bytes", "[benchmark]")
{
    std::array<uint8_t, 64> data{};
    for (std::size_t i = 0; i < 64; ++i)
    {
        data[i] = static_cast<uint8_t>(i);
    }

    BENCHMARK("CRC-32 over 64 bytes")
    {
        return MessageParser::computeCrc32(std::span<const uint8_t>(data));
    };
}

// ===========================================================================
// Benchmark: Frame parsing
// ===========================================================================

TEST_CASE("Benchmark frame parsing without CRC", "[benchmark]")
{
    using Frame = MessageParser::FrameDefinition<0xAA, 0x55, 16, false>;
    constexpr std::array<uint8_t, 3> payload = { 0x01, 0x02, 0x03 };
    auto frame = MessageParser::buildFrame<Frame>(payload);

    BENCHMARK("parseFrame (no CRC)")
    {
        return MessageParser::parseFrame<Frame>(std::span<const uint8_t>(frame));
    };
}

TEST_CASE("Benchmark frame parsing with CRC", "[benchmark]")
{
    using Frame = MessageParser::FrameDefinition<0xAA, 0x55, 16, true>;
    constexpr std::array<uint8_t, 4> payload = { 0x10, 0x20, 0x30, 0x40 };
    auto frame = MessageParser::buildFrame<Frame>(payload);

    BENCHMARK("parseFrame (with CRC-8)")
    {
        return MessageParser::parseFrame<Frame>(std::span<const uint8_t>(frame));
    };
}

// ===========================================================================
// Benchmark: std::span vs std::array
// ===========================================================================

TEST_CASE("Benchmark span vs array parsing", "[benchmark]")
{
    static constexpr auto field = FieldConfiguration<0, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }>{};
    std::array<uint8_t, 2> data = { 0x12, 0x34 };
    const std::span<const uint8_t> spanMsg(data);

    BENCHMARK("parse from std::array")
    {
        return MessageParser::convertByteType(data, field);
    };

    BENCHMARK("parse from std::span")
    {
        return MessageParser::convertByteType(spanMsg, field);
    };
}

// ===========================================================================
// Benchmark: Bit-field parsing
// ===========================================================================

TEST_CASE("Benchmark bit-field parsing", "[benchmark]")
{
    static constexpr auto bf = BitFieldConfiguration<0, 2, 4, uint8_t>{};
    constexpr std::array<uint8_t, 1> msg = { 0xAB };

    BENCHMARK("parse 4-bit field")
    {
        return MessageParser::convertBitField(msg, bf);
    };
}

// ===========================================================================
// Benchmark: Float parsing
// ===========================================================================

TEST_CASE("Benchmark float field parsing", "[benchmark]")
{
    std::array<uint8_t, 4> msg = { 0x40, 0x48, 0xF5, 0xC3 };// 3.14f

    BENCHMARK("getFloatField BE")
    {
        return MessageParser::getFloatField<0, ByteOrder::BigEndian>(msg);
    };

    BENCHMARK("getFloatField LE")
    {
        constexpr std::array<uint8_t, 4> leMsg = { 0xC3, 0xF5, 0x48, 0x40 };
        return MessageParser::getFloatField<0, ByteOrder::LittleEndian>(leMsg);
    };
}
