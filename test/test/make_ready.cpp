#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;

TEST_CASE("make_ready")
{
    // Completed synchronously with a value, with no coroutine frame.
    {
        IAsyncOperation<int32_t> op = make_ready<int32_t>(42);
        REQUIRE(op.Status() == AsyncStatus::Completed);
        REQUIRE(op.ErrorCode() == 0);
        REQUIRE(op.GetResults() == 42);
        REQUIRE(op.get() == 42);
    }

    // co_await yields the value through the synchronous-completion path.
    {
        auto coro = []() -> IAsyncOperation<int32_t>
        {
            co_return co_await make_ready<int32_t>(7);
        };

        REQUIRE(coro().get() == 7);
    }

    // A Completed handler on an already-completed operation fires immediately.
    {
        auto op = make_ready<int32_t>(5);
        int32_t observed = 0;
        AsyncStatus observed_status = AsyncStatus::Started;

        op.Completed([&](IAsyncOperation<int32_t> const& sender, AsyncStatus status)
        {
            observed = sender.GetResults();
            observed_status = status;
        });

        REQUIRE(observed == 5);
        REQUIRE(observed_status == AsyncStatus::Completed);
    }

    // Assigning Completed twice is illegal, matching the coroutine promise.
    {
        auto op = make_ready<int32_t>(1);
        op.Completed([](auto&&, auto&&) {});
        REQUIRE_THROWS_AS(op.Completed([](auto&&, auto&&) {}), hresult_illegal_delegate_assignment);
    }

    // Action variant carries no result.
    {
        IAsyncAction action = make_ready();
        REQUIRE(action.Status() == AsyncStatus::Completed);
        action.get();
    }

    // A non-trivial result type round-trips.
    {
        auto op = make_ready(hstring{ L"ready" });
        REQUIRE(op.get() == L"ready");
    }
}
