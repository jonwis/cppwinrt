
WINRT_EXPORT namespace winrt::impl
{
    template <typename T>
    struct reference;

    // The scalar types that combase PropertyValue can carry by value. box_value on one of these
    // produces an in-process reference<T> for the fast path, but must still marshal by value across
    // apartments/processes so the destination sees a real PropertyValue copy rather than a proxy.
    template <typename T>
    inline constexpr bool is_stock_reference_v =
        std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::int16_t> ||
        std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::int32_t> ||
        std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::int64_t> ||
        std::is_same_v<T, std::uint64_t> || std::is_same_v<T, float> ||
        std::is_same_v<T, double> || std::is_same_v<T, char16_t> ||
        std::is_same_v<T, bool> || std::is_same_v<T, hstring> ||
        std::is_same_v<T, guid> || std::is_same_v<T, Windows::Foundation::DateTime> ||
        std::is_same_v<T, Windows::Foundation::TimeSpan> || std::is_same_v<T, Windows::Foundation::Point>;

    // Stock scalar references are marked non_agile so that our query_interface_tearoff supplies
    // IMarshal (delegating to combase PropertyValue for by-value marshaling) instead of the default
    // free-threaded-marshaler that marshals by reference. All other T take an inert marker placeholder,
    // which implements<> ignores, keeping the default agile shape.
    template <typename T>
    using reference_base_t = implements<reference<T>,
        Windows::Foundation::IReference<T>, Windows::Foundation::IPropertyValue,
        std::conditional_t<is_stock_reference_v<T>, non_agile, marker>>;

    template <typename T>
    struct reference : reference_base_t<T>
    {
        reference(T const& value) : m_value(value)
        {
        }

        T Value() const
        {
            return m_value;
        }

        Windows::Foundation::PropertyType Type() const noexcept
        {
            using pt = Windows::Foundation::PropertyType;

            if constexpr (std::is_same_v<T, std::uint8_t>) { return pt::UInt8; }
            else if constexpr (std::is_same_v<T, std::int16_t>) { return pt::Int16; }
            else if constexpr (std::is_same_v<T, std::uint16_t>) { return pt::UInt16; }
            else if constexpr (std::is_same_v<T, std::int32_t>) { return pt::Int32; }
            else if constexpr (std::is_same_v<T, std::uint32_t>) { return pt::UInt32; }
            else if constexpr (std::is_same_v<T, std::int64_t>) { return pt::Int64; }
            else if constexpr (std::is_same_v<T, std::uint64_t>) { return pt::UInt64; }
            else if constexpr (std::is_same_v<T, float>) { return pt::Single; }
            else if constexpr (std::is_same_v<T, double>) { return pt::Double; }
            else if constexpr (std::is_same_v<T, char16_t>) { return pt::Char16; }
            else if constexpr (std::is_same_v<T, bool>) { return pt::Boolean; }
            else if constexpr (std::is_same_v<T, hstring>) { return pt::String; }
            else if constexpr (std::is_same_v<T, guid>) { return pt::Guid; }
            else if constexpr (std::is_same_v<T, Windows::Foundation::DateTime>) { return pt::DateTime; }
            else if constexpr (std::is_same_v<T, Windows::Foundation::TimeSpan>) { return pt::TimeSpan; }
            else if constexpr (std::is_same_v<T, Windows::Foundation::Point>) { return pt::Point; }
            else { return pt::OtherType; }
        }

        static constexpr bool IsNumericScalar() noexcept
        {
            return (std::is_arithmetic_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char16_t>) || std::is_enum_v<T>;
        }

        std::uint8_t GetUInt8() const
        {
            return to_scalar<std::uint8_t>();
        }

        std::int16_t GetInt16() const
        {
            return to_scalar<std::int16_t>();
        }

        std::uint16_t GetUInt16() const
        {
            return to_scalar<std::uint16_t>();
        }

        std::int32_t GetInt32() const
        {
            return to_scalar<std::int32_t>();
        }

        std::uint32_t GetUInt32() const
        {
            return to_scalar<std::uint32_t>();
        }

        std::int64_t GetInt64() const
        {
            return to_scalar<std::int64_t>();
        }

        std::uint64_t GetUInt64() const
        {
            return to_scalar<std::uint64_t>();
        }

        float GetSingle() { return to_scalar<float>(); }
        double GetDouble() { return to_scalar<double>(); }
        char16_t GetChar16() { if constexpr (std::is_same_v<T, char16_t>) { return m_value; } else { throw hresult_not_implemented(); } }
        bool GetBoolean() { if constexpr (std::is_same_v<T, bool>) { return m_value; } else { throw hresult_not_implemented(); } }
        hstring GetString() { if constexpr (std::is_same_v<T, hstring>) { return m_value; } else { throw hresult_not_implemented(); } }
        guid GetGuid() { if constexpr (std::is_same_v<T, guid>) { return m_value; } else { throw hresult_not_implemented(); } }
        Windows::Foundation::DateTime GetDateTime() { if constexpr (std::is_same_v<T, Windows::Foundation::DateTime>) { return m_value; } else { throw hresult_not_implemented(); } }
        Windows::Foundation::TimeSpan GetTimeSpan() { if constexpr (std::is_same_v<T, Windows::Foundation::TimeSpan>) { return m_value; } else { throw hresult_not_implemented(); } }
        Windows::Foundation::Point GetPoint() { if constexpr (std::is_same_v<T, Windows::Foundation::Point>) { return m_value; } else { throw hresult_not_implemented(); } }
        Windows::Foundation::Size GetSize() { throw hresult_not_implemented(); }
        Windows::Foundation::Rect GetRect() { throw hresult_not_implemented(); }
        void GetUInt8Array(com_array<std::uint8_t> &) { throw hresult_not_implemented(); }
        void GetInt16Array(com_array<std::int16_t> &) { throw hresult_not_implemented(); }
        void GetUInt16Array(com_array<std::uint16_t> &) { throw hresult_not_implemented(); }
        void GetInt32Array(com_array<std::int32_t> &) { throw hresult_not_implemented(); }
        void GetUInt32Array(com_array<std::uint32_t> &) { throw hresult_not_implemented(); }
        void GetInt64Array(com_array<std::int64_t> &) { throw hresult_not_implemented(); }
        void GetUInt64Array(com_array<std::uint64_t> &) { throw hresult_not_implemented(); }
        void GetSingleArray(com_array<float> &) { throw hresult_not_implemented(); }
        void GetDoubleArray(com_array<double> &) { throw hresult_not_implemented(); }
        void GetChar16Array(com_array<char16_t> &) { throw hresult_not_implemented(); }
        void GetBooleanArray(com_array<bool> &) { throw hresult_not_implemented(); }
        void GetStringArray(com_array<hstring> &) { throw hresult_not_implemented(); }
        void GetInspectableArray(com_array<Windows::Foundation::IInspectable> &) { throw hresult_not_implemented(); }
        void GetGuidArray(com_array<guid> &) { throw hresult_not_implemented(); }
        void GetDateTimeArray(com_array<Windows::Foundation::DateTime> &) { throw hresult_not_implemented(); }
        void GetTimeSpanArray(com_array<Windows::Foundation::TimeSpan> &) { throw hresult_not_implemented(); }
        void GetPointArray(com_array<Windows::Foundation::Point> &) { throw hresult_not_implemented(); }
        void GetSizeArray(com_array<Windows::Foundation::Size> &) { throw hresult_not_implemented(); }
        void GetRectArray(com_array<Windows::Foundation::Rect> &) { throw hresult_not_implemented(); }

    private:

        // For stock scalar T, hand out an IMarshal that marshals by value: build the equivalent
        // combase PropertyValue on demand and delegate marshaling to it. This is lazy - box_value and
        // unbox_value never touch combase; the hop only happens if the reference is actually marshaled.
        std::int32_t query_interface_tearoff(guid const& id, void** object) const noexcept override
        {
            if constexpr (is_stock_reference_v<T>)
            {
                if (is_guid_of<IMarshal>(id))
                {
                    try
                    {
                        auto marshal = create_property_value().template as<IMarshal>();
                        *object = detach_abi(marshal);
                        return error_ok;
                    }
                    catch (...)
                    {
                        *object = nullptr;
                        return to_hresult();
                    }
                }

                // reference<T> is immutable, so it is safe to call from any apartment. Advertise
                // IAgileObject (as combase PropertyValue does) so callers keep the agile fast path;
                // cross-apartment/process marshaling still routes through the by-value IMarshal above.
                if (is_guid_of<IAgileObject>(id))
                {
                    auto unknown = reinterpret_cast<unknown_abi*>(to_abi<Windows::Foundation::IReference<T>>(this));
                    unknown->AddRef();
                    *object = unknown;
                    return error_ok;
                }
            }

            *object = nullptr;
            return error_no_interface;
        }

        Windows::Foundation::IInspectable create_property_value() const
        {
            using pv = Windows::Foundation::PropertyValue;

            if constexpr (std::is_same_v<T, std::uint8_t>) { return pv::CreateUInt8(m_value); }
            else if constexpr (std::is_same_v<T, std::int16_t>) { return pv::CreateInt16(m_value); }
            else if constexpr (std::is_same_v<T, std::uint16_t>) { return pv::CreateUInt16(m_value); }
            else if constexpr (std::is_same_v<T, std::int32_t>) { return pv::CreateInt32(m_value); }
            else if constexpr (std::is_same_v<T, std::uint32_t>) { return pv::CreateUInt32(m_value); }
            else if constexpr (std::is_same_v<T, std::int64_t>) { return pv::CreateInt64(m_value); }
            else if constexpr (std::is_same_v<T, std::uint64_t>) { return pv::CreateUInt64(m_value); }
            else if constexpr (std::is_same_v<T, float>) { return pv::CreateSingle(m_value); }
            else if constexpr (std::is_same_v<T, double>) { return pv::CreateDouble(m_value); }
            else if constexpr (std::is_same_v<T, char16_t>) { return pv::CreateChar16(m_value); }
            else if constexpr (std::is_same_v<T, bool>) { return pv::CreateBoolean(m_value); }
            else if constexpr (std::is_same_v<T, hstring>) { return pv::CreateString(m_value); }
            else if constexpr (std::is_same_v<T, guid>) { return pv::CreateGuid(m_value); }
            else if constexpr (std::is_same_v<T, Windows::Foundation::DateTime>) { return pv::CreateDateTime(m_value); }
            else if constexpr (std::is_same_v<T, Windows::Foundation::TimeSpan>) { return pv::CreateTimeSpan(m_value); }
            else if constexpr (std::is_same_v<T, Windows::Foundation::Point>) { return pv::CreatePoint(m_value); }
            else { return nullptr; }
        }

        template <typename To>
        To to_scalar() const
        {
            if constexpr (IsNumericScalar())
            {
                return static_cast<To>(m_value);
            }
            else
            {
                throw hresult_not_implemented();
            }
        }

        T m_value;
    };

    template <typename T>
    struct reference_traits
    {
        static auto make(T const& value) { return winrt::make<impl::reference<T>>(value); }
        using itf = Windows::Foundation::IReference<T>;
    };

    template <>
    struct reference_traits<Windows::Foundation::IInspectable>
    {
        static auto make(Windows::Foundation::IInspectable const& value) { return Windows::Foundation::PropertyValue::CreateInspectable(value); }
        using itf = Windows::Foundation::IInspectable;
    };

    template <>
    struct reference_traits<GUID>
    {
        static auto make(GUID const& value) { return reference_traits<guid>::make(reinterpret_cast<guid const&>(value)); }
        using itf = Windows::Foundation::IReference<guid>;
    };

    template <>
    struct reference_traits<Windows::Foundation::Size>
    {
        static auto make(Windows::Foundation::Size const& value) { return Windows::Foundation::PropertyValue::CreateSize(value); }
        using itf = Windows::Foundation::IReference<Windows::Foundation::Size>;
    };

    template <>
    struct reference_traits<Windows::Foundation::Rect>
    {
        static auto make(Windows::Foundation::Rect const& value) { return Windows::Foundation::PropertyValue::CreateRect(value); }
        using itf = Windows::Foundation::IReference<Windows::Foundation::Rect>;
    };

    template <>
    struct reference_traits<com_array<std::uint8_t>>
    {
        static auto make(array_view<std::uint8_t const> const& value) { return Windows::Foundation::PropertyValue::CreateUInt8Array(value); }
        using itf = Windows::Foundation::IReferenceArray<std::uint8_t>;
    };

    template <>
    struct reference_traits<com_array<std::int16_t>>
    {
        static auto make(array_view<std::int16_t const> const& value) { return Windows::Foundation::PropertyValue::CreateInt16Array(value); }
        using itf = Windows::Foundation::IReferenceArray<std::int16_t>;
    };

    template <>
    struct reference_traits<com_array<std::uint16_t>>
    {
        static auto make(array_view<std::uint16_t const> const& value) { return Windows::Foundation::PropertyValue::CreateUInt16Array(value); }
        using itf = Windows::Foundation::IReferenceArray<std::uint16_t>;
    };

    template <>
    struct reference_traits<com_array<std::int32_t>>
    {
        static auto make(array_view<std::int32_t const> const& value) { return Windows::Foundation::PropertyValue::CreateInt32Array(value); }
        using itf = Windows::Foundation::IReferenceArray<std::int32_t>;
    };

    template <>
    struct reference_traits<com_array<std::uint32_t>>
    {
        static auto make(com_array<std::uint32_t> const& value) { return Windows::Foundation::PropertyValue::CreateUInt32Array(value); }
        using itf = Windows::Foundation::IReferenceArray<std::uint32_t>;
    };

    template <>
    struct reference_traits<com_array<std::int64_t>>
    {
        static auto make(array_view<std::int64_t const> const& value) { return Windows::Foundation::PropertyValue::CreateInt64Array(value); }
        using itf = Windows::Foundation::IReferenceArray<std::int64_t>;
    };

    template <>
    struct reference_traits<com_array<std::uint64_t>>
    {
        static auto make(array_view<std::uint64_t const> const& value) { return Windows::Foundation::PropertyValue::CreateUInt64Array(value); }
        using itf = Windows::Foundation::IReferenceArray<std::uint64_t>;
    };

    template <>
    struct reference_traits<com_array<float>>
    {
        static auto make(array_view<float const> const& value) { return Windows::Foundation::PropertyValue::CreateSingleArray(value); }
        using itf = Windows::Foundation::IReferenceArray<float>;
    };

    template <>
    struct reference_traits<com_array<double>>
    {
        static auto make(array_view<double const> const& value) { return Windows::Foundation::PropertyValue::CreateDoubleArray(value); }
        using itf = Windows::Foundation::IReferenceArray<double>;
    };

    template <>
    struct reference_traits<com_array<char16_t>>
    {
        static auto make(array_view<char16_t const> const& value) { return Windows::Foundation::PropertyValue::CreateChar16Array(value); }
        using itf = Windows::Foundation::IReferenceArray<char16_t>;
    };

    template <>
    struct reference_traits<com_array<bool>>
    {
        static auto make(array_view<bool const> const& value) { return Windows::Foundation::PropertyValue::CreateBooleanArray(value); }
        using itf = Windows::Foundation::IReferenceArray<bool>;
    };

    template <>
    struct reference_traits<com_array<hstring>>
    {
        static auto make(array_view<hstring const> const& value) { return Windows::Foundation::PropertyValue::CreateStringArray(value); }
        using itf = Windows::Foundation::IReferenceArray<hstring>;
    };

    template <>
    struct reference_traits<com_array<Windows::Foundation::IInspectable>>
    {
        static auto make(array_view<Windows::Foundation::IInspectable const> const& value) { return Windows::Foundation::PropertyValue::CreateInspectableArray(value); }
        using itf = Windows::Foundation::IReferenceArray<Windows::Foundation::IInspectable>;
    };

    template <>
    struct reference_traits<com_array<guid>>
    {
        static auto make(array_view<guid const> const& value) { return Windows::Foundation::PropertyValue::CreateGuidArray(value); }
        using itf = Windows::Foundation::IReferenceArray<guid>;
    };

    template <>
    struct reference_traits<com_array<GUID>>
    {
        static auto make(array_view<GUID const> const& value) { return Windows::Foundation::PropertyValue::CreateGuidArray(reinterpret_cast<array_view<guid const> const&>(value)); }
        using itf = Windows::Foundation::IReferenceArray<guid>;
    };

    template <>
    struct reference_traits<com_array<Windows::Foundation::DateTime>>
    {
        static auto make(array_view<Windows::Foundation::DateTime const> const& value) { return Windows::Foundation::PropertyValue::CreateDateTimeArray(value); }
        using itf = Windows::Foundation::IReferenceArray<Windows::Foundation::DateTime>;
    };

    template <>
    struct reference_traits<com_array<Windows::Foundation::TimeSpan>>
    {
        static auto make(array_view<Windows::Foundation::TimeSpan const> const& value) { return Windows::Foundation::PropertyValue::CreateTimeSpanArray(value); }
        using itf = Windows::Foundation::IReferenceArray<Windows::Foundation::TimeSpan>;
    };

    template <>
    struct reference_traits<com_array<Windows::Foundation::Point>>
    {
        static auto make(array_view<Windows::Foundation::Point const> const& value) { return Windows::Foundation::PropertyValue::CreatePointArray(value); }
        using itf = Windows::Foundation::IReferenceArray<Windows::Foundation::Point>;
    };

    template <>
    struct reference_traits<com_array<Windows::Foundation::Size>>
    {
        static auto make(array_view<Windows::Foundation::Size const> const& value) { return Windows::Foundation::PropertyValue::CreateSizeArray(value); }
        using itf = Windows::Foundation::IReferenceArray<Windows::Foundation::Size>;
    };

    template <>
    struct reference_traits<com_array<Windows::Foundation::Rect>>
    {
        static auto make(array_view<Windows::Foundation::Rect const> const& value) { return Windows::Foundation::PropertyValue::CreateRectArray(value); }
        using itf = Windows::Foundation::IReferenceArray<Windows::Foundation::Rect>;
    };
}

WINRT_EXPORT namespace winrt::Windows::Foundation
{
    template <typename T>
    bool operator==(IReference<T> const& left, IReference<T> const& right)
    {
        if (get_abi(left) == get_abi(right))
        {
            return true;
        }

        if (!left || !right)
        {
            return false;
        }

        return left.Value() == right.Value();
    }

    template <typename T>
    bool operator!=(IReference<T> const& left, IReference<T> const& right)
    {
        return !(left == right);
    }
}

WINRT_EXPORT namespace winrt::impl
{
    template <typename T, typename From>
    T unbox_value_type(From&& value)
    {
        if (!value)
        {
            throw hresult_no_interface();
        }
        if constexpr (std::is_enum_v<T>)
        {
            if (auto temp = value.template try_as<Windows::Foundation::IReference<T>>())
            {
                return temp.Value();
            }
            else
            {
                return static_cast<T>(value.template as<Windows::Foundation::IReference<std::underlying_type_t<T>>>().Value());
            }
        }
        else if constexpr (std::is_same_v<T, com_array<GUID>>)
        {
            T result;
            reinterpret_cast<com_array<guid>&>(result) = value.template as<typename impl::reference_traits<T>::itf>().Value();
            return result;
        }
        else
        {
            return value.template as<typename impl::reference_traits<T>::itf>().Value();
        }
    }

    template <typename T, typename Ret = T, typename From, typename U>
    Ret unbox_value_type_or(From&& value, U&& default_value)
    {
        if constexpr (std::is_enum_v<T>)
        {
            if (auto temp = value.template try_as<Windows::Foundation::IReference<T>>())
            {
                return temp.Value();
            }

            if (auto temp = value.template try_as<Windows::Foundation::IReference<std::underlying_type_t<T>>>())
            {
                return static_cast<T>(temp.Value());
            }
        }
        else if constexpr (std::is_same_v<T, com_array<GUID>>)
        {
            if (auto temp = value.template try_as<typename impl::reference_traits<T>::itf>())
            {
                T result;
                reinterpret_cast<com_array<guid>&>(result) = temp.Value();
                return result;
            }
        }
        else
        {
            if (auto temp = value.template try_as<typename impl::reference_traits<T>::itf>())
            {
                return temp.Value();
            }
        }
        return default_value;
    }

    template <typename To, typename From, std::enable_if_t<!is_com_interface_v<To>, int>>
    auto as(From* ptr)
    {
        if constexpr (impl::is_com_interface_v<From>)
        {
            return unbox_value_type<To>(reinterpret_cast<Windows::Foundation::IUnknown const&>(ptr));
        }
        else
        {
            return unbox_value_type<To>(reinterpret_cast<com_ptr<From> const&>(ptr));
        }
    }

    template <typename To, typename From, std::enable_if_t<!is_com_interface_v<To>, int>>
    auto try_as(From* ptr) noexcept
    {
        using type = std::conditional_t<impl::is_com_interface_v<From>, Windows::Foundation::IUnknown, com_ptr<From>>;
        return unbox_value_type_or<To, std::optional<To>>(reinterpret_cast<type const&>(ptr), std::nullopt);
    }
}

WINRT_EXPORT namespace winrt
{
    template <typename T, std::enable_if_t<std::is_constructible_v<hstring, T>, int> = 0>
    Windows::Foundation::IInspectable box_value(T&& value)
    {
        return Windows::Foundation::IReference<hstring>(hstring(std::forward<T>(value)));
    }

    template <typename T, std::enable_if_t<!std::is_constructible_v<hstring, T>, int> = 0>
    Windows::Foundation::IInspectable box_value(T const& value)
    {
        if constexpr (std::is_base_of_v<Windows::Foundation::IInspectable, T>)
        {
            return value;
        }
        else
        {
            return impl::reference_traits<T>::make(value);
        }
    }

    template <typename T>
    T unbox_value(Windows::Foundation::IInspectable const& value)
    {
        if constexpr (std::is_base_of_v<Windows::Foundation::IInspectable, T>)
        {
            return value.as<T>();
        }
        else
        {
            return impl::unbox_value_type<T>(value);
        }
    }

    template <typename T = hstring, std::enable_if_t<std::is_same_v<T, hstring>, int> = 0>
    hstring unbox_value_or(Windows::Foundation::IInspectable const& value, param::hstring const& default_value)
    {
        if (value)
        {
            if (auto temp = value.try_as<Windows::Foundation::IReference<hstring>>())
            {
                return temp.Value();
            }
        }

        return *(hstring*)(&default_value);
    }

    template <typename T, std::enable_if_t<!std::is_same_v<T, hstring>, int> = 0>
    T unbox_value_or(Windows::Foundation::IInspectable const& value, T const& default_value)
    {
        if (value)
        {
            if constexpr (std::is_base_of_v<Windows::Foundation::IInspectable, T>)
            {
                if (auto temp = value.try_as<T>())
                {
                    return temp;
                }
            }
            else
            {
                return impl::unbox_value_type_or<T>(value, default_value);
            }
        }
        return default_value;
    }

    template <typename T>
    using optional = typename impl::reference_traits<T>::itf;
}
