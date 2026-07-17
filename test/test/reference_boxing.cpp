#include "pch.h"
#include <objbase.h>
#include <objidl.h>

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

    // DateTime, TimeSpan, and Point are also boxed in-process now (they still marshal by value).
    {
        Point const point{ 3.0f, 4.0f };
        auto pv = box_value(point).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::Point);
        REQUIRE(!pv.IsNumericScalar());
        REQUIRE(pv.GetPoint().X == point.X);
        REQUIRE(pv.GetPoint().Y == point.Y);
        auto const round_tripped = unbox_value<Point>(box_value(point));
        REQUIRE(round_tripped.X == point.X);
        REQUIRE(round_tripped.Y == point.Y);
        REQUIRE_THROWS_AS(pv.GetInt32(), hresult_not_implemented);
    }

    {
        TimeSpan const span{ std::chrono::seconds{ 90 } };
        auto pv = box_value(span).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::TimeSpan);
        REQUIRE(!pv.IsNumericScalar());
        REQUIRE(pv.GetTimeSpan() == span);
        REQUIRE(unbox_value<TimeSpan>(box_value(span)) == span);
    }

    {
        DateTime const when{ TimeSpan{ std::chrono::seconds{ 1000 } } };
        auto pv = box_value(when).as<IPropertyValue>();
        REQUIRE(pv.Type() == PropertyType::DateTime);
        REQUIRE(!pv.IsNumericScalar());
        REQUIRE(pv.GetDateTime() == when);
        REQUIRE(unbox_value<DateTime>(box_value(when)) == when);
    }
}

// The in-proc reference stays agile but must marshal by value across processes, exactly like a real
// combase PropertyValue. Prove it by confirming our IMarshal reports the SAME unmarshal class as a
// genuine PropertyValue - i.e. we forward marshaling to combase - and specifically NOT the
// free-threaded (marshal-by-reference) class the default agile path would have used.
TEST_CASE("reference_boxing marshal by value")
{
    auto boxed = box_value(42);
    REQUIRE(boxed.try_as<IAgileObject>());
    auto ours = boxed.as<impl::IMarshal>();

    auto genuine = PropertyValue::CreateInt32(42);
    auto reference = genuine.as<impl::IMarshal>();

    guid our_clsid{};
    guid reference_clsid{};
    check_hresult(ours->GetUnmarshalClass(guid_of<IPropertyValue>(), get_unknown(boxed),
        MSHCTX_DIFFERENTMACHINE, nullptr, MSHLFLAGS_NORMAL, &our_clsid));
    check_hresult(reference->GetUnmarshalClass(guid_of<IPropertyValue>(), get_unknown(genuine),
        MSHCTX_DIFFERENTMACHINE, nullptr, MSHLFLAGS_NORMAL, &reference_clsid));

    REQUIRE(our_clsid == reference_clsid);

    // CLSID_InProcFreeMarshaler - the by-reference class the agile FTM would have produced.
    guid const free_threaded_marshaler{ 0x0000033A, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
    REQUIRE(our_clsid != free_threaded_marshaler);
}
