#include "pch.h"
#include <type_traits>
#include <winrt/Test.RangePlan.h>

using namespace winrt;
using namespace winrt::Test::RangePlan;

static_assert(requires(MyType value)
{
    value.as<IMyType>();
    value.as<IAlwaysVisible>();
    value.as<IMiddleVisible>();
    value.as<INotProjected>();
});

static_assert(requires(IMyType value)
{
    value.AlwaysVisible();
});

static_assert(requires(IMiddleVisible value)
{
    value.Grump();
});

static_assert(requires(INotProjected value)
{
    value.Oops();
});

static_assert(requires(HiddenType hidden)
{
    hidden.as<IHiddenType>();
    hidden.HiddenMethod();
});

static_assert(false);

TEST_CASE("range_plan_full_projection_shape [projection-range-plan]")
{
    SUCCEED();
}
