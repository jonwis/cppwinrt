#include "pch.h"
#include "catch.hpp"
#include <array>
#include "produce_IPropertyValue.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Data::Json;
using namespace Windows::Devices::Sms;
using namespace Windows::Security::Cryptography::Certificates;

//
// First some array tests using the projection (real-world examples).
//

//
// This is a helper to create a data reader for use in testing arrays.
//
static IAsyncOperation<IDataReader> CreateDataReader(std::initializer_list<byte> values)
{
    InMemoryRandomAccessStream stream;
    DataWriter writer(stream);
    writer.WriteByte(1);
    writer.WriteByte(2);
    writer.WriteByte(3);
    co_await writer.StoreAsync();

    stream.Seek(0);
    DataReader reader(stream);
    co_await reader.LoadAsync(3);
    co_return reader;
}

//
// This is a helper to create a JSON array (collection) for testing arrays.
//
static JsonArray CreateJsonArray()
{
    return JsonArray::Parse(LR"(["a","b","c","d","e"])");
}

//
// This test illustrates an span<T const> and a com_array<T> as a retval.
//
TEST_CASE("array,SmsBinaryMessage")
{
    CertificateQuery query;
    query.Thumbprint({ 1, 2, 3 }); // PassArray pattern

    com_array<byte> a = query.Thumbprint(); // ReceiveArray pattern

    REQUIRE(3 == a.size());
    REQUIRE(1 == a[0]);
    REQUIRE(2 == a[1]);
    REQUIRE(3 == a[2]);

    query.Thumbprint(a);
}

//
// This test illustrates a com_array<T> as an out param.
//
TEST_CASE("array,CreateInt32Array,GetInt32Array")
{
    com_array<int> a;
    PropertyValue::CreateInt32Array({ 1, 2, 3 }). // PassArray pattern
        as<IPropertyValue>().GetInt32Array(a); // ReceiveArray pattern

    REQUIRE(3 == a.size());
    REQUIRE(1 == a[0]);
    REQUIRE(2 == a[1]);
    REQUIRE(3 == a[2]);
}

//
// This test illustrates an std::span<T> (non-const) bound to a std::array<T, N>
//
TEST_CASE("array,DataReader")
{
    auto reader = CreateDataReader({1, 2, 3}).get();

    std::array<byte, 3> a;
    reader.ReadBytes(a); // FillArray pattern

    REQUIRE(3 == a.size());
    REQUIRE(1 == a[0]);
    REQUIRE(2 == a[1]);
    REQUIRE(3 == a[2]);
}

//
// This test illustrates an std::span<T> (non-const) bound to a std::vector<T>
//
TEST_CASE("array,DataReader,std::vector")
{
    auto reader = CreateDataReader({ 1, 2, 3 }).get();

    std::vector<byte> a(3);
    reader.ReadBytes(a); // FillArray pattern

    REQUIRE(a.size() == 3);
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 2);
    REQUIRE(a[2] == 3);
}

//
// This test illustrates an std::span<T> (non-const) bound to a custom container
//
TEST_CASE("custom,DataReader")
{
    auto reader = CreateDataReader({ 1, 2, 3 }).get();

    std::array<byte, 3> a;
    byte* ptr = a.data();
    auto size = a.size();
    reader.ReadBytes({ ptr, ptr + size });

    REQUIRE(3 == a.size());
    REQUIRE(1 == a[0]);
    REQUIRE(2 == a[1]);
    REQUIRE(3 == a[2]);
}

//
// This test illustrates an std::span<T> (non-const) bound to a raw buffer
//
TEST_CASE("buffer,DataReader")
{
    auto reader = CreateDataReader({ 1, 2, 3 }).get();

    std::array<byte, 3> a;
    byte* ptr = a.data();
    auto size = a.size();
    reader.ReadBytes({ ptr, static_cast<uint32_t>(size) });

    REQUIRE(3 == a.size());
    REQUIRE(1 == a[0]);
    REQUIRE(2 == a[1]);
    REQUIRE(3 == a[2]);
}

//
// This test illustrates receiving an IVector and calling GetMany to fill an array.
//
TEST_CASE("array,EBO")
{
    //
    // This version calls GetMany on the collection.
    //
    SECTION("collection")
    {
        JsonArray array = CreateJsonArray();

        std::vector<hstring> expected;

        for (auto item : array)
        {
            expected.emplace_back(item.GetString());
        }

        std::vector<IJsonValue> items(expected.size());
        REQUIRE(expected.size() == array.GetMany(0, items));

        std::vector<hstring> actual;

        for (auto && item : items)
        {
            actual.emplace_back(item.GetString());
        }

        REQUIRE(expected == actual);
    }

    //
    // And this version calls GetMany on the iterator.
    //
    SECTION("iterator")
    {
        JsonArray array = CreateJsonArray();

        std::vector<hstring> expected;

        for (auto item : array)
        {
            expected.emplace_back(item.GetString());
        }

        std::vector<IJsonValue> items(expected.size());
        REQUIRE(expected.size() == array.First().GetMany(items));

        std::vector<hstring> actual;

        for (auto && item : items)
        {
            actual.emplace_back(item.GetString());
        }

        REQUIRE(expected == actual);
    }
}

//
// Tests for the front/back methods for com_array
//
TEST_CASE("array,front,back")
{
    SECTION("com_array")
    {
        com_array<int> a{ 1, 2, 3 };

        a.front() = 10;
        a.back() = 30;

        REQUIRE(a.front() == 10);
        REQUIRE(a.back() == 30);
    }
}

//
// Tests for the front/back methods for com_array (const versions).
//
TEST_CASE("array,front,back,const")
{
    SECTION("com_array")
    {
        com_array<int> const a{ 1, 2, 3 };

        REQUIRE(a.front() == 1);
        REQUIRE(a.back() == 3);
    }
}

//
// Tests for the 'data' method for com_array.
//
TEST_CASE("array,data")
{
    SECTION("com_array")
    {
        com_array<int> a{ 1, 2, 3 };

        int * p = a.data();

        p[1] = 20;

        REQUIRE(p[0] == 1);
        REQUIRE(p[1] == 20);
        REQUIRE(p[2] == 3);

        REQUIRE(a[0] == 1);
        REQUIRE(a[1] == 20);
        REQUIRE(a[2] == 3);
    }
}

//
// Tests for the 'data' method for com_array.
//
TEST_CASE("array,data,const")
{
    SECTION("com_array")
    {
        com_array<int> const a{ 1, 2, 3 };

        int const * p = a.data();

        REQUIRE(p[0] == 1);
        REQUIRE(p[1] == 2);
        REQUIRE(p[2] == 3);
    }
}

//
// Tests for the 'begin/end' methods for com_array.
//
TEST_CASE("array,begin,end")
{
    SECTION("com_array")
    {
        com_array<int> a{ 1, 2, 3 };

        auto first = a.begin();
        auto last = a.end();

        for (auto i = first; i != last; ++i)
        {
            *i *= 10;
        }

        REQUIRE(a[0] == 10);
        REQUIRE(a[1] == 20);
        REQUIRE(a[2] == 30);
    }

}

//
// Tests for the 'begin/end' methods for com_array (const versions).
//
TEST_CASE("array,begin,end,const")
{
    SECTION("com_array")
    {
        std::vector<int> v{ 1, 2, 3 };
        com_array<int> const a(v);

        std::vector<int> copy(a.begin(), a.end());
        REQUIRE(v == copy);
    }
}

//
// Tests for the 'rbegin/rend' methods for the com_array.
//
TEST_CASE("array,rbegin,rend")
{
    SECTION("com_array")
    {
        com_array<int> a{ 1, 2, 3 };

        auto first = a.rbegin();
        auto last = a.rend();

        REQUIRE(3 == *first);

        for (auto i = first; i != last; ++i)
        {
            *i *= 10;
        }

        REQUIRE(a[0] == 10);
        REQUIRE(a[1] == 20);
        REQUIRE(a[2] == 30);
    }

}

//
// Tests for the 'rbegin/rend' methods for com_array (const versions).
//
TEST_CASE("array,rbegin,rend,const")
{
    SECTION("com_array")
    {
        std::vector<int> v{ 1, 2, 3 };
        com_array<int> const a(v);

        std::vector<int> copy(a.rbegin(), a.rend());
        std::reverse(v.begin(), v.end());
        REQUIRE(v == copy);
    }
}

//
// Tests for the 'crbegin/crend' methods for the array pattern.
//
TEST_CASE("array,crbegin,crend,const")
{
    SECTION("com_array")
    {
        std::vector<int> v{ 1, 2, 3 };
        com_array<int> const a(v);

        std::vector<int> copy(a.rbegin(), a.rend());
        std::reverse(v.begin(), v.end());
        REQUIRE(v == copy);
    }
}

//
// com_array
//

//
// Tests default construction of com_array.
//
TEST_CASE("com_array,default")
{
    com_array<int> a;
    REQUIRE(a.empty());
    REQUIRE(a.begin() == a.end());
    REQUIRE(a.size() == 0);
    REQUIRE(a.empty());
}

//
// Tests com_array's count constructor (must not be confused with initializer list).
//
TEST_CASE("com_array,count")
{
    com_array<int> a(3);
    REQUIRE(3 == a.size());
    REQUIRE(a[0] == 0);
    REQUIRE(a[1] == 0);
    REQUIRE(a[2] == 0);
}

//
// Tests com_array's N values constructor (must not be confused with initializer list).
//
TEST_CASE("com_array,count,value")
{
    com_array<int> a(3U, 123);
    REQUIRE(3 == a.size());
    REQUIRE(a[0] == 123);
    REQUIRE(a[1] == 123);
    REQUIRE(a[2] == 123);
}

//
// Tests com_array's range constructor.
//
TEST_CASE("com_array,range")
{
    std::vector<int> v { 1, 2, 3 };
    com_array<int> a(v.begin(), v.end());
    REQUIRE(3 == a.size());
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 2);
    REQUIRE(a[2] == 3);
}

//
// Tests com_array's std::vector constructor.
//
TEST_CASE("com_array,vector")
{
    std::vector<int> v{ 1, 2, 3 };
    com_array<int> a(v);
    REQUIRE(3 == a.size());
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 2);
    REQUIRE(a[2] == 3);
}

//
// Tests com_array's std::array constructor.
//
TEST_CASE("com_array,array")
{
    std::array<int, 3> v{ 1, 2, 3 };
    com_array<int> a(v);
    REQUIRE(3 == a.size());
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 2);
    REQUIRE(a[2] == 3);
}

//
// Tests com_array's C-style array constructor.
//
TEST_CASE("com_array,C-style array")
{
    int v[3] { 1, 2, 3 };
    com_array<int> a(v);
    REQUIRE(3 == a.size());
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 2);
    REQUIRE(a[2] == 3);
}

//
// Tests com_array's initializer list constructor.
//
TEST_CASE("com_array,initializer_list")
{
    com_array<int> a { 1, 2, 3 };
    REQUIRE(3 == a.size());
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 2);
    REQUIRE(a[2] == 3);
}

//
// Tests com_array's move constructor and move assignment.
//
TEST_CASE("com_array,move")
{
    com_array<int> a{ 1, 2, 3 };
    com_array<int> b = std::move(a);

    REQUIRE(a.empty());
    REQUIRE(3 == b.size());

    a = std::move(b);

    REQUIRE(3 == a.size());
    REQUIRE(b.empty());
}

//
// Now some tests covering producing and consuming arrays
//

bool operator==(Size left, Size right)
{
    return left.Width == right.Width &&
           left.Height == right.Height;
}

TEST_CASE("array,PropertyValue")
{
    SECTION("consume,array,int32_t")
    {
        auto inspectable = PropertyValue::CreateInt32Array({ 1, 2, 3 });
        auto pv = inspectable.as<IPropertyValue>();

        com_array<int32_t> a;
        pv.GetInt32Array(a);

        REQUIRE(3 == a.size());
        REQUIRE(a[0] == 1);
        REQUIRE(a[1] == 2);
        REQUIRE(a[2] == 3);
    }

    SECTION("produce,array,int32_t")
    {
        auto pv = make<produce_IPropertyValue>();

        com_array<int32_t> a;
        pv.GetInt32Array(a);

        REQUIRE(3 == a.size());
        REQUIRE(a[0] == 1);
        REQUIRE(a[1] == 2);
        REQUIRE(a[2] == 3);
    }

    SECTION("consume,array,Size")
    {
        auto inspectable = PropertyValue::CreateSizeArray({ { 1,1 },{ 2,2 },{ 3,3 } });
        auto pv = inspectable.as<IPropertyValue>();

        com_array<Size> a;
        pv.GetSizeArray(a);

        REQUIRE(3 == a.size());
        REQUIRE(a[0].Width == 1); REQUIRE(a[0].Height == 1);
        REQUIRE(a[1].Width == 2); REQUIRE(a[1].Height == 2);
        REQUIRE(a[2].Width == 3); REQUIRE(a[2].Height == 3);
    }

    SECTION("produce,array,Size")
    {
        auto pv = make<produce_IPropertyValue>();

        com_array<Size> a;
        pv.GetSizeArray(a);

        REQUIRE(3 == a.size());
        REQUIRE(a[0].Width == 1); REQUIRE(a[0].Height == 1);
        REQUIRE(a[1].Width == 2); REQUIRE(a[1].Height == 2);
        REQUIRE(a[2].Width == 3); REQUIRE(a[2].Height == 3);
    }

    SECTION("consume,array,Inspectable")
    {
        auto inspectable = PropertyValue::CreateInspectableArray({ Uri(L"http://one/"), Uri(L"http://two/"), Uri(L"http://three/") });
        auto pv = inspectable.as<IPropertyValue>();

        com_array<Windows::Foundation::IInspectable> a;
        pv.GetInspectableArray(a);

        REQUIRE(3 == a.size());
        REQUIRE(a[0].as<Uri>().Host() == L"one");
        REQUIRE(a[1].as<Uri>().Host() == L"two");
        REQUIRE(a[2].as<Uri>().Host() == L"three");
    }

    SECTION("produce,array,Inspectable")
    {
        auto pv = make<produce_IPropertyValue>();

        com_array<Windows::Foundation::IInspectable> a;
        pv.GetInspectableArray(a);

        REQUIRE(3 == a.size());
        REQUIRE(a[0].as<Uri>().Host() == L"one");
        REQUIRE(a[1].as<Uri>().Host() == L"two");
        REQUIRE(a[2].as<Uri>().Host() == L"three");
    }
}

//
// Testing comparisons of spans is tricky because we need to ensure that the array storage remains alive for the duration
// of the test. Previously this was done with an initializer_list but the list went out of scope before the comparison was performed
// leading to failures in some builds.
//

struct compare_results
{
    bool equal;
    bool not_equal;
    bool greater;
    bool less;
    bool greater_equal;
    bool less_equal;
};

static compare_results compare_com_array(com_array<char> const & left, com_array<char> const & right)
{
    return
    {
        left == right,
        left != right,
        left > right,
        left < right,
        left >= right,
        left <= right
    };
}

TEST_CASE("com_array,compare,com_array")
{
    compare_results result{};

    result = compare_com_array({ 'a', 'b', 'c' }, { 'a', 'b', 'c' });
    REQUIRE(result.equal);
    REQUIRE(!result.not_equal);
    REQUIRE(!result.greater);
    REQUIRE(!result.less);
    REQUIRE(result.greater_equal);
    REQUIRE(result.less_equal);

    result = compare_com_array({ 'a', 'b', 'c' }, { 'a', 'b', 'c', 'd' });
    REQUIRE(!result.equal);
    REQUIRE(result.not_equal);
    REQUIRE(!result.greater);
    REQUIRE(result.less);
    REQUIRE(!result.greater_equal);
    REQUIRE(result.less_equal);

    result = compare_com_array({ 'a', 'b', 'c', 'd' }, { 'a', 'b', 'c' });
    REQUIRE(!result.equal);
    REQUIRE(result.not_equal);
    REQUIRE(result.greater);
    REQUIRE(!result.less);
    REQUIRE(result.greater_equal);
    REQUIRE(!result.less_equal);
}

// Verify various ways of constructing a com_array.
TEST_CASE("com_array,construct")
{
    com_array<int> two_zeroes{ { 0, 0 } };

    REQUIRE(com_array<int>(2) == com_array<int>({ 0, 0 }));

    // Verify these are treated as { size, initial_value } constructors
    // instead of { first, last } constructors.
    REQUIRE(com_array<int>(2, 5) == com_array<int>({ 5, 5 }));
    REQUIRE(com_array<unsigned int>(2, 5) == com_array<unsigned int>({ 5, 5 }));
}

// Verify that class template argument deduction works for com_array.
TEST_CASE("com_array,ctad")
{
#define REQUIRE_DEDUCED_AS(T, ...) \
    static_assert(std::is_same_v<com_array<T>, decltype(com_array(__VA_ARGS__))>)

    REQUIRE_DEDUCED_AS(uint8_t, 3, uint8_t(5));

    // Note that this looks like both an array and an initializer_list.
    REQUIRE_DEDUCED_AS(uint8_t, { uint8_t(5) });

    REQUIRE_DEDUCED_AS(uint8_t, com_array<uint8_t>());

    uint8_t a[3]{};
    REQUIRE_DEDUCED_AS(uint8_t, &a[0], &a[0]);
    REQUIRE_DEDUCED_AS(uint8_t, a);

    std::array<uint8_t, 3> ar{};
    REQUIRE_DEDUCED_AS(uint8_t, ar);

    std::vector<uint8_t> v{};
    REQUIRE_DEDUCED_AS(uint8_t, v);

    uint8_t const ca[3]{};
    REQUIRE_DEDUCED_AS(uint8_t, &ca[0], &ca[0]);
    REQUIRE_DEDUCED_AS(uint8_t, ca);

    std::array<uint8_t const, 3> arc{};
    REQUIRE_DEDUCED_AS(uint8_t, arc);

#undef REQUIRE_DEDUCED_AS
}
