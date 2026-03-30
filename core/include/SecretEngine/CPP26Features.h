#pragma once

/**
 * C++26 Feature Detection and Fallback Macros
 * 
 * This header provides feature detection for C++26 features and fallback
 * implementations for platforms that don't support them yet (e.g., Android NDK).
 * 
 * Design Principle: Dual-path from day one. Every C++26 feature must have a
 * C++23 fallback because Android NDK lags upstream Clang by 12-18 months.
 */

// ============================================================================
// Feature Detection
// ============================================================================

// Static Reflection (P2996R8)
#if defined(__cpp_reflection) && __cpp_reflection >= 202306L
    #define SE_HAS_REFLECTION 1
#else
    #define SE_HAS_REFLECTION 0
#endif

// Contracts (P2900)
#if defined(__cpp_contracts) && __cpp_contracts >= 202506L
    #define SE_HAS_CONTRACTS 1
#else
    #define SE_HAS_CONTRACTS 0
#endif

// std::execution (P2300)
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202600L
    #define SE_HAS_EXECUTION 1
#else
    #define SE_HAS_EXECUTION 0
#endif

// std::inplace_vector (P0843R14)
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L
    #define SE_HAS_INPLACE_VECTOR 1
    #include <inplace_vector>
#else
    #define SE_HAS_INPLACE_VECTOR 0
#endif

// Pack Indexing
#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L
    #define SE_HAS_PACK_INDEXING 1
#else
    #define SE_HAS_PACK_INDEXING 0
#endif

// std::optional<T&> (P2988R12)
#if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202110L
    #define SE_HAS_OPTIONAL_REF 1
#else
    #define SE_HAS_OPTIONAL_REF 0
#endif

// std::debugging (P2546)
#if defined(__cpp_lib_debugging) && __cpp_lib_debugging >= 202403L
    #define SE_HAS_DEBUGGING 1
    #include <debugging>
#else
    #define SE_HAS_DEBUGGING 0
#endif

// std::simd
#if defined(__cpp_lib_simd) && __cpp_lib_simd >= 202306L
    #define SE_HAS_SIMD 1
    #include <simd>
#else
    #define SE_HAS_SIMD 0
#endif

// std::span (C++20, but not always available in Android NDK)
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
    #define SE_HAS_SPAN 1
    #include <span>
#else
    #define SE_HAS_SPAN 0
#endif

// std::mdspan (C++23, but relevant)
#if defined(__cpp_lib_mdspan) && __cpp_lib_mdspan >= 202207L
    #define SE_HAS_MDSPAN 1
    #include <mdspan>
#else
    #define SE_HAS_MDSPAN 0
#endif

// constexpr placement new
#if defined(__cpp_constexpr_dynamic_alloc) && __cpp_constexpr_dynamic_alloc >= 202106L
    #define SE_HAS_CONSTEXPR_PLACEMENT_NEW 1
#else
    #define SE_HAS_CONSTEXPR_PLACEMENT_NEW 0
#endif

// ============================================================================
// Contract Macros with Fallback
// ============================================================================

#if SE_HAS_CONTRACTS
    #define SE_PRECONDITION(condition) pre(condition)
    #define SE_POSTCONDITION(condition) post(condition)
    #define SE_ASSERT(condition) assert(condition)
#else
    #include <cassert>
    #define SE_PRECONDITION(condition) assert(condition)
    #define SE_POSTCONDITION(condition) assert(condition)
    #define SE_ASSERT(condition) assert(condition)
#endif

// ============================================================================
// Debugging Macros with Fallback
// ============================================================================

#if SE_HAS_DEBUGGING
    #define SE_BREAKPOINT() std::breakpoint()
    #define SE_IS_DEBUGGER_PRESENT() std::is_debugger_present()
#else
    // Platform-specific fallbacks
    #if defined(_MSC_VER)
        #define SE_BREAKPOINT() __debugbreak()
        #define SE_IS_DEBUGGER_PRESENT() (::IsDebuggerPresent() != 0)
    #elif defined(__GNUC__) || defined(__clang__)
        #include <signal.h>
        #define SE_BREAKPOINT() raise(SIGTRAP)
        #define SE_IS_DEBUGGER_PRESENT() false  // No portable way
    #else
        #define SE_BREAKPOINT() ((void)0)
        #define SE_IS_DEBUGGER_PRESENT() false
    #endif
#endif

// ============================================================================
// span Fallback (C++20 feature, but not always available in Android NDK)
// ============================================================================

#if !SE_HAS_SPAN
    #include <cstddef>
    #include <type_traits>
    #include <iterator>
    
    namespace std {
        /**
         * Lightweight non-owning view over a contiguous sequence (C++20 std::span fallback)
         * 
         * This is a minimal implementation that provides the essential functionality
         * needed for the SecretEngine codebase.
         */
        inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);
        
        template<typename T, size_t Extent = dynamic_extent>
        class span {
        public:
            using element_type = T;
            using value_type = typename std::remove_cv<T>::type;
            using size_type = size_t;
            using difference_type = ptrdiff_t;
            using pointer = T*;
            using const_pointer = const T*;
            using reference = T&;
            using const_reference = const T&;
            using iterator = T*;
            using const_iterator = const T*;
            
            static constexpr size_t extent = Extent;
            
            // Constructors
            constexpr span() noexcept : data_(nullptr), size_(0) {
                static_assert(Extent == 0 || Extent == dynamic_extent, 
                             "Cannot default construct a span with static extent != 0");
            }
            
            constexpr span(pointer ptr, size_type count) noexcept 
                : data_(ptr), size_(count) {
                if constexpr (Extent != dynamic_extent) {
                    // In debug builds, assert the size matches
                    assert(count == Extent && "span size mismatch with static extent");
                }
            }
            
            constexpr span(pointer first, pointer last) noexcept 
                : data_(first), size_(static_cast<size_type>(last - first)) {
                if constexpr (Extent != dynamic_extent) {
                    assert(size_ == Extent && "span size mismatch with static extent");
                }
            }
            
            template<size_t N>
            constexpr span(element_type (&arr)[N]) noexcept 
                : data_(arr), size_(N) {
                static_assert(Extent == dynamic_extent || Extent == N,
                             "span extent mismatch with array size");
            }
            
            // Copy constructor
            constexpr span(const span&) noexcept = default;
            constexpr span& operator=(const span&) noexcept = default;
            
            // Element access
            constexpr reference operator[](size_type idx) const noexcept {
                assert(idx < size_ && "span index out of bounds");
                return data_[idx];
            }
            
            constexpr reference front() const noexcept {
                assert(!empty() && "span::front() called on empty span");
                return data_[0];
            }
            
            constexpr reference back() const noexcept {
                assert(!empty() && "span::back() called on empty span");
                return data_[size_ - 1];
            }
            
            constexpr pointer data() const noexcept { return data_; }
            
            // Iterators
            constexpr iterator begin() const noexcept { return data_; }
            constexpr iterator end() const noexcept { return data_ + size_; }
            constexpr const_iterator cbegin() const noexcept { return data_; }
            constexpr const_iterator cend() const noexcept { return data_ + size_; }
            
            // Capacity
            constexpr size_type size() const noexcept { return size_; }
            constexpr size_type size_bytes() const noexcept { return size_ * sizeof(T); }
            constexpr bool empty() const noexcept { return size_ == 0; }
            
            // Subviews
            constexpr span<T, dynamic_extent> first(size_type count) const {
                assert(count <= size_ && "span::first() count exceeds size");
                return span<T, dynamic_extent>(data_, count);
            }
            
            constexpr span<T, dynamic_extent> last(size_type count) const {
                assert(count <= size_ && "span::last() count exceeds size");
                return span<T, dynamic_extent>(data_ + (size_ - count), count);
            }
            
            constexpr span<T, dynamic_extent> subspan(size_type offset, 
                                                      size_type count = dynamic_extent) const {
                assert(offset <= size_ && "span::subspan() offset exceeds size");
                const size_type actual_count = (count == dynamic_extent) 
                    ? (size_ - offset) 
                    : count;
                assert(offset + actual_count <= size_ && "span::subspan() range exceeds size");
                return span<T, dynamic_extent>(data_ + offset, actual_count);
            }
            
        private:
            pointer data_;
            size_type size_;
        };
        
        // Deduction guides
        template<typename T, size_t N>
        span(T (&)[N]) -> span<T, N>;
        
        // Helper to convert byte spans
        template<typename T, size_t Extent>
        span<const byte, (Extent == dynamic_extent ? dynamic_extent : sizeof(T) * Extent)>
        as_bytes(span<T, Extent> s) noexcept {
            return span<const byte, (Extent == dynamic_extent ? dynamic_extent : sizeof(T) * Extent)>(
                reinterpret_cast<const byte*>(s.data()), s.size_bytes());
        }
        
        template<typename T, size_t Extent>
        span<byte, (Extent == dynamic_extent ? dynamic_extent : sizeof(T) * Extent)>
        as_writable_bytes(span<T, Extent> s) noexcept {
            return span<byte, (Extent == dynamic_extent ? dynamic_extent : sizeof(T) * Extent)>(
                reinterpret_cast<byte*>(s.data()), s.size_bytes());
        }
    }
#endif

// ============================================================================
// inplace_vector Fallback
// ============================================================================

#if !SE_HAS_INPLACE_VECTOR
    #include <cassert>
    #include <cstddef>
    #include <utility>
    #include <type_traits>
    
    namespace std {
        // Stack-based fixed-capacity vector (C++26 std::inplace_vector fallback)
        template<typename T, size_t N>
        class inplace_vector {
            static_assert(N > 0, "inplace_vector capacity must be greater than 0");
            
            alignas(T) std::byte storage_[N * sizeof(T)];
            size_t size_ = 0;
            
            T* data() noexcept {
                return reinterpret_cast<T*>(storage_);
            }
            
            const T* data() const noexcept {
                return reinterpret_cast<const T*>(storage_);
            }
            
        public:
            using value_type = T;
            using size_type = size_t;
            using difference_type = ptrdiff_t;
            using reference = T&;
            using const_reference = const T&;
            using pointer = T*;
            using const_pointer = const T*;
            using iterator = T*;
            using const_iterator = const T*;
            
            // Constructors
            constexpr inplace_vector() noexcept = default;
            
            constexpr inplace_vector(size_type count, const T& value) {
                assert(count <= N && "inplace_vector initial size exceeds capacity");
                for (size_type i = 0; i < count; ++i) {
                    push_back(value);
                }
            }
            
            // Destructor
            ~inplace_vector() {
                clear();
            }
            
            // Copy constructor
            inplace_vector(const inplace_vector& other) {
                for (size_type i = 0; i < other.size_; ++i) {
                    push_back(other[i]);
                }
            }
            
            // Move constructor
            inplace_vector(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
                for (size_type i = 0; i < other.size_; ++i) {
                    push_back(std::move(other[i]));
                }
                other.clear();
            }
            
            // Copy assignment
            inplace_vector& operator=(const inplace_vector& other) {
                if (this != &other) {
                    clear();
                    for (size_type i = 0; i < other.size_; ++i) {
                        push_back(other[i]);
                    }
                }
                return *this;
            }
            
            // Move assignment
            inplace_vector& operator=(inplace_vector&& other) noexcept(std::is_nothrow_move_assignable_v<T>) {
                if (this != &other) {
                    clear();
                    for (size_type i = 0; i < other.size_; ++i) {
                        push_back(std::move(other[i]));
                    }
                    other.clear();
                }
                return *this;
            }
            
            // Element access
            reference operator[](size_type i) noexcept {
                assert(i < size_ && "inplace_vector index out of bounds");
                return data()[i];
            }
            
            const_reference operator[](size_type i) const noexcept {
                assert(i < size_ && "inplace_vector index out of bounds");
                return data()[i];
            }
            
            reference at(size_type i) {
                if (i >= size_) {
                    // In a real implementation, throw std::out_of_range
                    assert(false && "inplace_vector::at() index out of bounds");
                }
                return data()[i];
            }
            
            const_reference at(size_type i) const {
                if (i >= size_) {
                    assert(false && "inplace_vector::at() index out of bounds");
                }
                return data()[i];
            }
            
            reference front() noexcept {
                assert(!empty() && "inplace_vector::front() called on empty vector");
                return data()[0];
            }
            
            const_reference front() const noexcept {
                assert(!empty() && "inplace_vector::front() called on empty vector");
                return data()[0];
            }
            
            reference back() noexcept {
                assert(!empty() && "inplace_vector::back() called on empty vector");
                return data()[size_ - 1];
            }
            
            const_reference back() const noexcept {
                assert(!empty() && "inplace_vector::back() called on empty vector");
                return data()[size_ - 1];
            }
            
            // Iterators
            iterator begin() noexcept { return data(); }
            const_iterator begin() const noexcept { return data(); }
            const_iterator cbegin() const noexcept { return data(); }
            
            iterator end() noexcept { return data() + size_; }
            const_iterator end() const noexcept { return data() + size_; }
            const_iterator cend() const noexcept { return data() + size_; }
            
            // Capacity
            bool empty() const noexcept { return size_ == 0; }
            size_type size() const noexcept { return size_; }
            static constexpr size_type capacity() noexcept { return N; }
            static constexpr size_type max_size() noexcept { return N; }
            
            // Modifiers
            void clear() noexcept {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    for (size_type i = 0; i < size_; ++i) {
                        data()[i].~T();
                    }
                }
                size_ = 0;
            }
            
            void push_back(const T& value) {
                assert(size_ < N && "inplace_vector capacity exceeded");
                new (&storage_[size_ * sizeof(T)]) T(value);
                ++size_;
            }
            
            void push_back(T&& value) {
                assert(size_ < N && "inplace_vector capacity exceeded");
                new (&storage_[size_ * sizeof(T)]) T(std::move(value));
                ++size_;
            }
            
            template<typename... Args>
            reference emplace_back(Args&&... args) {
                assert(size_ < N && "inplace_vector capacity exceeded");
                T* ptr = new (&storage_[size_ * sizeof(T)]) T(std::forward<Args>(args)...);
                ++size_;
                return *ptr;
            }
            
            void pop_back() noexcept {
                assert(!empty() && "inplace_vector::pop_back() called on empty vector");
                --size_;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    data()[size_].~T();
                }
            }
            
            void resize(size_type count) {
                assert(count <= N && "inplace_vector resize exceeds capacity");
                if (count < size_) {
                    // Shrink
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        for (size_type i = count; i < size_; ++i) {
                            data()[i].~T();
                        }
                    }
                } else if (count > size_) {
                    // Grow with default-constructed elements
                    for (size_type i = size_; i < count; ++i) {
                        new (&storage_[i * sizeof(T)]) T();
                    }
                }
                size_ = count;
            }
            
            void resize(size_type count, const T& value) {
                assert(count <= N && "inplace_vector resize exceeds capacity");
                if (count < size_) {
                    // Shrink
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        for (size_type i = count; i < size_; ++i) {
                            data()[i].~T();
                        }
                    }
                } else if (count > size_) {
                    // Grow with copies of value
                    for (size_type i = size_; i < count; ++i) {
                        new (&storage_[i * sizeof(T)]) T(value);
                    }
                }
                size_ = count;
            }
        };
    }
#endif
