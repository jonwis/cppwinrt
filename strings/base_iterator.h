
WINRT_EXPORT namespace winrt::impl
{
    template <typename T>
    struct fast_iterator
    {
        using iterator_concept = std::random_access_iterator_tag;
        using iterator_category = std::input_iterator_tag;
        using value_type = decltype(std::declval<T>().GetAt(0));
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

        fast_iterator() noexcept = default;

        fast_iterator(T const& collection, std::uint32_t const index) noexcept :
            m_collection(&collection),
            m_index(index)
        {}

        fast_iterator& operator++() noexcept
        {
            ++m_index;
            return *this;
        }

        fast_iterator operator++(int) noexcept
        {
            auto previous = *this;
            ++m_index;
            return previous;
        }

        fast_iterator& operator--() noexcept
        {
            --m_index;
            return *this;
        }

        fast_iterator operator--(int) noexcept
        {
            auto previous = *this;
            --m_index;
            return previous;
        }

        fast_iterator& operator+=(difference_type n) noexcept
        {
            m_index += static_cast<std::uint32_t>(n);
            return *this;
        }

        fast_iterator operator+(difference_type n) const noexcept
        {
            return fast_iterator(*this) += n;
        }

        fast_iterator& operator-=(difference_type n) noexcept
        {
            return *this += -n;
        }

        fast_iterator operator-(difference_type n) const noexcept
        {
            return *this + -n;
        }

        difference_type operator-(fast_iterator const& other) const noexcept
        {
            WINRT_ASSERT(m_collection == other.m_collection);
            return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
        }

        reference operator*() const
        {
            return fetch(m_index);
        }

        reference operator[](difference_type n) const
        {
            return fetch(m_index + static_cast<std::uint32_t>(n));
        }

        bool operator==(fast_iterator const& other) const noexcept
        {
            WINRT_ASSERT(m_collection == other.m_collection);
            return m_index == other.m_index;
        }

        bool operator<(fast_iterator const& other) const noexcept
        {
            WINRT_ASSERT(m_collection == other.m_collection);
            return m_index < other.m_index;
        }

        bool operator>(fast_iterator const& other) const noexcept
        {
            WINRT_ASSERT(m_collection == other.m_collection);
            return m_index > other.m_index;
        }

        bool operator!=(fast_iterator const& other) const noexcept
        {
            return !(*this == other);
        }

        bool operator<=(fast_iterator const& other) const noexcept
        {
            return !(*this > other);
        }

        bool operator>=(fast_iterator const& other) const noexcept
        {
            return !(*this < other);
        }

        friend fast_iterator operator+(difference_type n, fast_iterator it) noexcept
        {
            return it + n;
        }

        friend fast_iterator operator-(difference_type n, fast_iterator it) noexcept
        {
            return it - n;
        }

    private:

        // Batched forward traversal for random-access (GetAt-capable) collections. Instead of one
        // GetAt ABI call per element, `fetch` fills a small block with a single GetMany call and
        // serves in-window reads from it, so range-for (sequential ++ then *) crosses the ABI
        // ~once per `block` elements. Random access still works: an out-of-window index re-anchors
        // the block there, and an at/after-end index defers to GetAt so the component's bounds
        // behavior (E_BOUNDS) is preserved. Elements are copied out -- no move-out -- so a given
        // index may be read repeatedly, as the random-access contract requires.
        static constexpr std::uint32_t block = static_cast<std::uint32_t>(
            (std::min<std::size_t>)(128, (std::max<std::size_t>)(1, std::size_t{ 2048 } / sizeof(value_type))));

        reference fetch(std::uint32_t const index) const
        {
            if (index < m_buffer_base || index >= m_buffer_base + m_buffer_size)
            {
                for (std::uint32_t i = 0; i < m_buffer_size; ++i)
                {
                    m_buffer[i] = value_type{};
                }
                m_buffer_base = index;
                m_buffer_size = m_collection->GetMany(index, m_buffer);
            }

            if (index < m_buffer_base + m_buffer_size)
            {
                return m_buffer[index - m_buffer_base];
            }

            return m_collection->GetAt(index);
        }

        T const* m_collection = nullptr;
        std::uint32_t m_index = 0;
        mutable std::uint32_t m_buffer_base = 0;
        mutable std::uint32_t m_buffer_size = 0;
        mutable std::array<value_type, block> m_buffer{};
    };

    template <typename T>
    class has_GetAt
    {
        template <typename U, typename = decltype(std::declval<U>().GetAt(0))> static constexpr bool get_value(int) { return true; }
        template <typename> static constexpr bool get_value(...) { return false; }

    public:

        static constexpr bool value = get_value<T>(0);
    };

    // Forward iterator that batches an IIterator via GetMany into a small buffer and yields from
    // it, so range-for over a collection that lacks GetAt crosses the ABI once per block instead of
    // once per element (Current/MoveNext). Single-pass, matching IIterator's semantics.
    template <typename Iterator>
    struct buffered_iterator
    {
        using value_type = decltype(std::declval<Iterator>().Current());
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type const*;
        using reference = value_type const&;

        static constexpr std::uint32_t buffer_capacity = static_cast<std::uint32_t>(
            (std::min<std::size_t>)(128, (std::max<std::size_t>)(1, std::size_t{ 2048 } / sizeof(value_type))));

        buffered_iterator() noexcept = default;

        explicit buffered_iterator(Iterator iterator) : m_iterator(std::move(iterator))
        {
            fill();
        }

        reference operator*() const noexcept
        {
            return m_buffer[m_index];
        }

        pointer operator->() const noexcept
        {
            return std::addressof(m_buffer[m_index]);
        }

        buffered_iterator& operator++()
        {
            if (++m_index == m_size)
            {
                fill();
            }

            return *this;
        }

        buffered_iterator operator++(int)
        {
            auto previous = *this;
            ++*this;
            return previous;
        }

        bool operator==(buffered_iterator const& other) const noexcept
        {
            return (m_size == 0) && (other.m_size == 0);
        }

        bool operator!=(buffered_iterator const& other) const noexcept
        {
            return !(*this == other);
        }

    private:

        void fill()
        {
            m_index = 0;
            m_size = m_iterator ? m_iterator.GetMany(m_buffer) : 0;
        }

        Iterator m_iterator{ nullptr };
        std::array<value_type, buffer_capacity> m_buffer;
        std::uint32_t m_size{ 0 };
        std::uint32_t m_index{ 0 };
    };

    template <typename T, std::enable_if_t<!has_GetAt<T>::value, int> = 0>
    auto get_begin_iterator(T const& collection)
    {
        return buffered_iterator<decltype(collection.First())>{ collection.First() };
    }

    template <typename T, std::enable_if_t<!has_GetAt<T>::value, int> = 0>
    auto get_end_iterator([[maybe_unused]] T const& collection) noexcept
    {
        return buffered_iterator<decltype(std::declval<T const&>().First())>{};
    }

    template <typename T, std::enable_if_t<has_GetAt<T>::value, int> = 0>
    fast_iterator<T> get_begin_iterator(T const& collection) noexcept
    {
        return { collection, 0 };
    }

    template <typename T, std::enable_if_t<has_GetAt<T>::value, int> = 0>
    fast_iterator<T> get_end_iterator(T const& collection)
    {
        return { collection, collection.Size() };
    }

    template <typename T, std::enable_if_t<has_GetAt<T>::value, int> = 0>
    auto rbegin(T const& collection)
    {
        return std::make_reverse_iterator(get_end_iterator(collection));
    }

    template <typename T, std::enable_if_t<has_GetAt<T>::value, int> = 0>
    auto rend(T const& collection)
    {
        return std::make_reverse_iterator(get_begin_iterator(collection));
    }

    using std::begin;
    using std::end;
}
