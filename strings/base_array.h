#include <span>

WINRT_EXPORT namespace winrt
{
    template <typename T>
    struct com_array : std::span<T>
    {
        using span_t = std::span<T>;
        using typename span_t::value_type;
        using typename span_t::size_type;
        using typename span_t::reference;
        using typename span_t::const_reference;
        using typename span_t::pointer;
        using typename span_t::const_pointer;
        using typename span_t::iterator;
        using typename span_t::reverse_iterator;

        com_array(com_array const&) = delete;
        com_array& operator=(com_array const&) = delete;

        com_array() noexcept = default;

        explicit com_array(size_type const count) :
            com_array(count, value_type())
        {}

        com_array(void* ptr, uint32_t const count, take_ownership_from_abi_t) noexcept :
            span_t(static_cast<value_type*>(ptr), static_cast<value_type*>(ptr) + count)
        {
        }

        com_array(size_type const count, value_type const& value)
        {
            alloc(count);
            std::uninitialized_fill_n(this->data(), count, value);
        }

        template <typename InIt, typename = std::void_t<typename std::iterator_traits<InIt>::difference_type>>
        com_array(InIt first, InIt last)
        {
            alloc(static_cast<size_type>(std::distance(first, last)));
            std::uninitialized_copy(first, last, this->begin());
        }

        template <typename U>
        explicit com_array(std::vector<U> const& value) :
            com_array(value.begin(), value.end())
        {}

        template <typename U, size_t N>
        explicit com_array(std::array<U, N> const& value) :
            com_array(value.begin(), value.end())
        {}

        template <typename U, size_t N>
        explicit com_array(U const(&value)[N]) :
            com_array(value, value + N)
        {}

        com_array(std::initializer_list<value_type> value) :
            com_array(value.begin(), value.end())
        {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        com_array(std::initializer_list<U> value) :
            com_array(value.begin(), value.end())
        {}

        com_array(com_array&& other)
        {
            as_span() = other;
            other.as_span() = {};
        }

        com_array& operator=(com_array&& other) noexcept
        {
            clear();
            as_span() = std::move(other);
            other.as_span() = {};
            return *this;
        }

        ~com_array() noexcept
        {
            clear();
        }

        void clear() noexcept
        {
            std::destroy(this->begin(), this->end());
            WINRT_IMPL_CoTaskMemFree(this->data());
            static_cast<span_t&>(*this) = {};
        }

        uint32_t size() const
        {
            return static_cast<uint32_t>(as_span().size());
        }

        friend void swap(com_array& left, com_array& right) noexcept
        {
            std::swap(left.as_span(), right.as_span());
        }

        void set_size(uint32_t size)
        {
            as_span() = {this->data(), size};
        }

    private:

        span_t& as_span() { return *this; };
        span_t const& as_span() const { return *this; }

        void alloc(size_type const size)
        {
            WINRT_ASSERT(this->empty());

            if (0 != size)
            {
                auto data = static_cast<value_type*>(WINRT_IMPL_CoTaskMemAlloc(size * sizeof(value_type)));
                if (!data)
                {
                    throw std::bad_alloc();
                }

                as_span() = { data, size };
            }
        }

        std::pair<uint32_t, impl::arg_out<T>> detach_abi() noexcept
        {
#ifdef _MSC_VER
            // https://github.com/microsoft/cppwinrt/pull/1165
            std::pair<uint32_t, impl::arg_out<T>> result;
            memset(&result, 0, sizeof(result));
            result.first = this->size();
            result.second = *reinterpret_cast<impl::arg_out<T>*>(this->data());
#else
            std::pair<uint32_t, impl::arg_out<T>> result(this->size(), *reinterpret_cast<impl::arg_out<T>*>(this->data()));
#endif
            as_span() = {};
            return result;
        }

        template <typename U>
        friend std::pair<uint32_t, impl::arg_out<U>> detach_abi(com_array<U>& object) noexcept;
    };

    template <typename C> com_array(uint32_t, C const&) -> com_array<std::decay_t<C>>;
    template <typename InIt, typename = std::void_t<typename std::iterator_traits<InIt>::difference_type>>
    com_array(InIt, InIt) -> com_array<std::decay_t<typename std::iterator_traits<InIt>::value_type>>;
    template <typename C> com_array(std::vector<C> const&) -> com_array<std::decay_t<C>>;
    template <size_t N, typename C> com_array(std::array<C, N> const&) -> com_array<std::decay_t<C>>;
    template <size_t N, typename C> com_array(C const(&)[N]) -> com_array<std::decay_t<C>>;
    template <typename C> com_array(std::initializer_list<C>) -> com_array<std::decay_t<C>>;

    namespace impl
    {
        template <typename T, typename U>
        inline constexpr bool array_comparable = std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>>;
    }

    template <typename T, typename U, 
        std::enable_if_t<impl::array_comparable<T, U>, int> = 0>
    bool operator==(com_array<T> const& left, com_array<U> const& right) noexcept
    {
        return std::equal(left.begin(), left.end(), right.begin(), right.end());
    }

    template <typename T, typename U,
        std::enable_if_t<impl::array_comparable<T, U>, int> = 0>
    bool operator<(com_array<T> const& left, com_array<U> const& right) noexcept
    {
        return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
    }

    template <typename T, typename U, std::enable_if_t<impl::array_comparable<T, U>, int> = 0>
    bool operator!=(com_array<T> const& left, com_array<U> const& right) noexcept { return !(left == right); }
    template <typename T, typename U,std::enable_if_t<impl::array_comparable<T, U>, int> = 0>
    bool operator>(com_array<T> const& left, com_array<U> const& right) noexcept { return right < left; }
    template <typename T, typename U,std::enable_if_t<impl::array_comparable<T, U>, int> = 0>
    bool operator<=(com_array<T> const& left, com_array<U> const& right) noexcept { return !(right < left); }
    template <typename T, typename U, std::enable_if_t<impl::array_comparable<T, U>, int> = 0>
    bool operator>=(com_array<T> const& left, com_array<U> const& right) noexcept { return !(left < right); }

    template <typename T>
    auto get_abi(std::span<T> object) noexcept
    {
        auto data = object.size() ? object.data() : (T*)alignof(T);

        if constexpr (std::is_base_of_v<Windows::Foundation::IUnknown, T>)
        {
            return (void**)data;
        }
        else
        {
            return reinterpret_cast<impl::arg_out<std::remove_const_t<T>>>(const_cast<std::remove_const_t<T>*>(data));
        }
    }

    template <typename T>
    auto put_abi(std::span<T> object) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            std::fill(object.begin(), object.end(), impl::empty_value<T>());
        }

        return get_abi(object);
    }

    template<typename T>
    uint32_t get_abi_size(std::span<T> object) noexcept
    {
        auto size = object.size();
        WINRT_ASSERT(size < UINT_MAX);
        return static_cast<uint32_t>(size);
    }

    template<typename T>
    auto put_abi(com_array<T>& object) noexcept
    {
        object.clear();
        return reinterpret_cast<impl::arg_out<T>*>(&object);
    }

    template <typename T>
    std::pair<uint32_t, impl::arg_out<T>> detach_abi(com_array<T>& object) noexcept
    {
        return object.detach_abi();
    }

    template <typename T>
    auto detach_abi(com_array<T>&& object) noexcept
    {
        return detach_abi(object);
    }
}

namespace winrt::impl
{
    template <typename T>
    struct array_size_proxy
    {
        array_size_proxy& operator=(array_size_proxy const&) = delete;

        array_size_proxy(com_array<T>& value) noexcept : m_value(value)
        {}

        ~array_size_proxy() noexcept
        {
            WINRT_ASSERT(m_value.data() || (!m_value.data() && m_size == 0));
            m_value.set_size(m_size);
        }

        operator uint32_t*() noexcept
        {
            return &m_size;
        }

        operator unsigned long*() noexcept
        {
            return reinterpret_cast<unsigned long*>(&m_size);
        }

    private:

        com_array<T>& m_value;
        uint32_t m_size{ 0 };
    };

    template<typename T>
    array_size_proxy<T> put_size_abi(com_array<T>& object) noexcept
    {
        return array_size_proxy<T>(object);
    }

    template <typename T>
    struct com_array_proxy
    {
        com_array_proxy(uint32_t* size, winrt::impl::arg_out<T>* value) noexcept : m_size(size), m_value(value)
        {}

        ~com_array_proxy() noexcept
        {
            std::tie(*m_size, *m_value) = detach_abi(m_temp);
        }

        operator com_array<T>&() noexcept
        {
            return m_temp;
        }

        com_array_proxy(com_array_proxy const&) noexcept
        {
            // A Visual C++ compiler bug (550631) requires the copy constructor even though it is never called.
            WINRT_ASSERT(false);
        }

    private:

        uint32_t* m_size;
        arg_out<T>* m_value;
        com_array<T> m_temp;
    };
}

WINRT_EXPORT namespace winrt
{
    template <typename T>
    auto detach_abi(uint32_t* __valueSize, impl::arg_out<T>* value) noexcept
    {
        return impl::com_array_proxy<T>(__valueSize, value);
    }

    inline hstring get_class_name(Windows::Foundation::IInspectable const& object)
    {
        void* value{};
        check_hresult((*(impl::inspectable_abi**)&object)->GetRuntimeClassName(&value));
        return { value, take_ownership_from_abi };
    }

    inline com_array<guid> get_interfaces(Windows::Foundation::IInspectable const& object)
    {
        com_array<guid> value;
        check_hresult((*(impl::inspectable_abi**)&object)->GetIids(impl::put_size_abi(value), put_abi(value)));
        return value;
    }

    inline Windows::Foundation::TrustLevel get_trust_level(Windows::Foundation::IInspectable const& object)
    {
        Windows::Foundation::TrustLevel value{};
        check_hresult((*(impl::inspectable_abi**)&object)->GetTrustLevel(&value));
        return value;
    }
}
