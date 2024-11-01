#include <MessageParser.h>
#include <catch2/catch_all.hpp>


// WORKAROUND for clang llvm issue: https://github.com/llvm/llvm-project/issues/113087
namespace std {
    template<> struct tuple_size<Catch::Decomposer> { static constexpr size_t value = 1; };
}

TEST_CASE("Calculate size with one field")
{
    static constexpr auto val = FieldConfiguration<3, std::uint8_t>{};
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
}

TEST_CASE("Calculate size with two fields")
{
    static constexpr auto val = FieldConfiguration<3, std::uint8_t>{};
    static constexpr auto val2 = FieldConfiguration<1, std::uint32_t>{};
    STATIC_REQUIRE(MessageParser::getSize<decltype(val), decltype(val2)>() == (val.byteLength + val2.byteLength));
}

TEST_CASE("Check MinMaxRange with below lower range data")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = {};
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
    STATIC_REQUIRE(MessageParser::convertByteType(msg, val) == std::nullopt);
}

TEST_CASE("Check MinMaxRange with valid data")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0xAA, 0xFF };
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
    STATIC_REQUIRE(MessageParser::convertByteType(msg, val) == 43775);
}

TEST_CASE("Check MinMaxRange with above upper range data")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0xFF, 0xFF };
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
    REQUIRE(MessageParser::convertByteType(msg, val) == std::nullopt);
}

TEST_CASE("Check MinMaxRange complete range")
{
    using Type = uint8_t;
    constexpr FieldRanges::MinMaxRange range{ .min = 20, .max = 240 };
    static constexpr auto val = FieldConfiguration<0, Type, range>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    for(Type tempValue = range.min; tempValue <= range.max; ++tempValue)
    {
        const Message msg = { tempValue };
        REQUIRE(MessageParser::convertByteType(msg, val) == tempValue);
    }
}

TEST_CASE("Check getField")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0x42 };
    const auto returnValue = MessageParser::getField<decltype(val)>(msg);
    REQUIRE(returnValue == 0x42);
}

TEST_CASE("Check getField with enum")
{
    enum class Error : std::uint8_t
    {
        Info,
        Warning,
        Fatal,
        Critical
    };
    using Type = Error;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0x2 };
    const auto returnValue = MessageParser::getField<decltype(val)>(msg);
    REQUIRE(returnValue == Error::Fatal);
}

TEST_CASE("Check getField with enum and MinMaxRange")
{
    enum class Error
    {
        Info,
        Warning,
        Fatal,
        Critical,
        Unknown
    };
    using Type = Error;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = Error::Warning, .max = Error::Critical }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x0 };
        constexpr auto returnValue = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(returnValue == std::nullopt);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x1 };
        constexpr auto returnValue = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(returnValue == Error::Warning);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x2 };
        constexpr auto returnValue = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(returnValue == Error::Fatal);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x3 };
        constexpr auto returnValue = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(returnValue == Error::Critical);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x4 };
        constexpr auto returnValue = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(returnValue == std::nullopt);
    }
}
