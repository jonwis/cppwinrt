#include "pch.h"
#include <fstream>
#include <string>
#include <type_traits>
#include <winrt/Test.RangePlan.h>

using namespace winrt;
using namespace winrt::Test::RangePlan;

template<typename T>
concept has_grump = requires(T& value)
{
    value.Grump();
};

template<typename T>
concept has_oops = requires(T& value)
{
    value.Oops();
};

static_assert(requires(MyType value)
{
    value.as<IMyType>();
    value.as<IAlwaysVisible>();
    value.try_as<IMiddleVisible>();
});

static_assert(requires(IMiddleVisible value)
{
    value.Grump();
});

static_assert(requires(MyType value)
{
    value.as<IMiddleVisible>().Grump();
});

static_assert(requires(MyType value)
{
    value.template as<IMyType>().AlwaysVisible();
});

static_assert(!has_grump<MyType>);
static_assert(!has_oops<MyType>);
static_assert(!std::is_convertible_v<MyType, INotProjected>);
static_assert(!std::is_convertible_v<MyType, IMiddleVisible>);
static_assert(std::is_convertible_v<MyType, IAlwaysVisible>);

TEST_CASE("range_plan_filtered_projection_shape [projection-range-plan]")
{
    std::ifstream header(CPPWINRT_RANGE_PLAN_FILTERED_HEADER);
    REQUIRE(header.is_open());

    std::string text((std::istreambuf_iterator<char>(header)), std::istreambuf_iterator<char>());

    REQUIRE(text.find("struct HiddenType") == std::string::npos);
    REQUIRE(text.find("struct EmptySurface") != std::string::npos);
    REQUIRE(text.find("IOutOfRangeBase") == std::string::npos);
}
