#include "pch.h"
#include <atomic>
#include <tuple>
#include <type_traits>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.Collections.h>

// This type stores a set of interfaces that can be QI'd from a type, using lockless QI
// caching. This could greatly reduce the amount of QI operations on certain heavily
// derived types, like XAML or PropertyValue, etc. Template parameters are the C++/WinRT
// interface types to be stored.
//
// Internally, the type stores the ABI pointers in a std::atomic<> jacket and uses "racy
// query interface" when needed.
template <typename... TIFaces>
struct multibase_storage
{
    using TRootIFace = std::tuple_element_t<0, std::tuple<TIFaces...>>;
    using TRootIFaceABI = winrt::impl::abi_t<TRootIFace>;

    ~multibase_storage()
    {
        auto maybe_release = [](auto& iface) {
            if (auto ptr = iface.exchange(nullptr))
            {
                ptr->Release();
            }
            };

        std::apply(
            [&](auto&... tupleArgs) { (maybe_release(tupleArgs), ...); }, ifaces);
    }

    explicit multibase_storage(TRootIFaceABI* base) noexcept
    {
        base->AddRef();
        std::get<0>(ifaces) = base;
    }

    explicit multibase_storage(TRootIFaceABI* base, winrt::take_ownership_from_abi_t) noexcept
    {
        std::get<0>(ifaces) = base;
    }

    // Initialize from an existing C++/WinRT interface
    explicit multibase_storage(TRootIFace q) : multibase_storage(static_cast<TRootIFaceABI*>(winrt::impl::detach_from(std::move(q))), winrt::take_ownership_from_abi) {}

    template <typename TIFace>
    TIFace const& get() requires (std::is_same_v<TIFace, TIFaces> || ...)
    {
        using TIFaceABI = winrt::impl::abi_t<TIFace>;
        using TIFaceAtomic = std::atomic<TIFaceABI*>;

        if (auto existing = std::get<TIFaceAtomic>(ifaces).load())
        {
            return *reinterpret_cast<TIFace*>(existing);
        }
        else
        {
            return *load_iface<TIFace>();
        }
    }

    template<typename TIFace> TIFace get() requires (!std::is_same_v<TIFace, TIFaces> && ...)
    {
        TIFace i{ nullptr };
        winrt::copy_from_abi(i, std::get<0>(ifaces).load());
        return i;
    }

    template<typename TIFace, typename Q> auto call(Q&& q)
    {
        auto const& self = get<TIFace>();
        return std::forward<Q>(q)(self);
    }

private:
    std::tuple<std::atomic<winrt::impl::abi_t<TIFaces>*>...> ifaces;

    template<typename TIFace> TIFace* load_iface()
    {
        using TIFaceABI = winrt::impl::abi_t<TIFace>;
        using TIFaceAtomic = std::atomic<TIFaceABI*>;

        auto base = std::get<0>(ifaces).load();
        TIFaceABI* newInterface{ nullptr };
        winrt::check_hresult(base->QueryInterface(winrt::guid_of<TIFace>(), reinterpret_cast<void**>(&newInterface)));

        TIFaceABI* emptyValue{ nullptr };
        auto& storage = std::get<std::atomic<TIFaceABI*>>(ifaces);
        if (storage.compare_exchange_weak(emptyValue, newInterface))
        {
            return reinterpret_cast<TIFace*>(newInterface);
        }
        else
        {
            newInterface->Release();
            return reinterpret_cast<TIFace*>(storage.load());
        }
    }
};

using propset_muple = multibase_storage<
    winrt::Windows::Foundation::Collections::IPropertySet,
    winrt::Windows::Foundation::Collections::IObservableMap<winrt::hstring, winrt::Windows::Foundation::IInspectable>,
    winrt::Windows::Foundation::Collections::IMap<winrt::hstring, winrt::Windows::Foundation::IInspectable>,
    winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::Collections::IKeyValuePair<winrt::hstring, winrt::Windows::Foundation::IInspectable>>>;

void muple()
{
    winrt::Windows::Foundation::Collections::PropertySet ps;
    propset_muple mup(ps);
    mup.get<winrt::Windows::Foundation::Collections::IPropertySet>();
    mup.get<winrt::Windows::Foundation::IStringable>().ToString();   
}
