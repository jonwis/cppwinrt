
#ifdef __cpp_consteval
#define WINRT_IMPL_CONSTEVAL consteval
#else
#define WINRT_IMPL_CONSTEVAL constexpr
#endif

WINRT_EXPORT namespace winrt::param
{
    struct hstring
    {
#ifdef _MSC_VER
#pragma warning(suppress: 26495)
#endif
        WINRT_IMPL_CONSTEVAL hstring() noexcept : m_handle(nullptr) {}
        hstring(hstring const& values) = delete;
        hstring& operator=(hstring const& values) = delete;
        hstring(std::nullptr_t) = delete;

#ifdef _MSC_VER
#pragma warning(suppress: 26495)
#endif
        hstring(winrt::hstring const& value) noexcept : m_handle(get_abi(value))
        {
        }

        constexpr hstring(std::wstring_view value) noexcept
        {
            if (value.size() > 0)
            {
                WINRT_ASSERT(value.size() < UINT_MAX);
                m_handle = &m_header;
                m_header.flags = impl::hstring_reference_flag;
                m_header.length = static_cast<uint32_t>(value.size());
                m_header.ptr = value.data();
            }
        }

        constexpr hstring(std::wstring const& value) noexcept : hstring(std::wstring_view(value))
        {
        }

        constexpr hstring(wchar_t const* const value) noexcept : hstring(std::wstring_view(value))
        {
        }

        template<uint32_t size> WINRT_IMPL_CONSTEVAL hstring(const wchar_t (&value)[size]) noexcept
        {
            static_assert(size < UINT_MAX, "string too large");
            if (size == 1)
            {
                m_handle = nullptr;
            }
            else
            {
                m_handle = &m_header;
                m_header.flags = impl::hstring_reference_flag;
                m_header.length = size - 1;
                m_header.ptr = value;
            }
        }

        operator winrt::hstring const&() const noexcept
        {
            return *reinterpret_cast<winrt::hstring const*>(this);
        }

    private:
        void* m_handle = nullptr;
        impl::hstring_header m_header{};
    };

    inline void* get_abi(hstring const& object) noexcept
    {
        return *(void**)(&object);
    }
}

namespace winrt::impl
{
    template <typename T>
    using param_type = std::conditional_t<std::is_same_v<T, hstring>, param::hstring, T>;
}
