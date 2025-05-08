#ifndef WINPLACE_VECTOR_HPP_ 
#define WINPLACE_VECTOR_HPP_ 1

#ifndef WCSINPLACE_VECTOR_SIZE_TYPE
#   include <cstddef>
#   define WCSINPLACE_VECTOR_SIZE_TYPE std::size_t
#endif

#ifndef WCSINPLACE_VECTOR_MOVE
#   include <utility>
#   define WCSINPLACE_VECTOR_MOVE std::move
#endif

#if __cplusplus >= 201703L
# define WCS_INPLACEVECTOR_NODISCARD [[nodiscard]]
#else
# define WCS_INPLACEVECTOR_NODISCARD
#endif

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <iterator>

namespace wcs {
    template<WCSINPLACE_VECTOR_SIZE_TYPE Len, WCSINPLACE_VECTOR_SIZE_TYPE Align = alignof(std::max_align_t)>
    struct tiny_aligned_storage {
        struct type {
            alignas(Align) unsigned char data[Len];
        };
    };

    template <WCSINPLACE_VECTOR_SIZE_TYPE Len, WCSINPLACE_VECTOR_SIZE_TYPE Align = alignof(std::max_align_t)>
    using tiny_aligned_storage_t = typename tiny_aligned_storage<Len, Align>::type;

    template <class InputIt>
    typename std::enable_if<
        std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>::value,
        typename std::iterator_traits<InputIt>::difference_type
    >::type
    simple_distance(InputIt first, InputIt last) {
        return last - first;
    }

    template <class InputIt>
    typename std::enable_if<
        !std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>::value,
        typename std::iterator_traits<InputIt>::difference_type
    >::type
    simple_distance(InputIt first, InputIt last) {
        typename std::iterator_traits<InputIt>::difference_type n = 0;
        while (first != last) {
            ++first;
            ++n;
        }
        return n;
    }

    template <class T, WCSINPLACE_VECTOR_SIZE_TYPE Capacity_>
    struct inplace_vector {
        public:
        using value_type = T;

        using size_type = WCSINPLACE_VECTOR_SIZE_TYPE;

        using reference = T&;
        using const_reference = const T&;

        using pointer = T*;
        using const_pointer = const T*;

        using iterator = T*;
        using const_iterator = const T*;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        private:
        tiny_aligned_storage_t<sizeof(T), alignof(T)> data_[Capacity_];
        size_type size_;

        public:
        inplace_vector() : data_{}, size_(0) {

        }
        inplace_vector(const inplace_vector& other) : data_{}, size_(0) {
            assign(other.begin(), other.end());
        }
        template<class IteratorT_>
        inplace_vector(IteratorT_ begino, IteratorT_ endo) : data_{}, size_(0) {
            insert(begin(), begino, endo);
        }
        inplace_vector(std::initializer_list<value_type> list) : data_{}, size_(0) {
            insert(begin(), list);
        }
        inplace_vector(inplace_vector&& other) noexcept : data_{}, size_(other.size_) {
            WCSINPLACE_VECTOR_MOVE(other.begin(), other.end(), begin());
            other.clear();
        }

        public:
        ~inplace_vector() {
            clear();
        }

        public:
        inplace_vector& operator=(const inplace_vector& other) {
            if (this != &other) {
                assign(other.begin(), other.end());
            }
            return *this;
        }
        inplace_vector& operator=(inplace_vector&& other) noexcept  {
            if (this != &other) {
                clear();
                size_ = other.size_;
                WCSINPLACE_VECTOR_MOVE(other.begin(), other.end(), begin());
                other.clear();
            }
            return *this;
        }  

        public:
        void assign(size_type count, const T& value) {
            clear();
            for (size_type i = 0; i < count; ++i) {
                emplace_back(value);
            }
        }

        template <class InputIt>
        void assign(InputIt first, InputIt last) {
            clear();
            for (; first != last; ++first) {
                emplace_back(*first);
            }
        }

        void assign_range(const inplace_vector& other) {
            assign(other.begin(), other.end());
        }

        public:
        WCS_INPLACEVECTOR_NODISCARD size_type size() const noexcept {
            return size_;
        }

        WCS_INPLACEVECTOR_NODISCARD size_type capacity() const noexcept {
            return Capacity_;
        }

        WCS_INPLACEVECTOR_NODISCARD bool empty() const noexcept {
            return size_ == 0;
        }

        WCS_INPLACEVECTOR_NODISCARD static constexpr size_type max_size() noexcept {
            return Capacity_;
        }

        WCS_INPLACEVECTOR_NODISCARD reference operator[](size_type index) noexcept {
            return *reinterpret_cast<pointer>(&data_[index]);
        }

        WCS_INPLACEVECTOR_NODISCARD const_reference operator[](size_type index) const noexcept {
            return *reinterpret_cast<const_pointer>(&data_[index]);
        }

        WCS_INPLACEVECTOR_NODISCARD reference at(size_type index) {
            if (index >= size_) {
                throw std::out_of_range("wcs::inplace_vector::at index out of range");
            }
            return *reinterpret_cast<pointer>(&data_[index]);
        }

        WCS_INPLACEVECTOR_NODISCARD const_reference at(size_type index) const {
            if (index >= size_) {
                throw std::out_of_range("wcs::inplace_vector::at index out of range");
            }
            return *reinterpret_cast<const_pointer>(&data_[index]);
        }

        // https://en.cppreference.com/w/cpp/container/vector/back - 'Calling back on an empty container causes undefined behavior.'

        WCS_INPLACEVECTOR_NODISCARD reference front() noexcept {
            return (*this)[0];
        }
        WCS_INPLACEVECTOR_NODISCARD const_reference front() const noexcept {
            return (*this)[0];
        }

        WCS_INPLACEVECTOR_NODISCARD reference back() noexcept {
            return (*this)[size_ - 1];
        }
        WCS_INPLACEVECTOR_NODISCARD const_reference back() const noexcept {
            return (*this)[size_ - 1];
        }

        WCS_INPLACEVECTOR_NODISCARD pointer data() noexcept {
            return reinterpret_cast<pointer>(&data_[0]);
        }
        WCS_INPLACEVECTOR_NODISCARD const_pointer data() const noexcept {
            return reinterpret_cast<const_pointer>(&data_[0]);
        }

        public:
        WCS_INPLACEVECTOR_NODISCARD iterator begin() noexcept {
            return reinterpret_cast<iterator>(&data_[0]);
        }
        WCS_INPLACEVECTOR_NODISCARD const_iterator begin() const noexcept {
            return reinterpret_cast<const_iterator>(&data_[0]);
        }

        iterator end() noexcept {
            return reinterpret_cast<iterator>(&data_[size_]);
        }
        const_iterator end() const noexcept {
            return reinterpret_cast<const_iterator>(&data_[size_]);
        }

        const_iterator cbegin() const noexcept {
            return begin();
        }

        const_iterator cend() const noexcept {
            return end();
        }

        reverse_iterator rbegin() noexcept {
            return reverse_iterator(end());
        }
        const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator(end());
        }

        reverse_iterator rend() noexcept {
            return reverse_iterator(begin());
        }
        const_reverse_iterator rend() const noexcept {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crbegin() const noexcept {
            return rbegin();
        }

        const_reverse_iterator crend() const {
            return rend();
        }

        public:
        void push_back(const T& value) {
            if (size_ >= Capacity_) {
                throw std::length_error("wcs::inplace_vector::push_back vector is full");
            }
            new(&data_[size_++]) value_type(value);
            // size++ after posible exception
        }

        template<class... ArgsT>
        void emplace_back(ArgsT&&... args) {
            if (size_ >= Capacity_) {
                throw std::length_error("wcs::inplace_vector::emplace_back vector is full");
            }
            new(&data_[size_++]) value_type(std::forward<ArgsT>(args)...);
            // size++ after posible exception
        }
        // exception in destructor - UB
        void pop_back() noexcept {
            if (size_ > 0) {
                --size_;
                reinterpret_cast<value_type*>(&data_[size_])->~value_type();
            }
        }
        // exception in destructor - UB
        void clear() noexcept {
            for (size_type i = 0; i < size_; ++i) {
                reinterpret_cast<value_type*>(&data_[i])->~value_type();
            }
            size_ = 0;
        }

        void resize(size_type count, const T& value) {
            if (count > Capacity_) {
                throw std::length_error("wcs::inplace_vector::resize requested size exceeds capacity");
            }
            if (count < size_) {
                for (size_type i = count; i < size_; ++i) {
                    reinterpret_cast<value_type*>(&data_[i])->~value_type();
                }
            } else {
                size_type i = size_;
                try {
                    for (; i < count; ++i) {
                        new(&data_[i]) value_type(value);
                    }
                } catch (...) {
                    for (size_type j = size_; j < i; ++j) {
                        reinterpret_cast<value_type*>(&data_[j])->~value_type();
                    }
                    throw;
                }
            }
            size_ = count;
        }
        void resize(size_type count) {
            resize(count, value_type());
        }

        void reserve(size_type new_cap) {
            if (new_cap > Capacity_) {
                throw std::length_error("wcs::inplace_vector::reserve requested capacity exceeds maximum capacity");
            }
        }
        void shrink_to_fit() noexcept {
            // No-op for inplace_vector as it has fixed capacity
        }
        iterator insert(const_iterator pos, const T& value) {
            if (size_ >= Capacity_) {
                throw std::length_error("wcs::inplace_vector::insert vector is full");
            }
            if (pos == cend()) {
                emplace_back(value);
                return end() - 1;
            }
            const size_type index = simple_distance(cbegin(), pos);
            new(&data_[size_]) value_type(back());
            std::copy_backward(begin() + index, end() - 1, end());
            *(begin() + index) = value;
            ++size_;
            return begin() + index;
        }
        iterator insert(const_iterator pos, T&& value) {
            if (size_ >= Capacity_) {
                throw std::length_error("wcs::inplace_vector::insert vector is full");
            }
            if (pos == cend()) {
                emplace_back(std::move(value));
                return end() - 1;
            }
            const size_type index = simple_distance(cbegin(), pos);
            new(&data_[size_]) value_type(std::move(back()));
            std::move_backward(begin() + index, end() - 1, end());
            *(begin() + index) = std::move(value);
            ++size_;
            return begin() + index;
        }
        template <class InputIt>
        iterator insert(const_iterator pos, InputIt first, InputIt last) {
            size_type count = simple_distance(first, last);
            if (size_ + count > Capacity_) {
                throw std::length_error("wcs::inplace_vector::insert range exceeds capacity");
            }
            const size_type index = simple_distance(cbegin(), pos);
            const size_type new_size = size_ + count;
            for (size_type i = size_; i < new_size; ++i) {
                new(&data_[i]) value_type(back());
            }
            size_ = new_size;
            std::move_backward(begin() + index, end() - count, end());
            std::copy(first, last, begin() + index);
            return begin() + index;
        }
        iterator insert(const_iterator pos, std::initializer_list<value_type> list) {
            return insert(pos, list.begin(), list.end());
        }
        iterator insert(const_iterator pos, size_type count, const value_type& value) {
            if (size_ + count > Capacity_) {
                throw std::length_error("wcs::inplace_vector::insert count exceeds capacity");
            }
            const size_type index = simple_distance(cbegin(), pos);
            const size_type new_size = size_ + count;
            for (size_type i = size_; i < new_size; ++i) {
                new(&data_[i]) value_type(back());
            }
            size_ = new_size;
            std::move_backward(begin() + index, end() - count, end());
            std::fill_n(begin() + index, count, value);
            return begin() + index;
        }

        void erase(iterator pos) {
            if (pos >= begin() && pos < end()) {
                std::swap_ranges(pos + 1, end(), pos);
                --size_;
                reinterpret_cast<value_type*>(&data_[size_])->~value_type();
            }
        }
        void erase(iterator first, iterator last) {
            if (first >= begin() && last <= end() && first < last) {
                std::swap_ranges(last, end(), first);
                const size_type count = (size_type)simple_distance(first, last);
                size_ -= count;
                for (size_type i = size_; i < size_ + count; ++i) {
                    reinterpret_cast<value_type*>(&data_[i])->~value_type();
                }
            }
        }

        void swap(inplace_vector& other) noexcept {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
        }
        
    };
};

#ifndef WCS_INPLACE_VECTOR_FORMATTER_IMPLEMENTATION
#define WCS_INPLACE_VECTOR_FORMATTER_IMPLEMENTATION 0
#endif

#if WCS_INPLACE_VECTOR_FORMATTER_IMPLEMENTATION
#ifdef _MSC_VER
    #if _MSVC_LANG >= 202002L
        #define WCS_CPP_STANDARD 20
    #elif _MSVC_LANG >= 201703L
        #define WCS_CPP_STANDARD 17
    #elif _MSVC_LANG >= 201402L
        #define WCS_CPP_STANDARD 14
    #elif _MSVC_LANG >= 201103L
        #define WCS_CPP_STANDARD 11
    #else
        #define WCS_CPP_STANDARD 98
    #endif
#else
    #if __cplusplus >= 202002L
        #define WCS_CPP_STANDARD 20
    #elif __cplusplus >= 201703L
        #define WCS_CPP_STANDARD 17
    #elif __cplusplus >= 201402L
        #define WCS_CPP_STANDARD 14
    #elif __cplusplus >= 201103L
        #define WCS_CPP_STANDARD 11
    #else
        #define WCS_CPP_STANDARD 98
    #endif
#endif

#if WCS_CPP_STANDARD >= 20
#include <format>
namespace std {
    template<class T, std::size_t Capacity_, class CharT>
    struct formatter<wcs::inplace_vector<T, Capacity_>, CharT> {
        bool quoted = false;

        template<class ParseContext>
        constexpr ParseContext::iterator parse(ParseContext& ctx)
        {
            auto it = ctx.begin();
            if (it == ctx.end())
                return it;
    
            if (*it == (CharT)'#')
            {
                quoted = true;
                ++it;
            }
            if (it != ctx.end() && *it != (CharT)'}')
                throw std::format_error("formatter<inplace_vector>::parse invalid format args for inplace_vector");
    
            return it;
        }

        template<class FmtContext>
        FmtContext::iterator format(const wcs::inplace_vector<T, Capacity_>& s, FmtContext& ctx) const
        {
            std::basic_ostringstream<CharT, char_traits<CharT>, allocator<CharT>> out;
            out << "[";
            for (const auto& o : s)
                out << o << ", ";
            if (!s.empty())
                out.seekp(-2, std::ios_base::end);
            out << "]";
    
            return std::ranges::copy(std::move(out).str(), ctx.out()).out;
        }
    };
}
#endif // >= C++20
#undef WCS_CPP_STANDARD
#endif // WCS_INPLACE_VECTOR_FORMATTER_IMPLEMENTATION
#undef WCS_INPLACEVECTOR_NODISCARD
#endif // WINPLACE_VECTOR_HPP_
