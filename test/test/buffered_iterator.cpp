#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation::Collections;

TEST_CASE("buffered_iterator")
{
    // Iterating through IIterable<T> (which has no GetAt) exercises the buffered
    // GetMany path. Use enough elements to span multiple buffer blocks so a refill
    // is required.
    {
        std::vector<int32_t> expected;
        for (int32_t i = 0; i < 300; ++i)
        {
            expected.push_back(i);
        }

        IIterable<int32_t> iterable = single_threaded_vector<int32_t>(std::vector<int32_t>(expected));

        std::vector<int32_t> observed;
        for (auto&& value : iterable)
        {
            observed.push_back(value);
        }

        REQUIRE(observed == expected);
    }

    // Exactly one element beyond a single block boundary (int32_t block is 128).
    {
        IIterable<int32_t> iterable = single_threaded_vector<int32_t>(std::vector<int32_t>(129, 7));

        uint32_t count = 0;
        for (auto&& value : iterable)
        {
            REQUIRE(value == 7);
            ++count;
        }

        REQUIRE(count == 129);
    }

    // Empty collection yields nothing.
    {
        IIterable<int32_t> iterable = single_threaded_vector<int32_t>();

        uint32_t count = 0;
        for (auto&& value : iterable)
        {
            (void)value;
            ++count;
        }

        REQUIRE(count == 0);
    }

    // A non-trivial element type round-trips through the buffer.
    {
        IIterable<hstring> iterable = single_threaded_vector<hstring>({ L"a", L"b", L"c" });

        std::vector<hstring> observed;
        for (auto&& value : iterable)
        {
            observed.push_back(value);
        }

        REQUIRE(observed.size() == 3);
        REQUIRE(observed[0] == L"a");
        REQUIRE(observed[2] == L"c");
    }
}
