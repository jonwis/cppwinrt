#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;

// Scalar box_value now produces a local IReference/IPropertyValue instead of hopping to
// combase PropertyValue. These confirm it reports the correct PropertyType, keeps combase-style
// numeric conversion on mismatched getters, and round-trips through unbox_value.
TEST_CASE("reference_boxing")
{
    {
        auto boxed = box_value(42);
        auto pv = boxed.as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::Int32);
        REQUIRE(pv.IsNumericScalar());
        REQUIRE(pv.GetInt32() == 42);
        REQUIRE(pv.GetInt16() == 42);
        REQUIRE(pv.GetDouble() == 42.0);
        REQUIRE(unbox_value<int32_t>(boxed) == 42);
    }

    {
        auto pv = box_value(3.5).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::Double);
        REQUIRE(pv.IsNumericScalar());
        REQUIRE(pv.GetDouble() == 3.5);
        REQUIRE(pv.GetSingle() == 3.5f);
    }

    {
        auto pv = box_value(hstring{ L"hello" }).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::String);
        REQUIRE(!pv.IsNumericScalar());
        REQUIRE(pv.GetString() == L"hello");
        REQUIRE_THROWS_AS(pv.GetInt32(), hresult_not_implemented);
    }

    {
        auto pv = box_value(true).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::Boolean);
        REQUIRE(!pv.IsNumericScalar());
        REQUIRE(pv.GetBoolean());
        REQUIRE_THROWS_AS(pv.GetInt32(), hresult_not_implemented);
    }

    {
        guid const g{ 0x11223344, 0x5566, 0x7788, { 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 } };
        auto pv = box_value(g).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::Guid);
        REQUIRE(!pv.IsNumericScalar());
        REQUIRE(pv.GetGuid() == g);
    }

    {
        auto pv = box_value(static_cast<uint8_t>(7)).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::UInt8);
        REQUIRE(pv.IsNumericScalar());
        REQUIRE(pv.GetUInt8() == 7);
        REQUIRE(unbox_value<uint8_t>(box_value(static_cast<uint8_t>(7))) == 7);
    }
}
