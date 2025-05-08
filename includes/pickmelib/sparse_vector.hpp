/*  sparse_vector.hpp
    MIT License

    Copyright (c) 2024 Aidar Shigapov

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

#ifndef SPARSE_VECTOR_HPP_
#define SPARSE_VECTOR_HPP_ 1

#ifndef SPARSE_VECTOR_DEFAULT_CONTAINER
#   include <vector>
#   define SPARSE_VECTOR_DEFAULT_CONTAINER std::vector
#endif

#ifndef SPARSE_VECTOR_SIZE_TYPE
#   include <cstddef>
#   define SPARSE_VECTOR_SIZE_TYPE std::size_t
#endif

#ifndef SPARSE_VECTOR_MOVE
#   include <utility>
#   define SPARSE_VECTOR_MOVE std::move
#endif

#if __cplusplus >= 201703L
# define WCS_SPARSE_VECTOR_NODISCARD [[nodiscard]]
#else
# define WCS_SPARSE_VECTOR_NODISCARD
#endif

#include <type_traits>
#include <exception>
#include <initializer_list>
#include <algorithm>

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

namespace wcs {
    namespace details {
        template<class U>
        typename std::enable_if<std::is_move_constructible<U>::value>::type
        move_place(U& dst, U& src) {
            new(&dst) U(SPARSE_VECTOR_MOVE(src));
        }

        template<class U>
        typename std::enable_if<!std::is_move_constructible<U>::value>::type
        move_place(U& dst, U& src) {
            new(&dst) U(src);
            src.~U();
        }
    };

    template <class T,
              class AllocatorT = std::allocator<T>,
              template <class...> class ContainerT = SPARSE_VECTOR_DEFAULT_CONTAINER>
    struct sparse_vector {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using size_type = SPARSE_VECTOR_SIZE_TYPE;

    protected:
        struct value_info final {
            value_type value;
            bool exist;
        };

    private:
        using container_type = ContainerT<size_type>;

    public:
#if (WCS_CPP_STANDARD >= 20)
        using allocator_traits = std::allocator_traits<AllocatorT>::template rebind_traits<value_info>;
        using allocator_type = std::allocator_traits<AllocatorT>::template rebind_alloc<value_info>;
#else
        using allocator_type = typename AllocatorT::template rebind<value_info>::other;
        using allocator_traits = std::allocator_traits<allocator_type>;
#endif

    private:
        typename allocator_traits::pointer data_;
        size_type size_;
        size_type capacity_;
        allocator_type allocator_;
        container_type freeIndeces_;

    private:
        void reallocate(size_type newCapacity) {
            typename allocator_traits::pointer newData = allocator_.allocate(newCapacity);
            for (size_type i = 0; i < size_; ++i) {
                if (!data_[i].exist) {
                    newData[i].exist = false;
                } else {
                    details::move_place<value_type>(newData[i].value, data_[i].value);
                    newData[i].exist = true;
                }
            }
            allocator_.deallocate(data_, capacity_);
            data_ = newData;
            capacity_ = newCapacity;
        }

        void mark_as_free(size_type i) {
            freeIndeces_.push_back(i);
            data_[i].exist = false;
        }

    public:
        sparse_vector() : data_(nullptr), size_(0), capacity_(2), allocator_(), freeIndeces_() {
            data_ = allocator_.allocate(capacity_);
        }

        explicit sparse_vector(const allocator_type& allocator) : data_(nullptr), size_(0), capacity_(2), allocator_(allocator), freeIndeces_() {
            data_ = allocator_.allocate(capacity_);
        }

        sparse_vector(const sparse_vector& other) : size_(other.size_), capacity_(other.capacity_), allocator_(other.allocator_), freeIndeces_(other.freeIndeces_) {
            data_ = allocator_.allocate(capacity_);
            for (size_type i = 0; i < size_; ++i) {
                value_info& cell = data_[i];
                const value_info& otherCell = other.data_[i];

                if (!otherCell.exist) {
                    cell.exist = false;
                } else {
                    new(&cell.value) value_type(otherCell.value);
                    cell.exist = true;
                }
            }
        }

        sparse_vector(sparse_vector&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_), allocator_(SPARSE_VECTOR_MOVE(other.allocator_)), freeIndeces_(SPARSE_VECTOR_MOVE(other.freeIndeces_)) {
            other.data_ = nullptr;
            other.size_ = 0;
        }

        sparse_vector(std::initializer_list<value_type> init) : data_(nullptr), size_(init.size()), capacity_(init.size()), allocator_(), freeIndeces_() {
            data_ = allocator_.allocate(capacity_);
            size_type i = 0;
            for (const auto& value : init) {
                value_info& cell = data_[i];
                new(&cell.value) value_type(value);
                cell.exist = true;
                ++i;
            }
        }

        ~sparse_vector() {
            if (data_ == nullptr)
                return;
            for (size_type i = 0; i < size_; ++i) {
                if (data_[i].exist) {
                    data_[i].value.~value_type();
                }
            }
            allocator_.deallocate(data_, capacity_);
            data_ = nullptr;
            size_ = 0;
        }

        sparse_vector& operator=(const sparse_vector& other) {
            if (this != &other) {
                clear();
                size_ = other.size_;
                capacity_ = other.capacity_;
                allocator_ = other.allocator_;
                freeIndeces_ = other.freeIndeces_;
                data_ = allocator_.allocate(capacity_);
                for (size_type i = 0; i < size_; ++i) {
                    value_info& cell = data_[i];
                    const value_info& otherCell = other.data_[i];

                    if (!otherCell.exist) {
                        cell.exist = false;
                    } else {
                        new(&cell.value) value_type(otherCell.value);
                        cell.exist = true;
                    }
                }
            }
            return *this;
        }

        sparse_vector& operator=(sparse_vector&& other) noexcept {
            if (this != &other) {
                clear();
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                allocator_ = SPARSE_VECTOR_MOVE(other.allocator_);
                freeIndeces_ = SPARSE_VECTOR_MOVE(other.freeIndeces_);
                other.data_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        size_type push_free(const_reference val) {
            size_type index;
            if (freeIndeces_.empty()) {
                if (size_ >= capacity_)
                    reallocate(capacity_ * 2);
                index = size_;
                ++size_;
            } else {
                index = freeIndeces_.back();
                freeIndeces_.pop_back();
            }

            value_info& cell = data_[index];
            new(&cell.value) value_type(val);
            cell.exist = true;
            return index;
        }

        template<class... ArgsT>
        size_type emplace_free(ArgsT&&... args) {
            size_type index;
            if (freeIndeces_.empty()) {
                if (size_ >= capacity_)
                    reallocate(capacity_ * 2);
                index = size_;
                ++size_;
            } else {
                index = freeIndeces_.back();
                freeIndeces_.pop_back();
            }

            value_info& cell = data_[index];
            new(&cell.value) value_type(std::forward<ArgsT>(args)...);
            cell.exist = true;
            return index;
        }

        void erase_at(size_type index) {
            if (index >= size_)
                throw std::out_of_range("out of sparse_vector range on erase_at.");
            value_info& cell = data_[index];
            if (!cell.exist)
                throw std::out_of_range("value doesnt exist in sparse_vector on this index. erase_at.");
            cell.value.~value_type();
            mark_as_free(index);
        }

        void pop_back() {
            if (size_ == 0)
                throw std::out_of_range("sparse_vector is empty on pop_back.");
            --size_;
            value_info& cell = data_[size_];
            cell.value.~value_type();
            cell.exist = false;
        }

        template<class FunctT>
        void fill_free_cells(FunctT funct) {
            for (size_type i = 0; i < size_; ++i) {
                value_info& cell = data_[i];
                if (!cell.exist) {
                    new(&cell.value) value_type(funct());
                    cell.exist = true;
                }
            }
            freeIndeces_.clear();
        }

        void reserve(size_type newCapacity) {
            if (capacity_ >= newCapacity)
                return;
            reallocate(newCapacity);
        }

        void resize(size_type newSize) {
            reserve(newSize);
            for (size_type i = size_; i < newSize; ++i) {
                mark_as_free(i);
            }
            size_ = newSize;
        }

        WCS_SPARSE_VECTOR_NODISCARD bool exists_at(size_type i) const noexcept {
            if (i >= size_)
                return false;
            const value_info& cell = data_[i];
            return cell.exist;
        }

        template<class... ArgsT>
        void emplace_at(size_type i, ArgsT&&... args) {
            if (i >= size_)
                throw std::out_of_range("index out of sparse_vector size on emplace_at.");
            value_info& cell = data_[i];
            if (cell.exist)
                throw std::out_of_range("value already exists in sparse_vector on this index. emplace_at.");
            new(&cell.value) value_type(std::forward<ArgsT>(args)...);
            cell.exist = true;
        }

        void clear() {
            for (size_type i = 0; i < size_; ++i) {
                value_info& cell = data_[i];
                if (cell.exist) {
                    cell.value.~value_type();
                    cell.exist = false;
                }
            }
            freeIndeces_.clear();
            size_ = 0;
        }

        WCS_SPARSE_VECTOR_NODISCARD size_type size() const noexcept {
            return size_;
        }
        WCS_SPARSE_VECTOR_NODISCARD size_type capacity() const noexcept {
            return capacity_;
        }

        WCS_SPARSE_VECTOR_NODISCARD const container_type& get_free_cells() const noexcept {
            return freeIndeces_;
        }

        WCS_SPARSE_VECTOR_NODISCARD reference operator[](size_type i) {
            return data_[i].value;
        }

        WCS_SPARSE_VECTOR_NODISCARD const_reference operator[](size_type i) const {
            return data_[i].value;
        }

        WCS_SPARSE_VECTOR_NODISCARD reference at(size_type i) {
            if (i >= size_)
                throw std::out_of_range("index out of sparse_vector size on at.");
            value_info& cell = data_[i];
            if (!cell.exist)
                throw std::out_of_range("value doesnt exist in sparse_vector on this index. at.");
            return data_[i].value;
        }

        WCS_SPARSE_VECTOR_NODISCARD const_reference at(size_type i) const {
            if (i >= size_)
                throw std::out_of_range("index out of sparse_vector size on at.");
            const value_info& cell = data_[i];
            if (!cell.exist)
                throw std::out_of_range("value doesnt exist in sparse_vector on this index. at.");
            return data_[i].value;
        }

        // https://en.cppreference.com/w/cpp/container/vector/back - `Calling back on an empty container causes undefined behavior`

        WCS_SPARSE_VECTOR_NODISCARD reference front() noexcept {
            return *(begin());
        }
        WCS_SPARSE_VECTOR_NODISCARD const_reference front() const noexcept {
            return *(cbegin());
        }

        WCS_SPARSE_VECTOR_NODISCARD reference back() noexcept {
            return *(end() - 1);
        }
        WCS_SPARSE_VECTOR_NODISCARD const_reference back() const noexcept {
            return *(cend() - 1);
        }

    public:
        struct iterator {
            using value_type = T;
            using reference = T&;
            using const_reference = const T&;
            using pointer = T*;
            using const_pointer = const T*;

        private:
            value_info* ptr_;
            const value_info* endPtr_;

        public:
            iterator(value_info* ptr, const value_info* endPtr) noexcept : ptr_(ptr), endPtr_(endPtr) {
                while ((ptr_ != endPtr_) && !ptr_->exist) {
                    ++ptr_;
                }
            }

            WCS_SPARSE_VECTOR_NODISCARD pointer operator->() noexcept {
                return &ptr_->value;
            }

            WCS_SPARSE_VECTOR_NODISCARD const_pointer operator->() const noexcept {
                return &ptr_->value;
            }

            WCS_SPARSE_VECTOR_NODISCARD reference operator*() noexcept {
                return ptr_->value;
            }

            WCS_SPARSE_VECTOR_NODISCARD const_reference operator*() const noexcept {
                return ptr_->value;
            }

            iterator& operator++() noexcept {
                ++ptr_;
                while ((ptr_ != endPtr_) && !ptr_->exist) {
                    ++ptr_;
                }
                return *this;
            }

            WCS_SPARSE_VECTOR_NODISCARD bool operator==(const iterator& other) const noexcept {
                return ptr_ == other.ptr_;
            }

            WCS_SPARSE_VECTOR_NODISCARD bool operator!=(const iterator& other) const noexcept {
                return ptr_ != other.ptr_;
            }
        };

        struct const_iterator {
            using value_type = T;
            using const_reference = const T&;
            using const_pointer = const T*;

        private:
            const value_info* ptr_;
            const value_info* endPtr_;

        public:
            const_iterator(const value_info* ptr, const value_info* endPtr) : ptr_(ptr), endPtr_(endPtr) {
                while ((ptr_ != endPtr_) && !ptr_->exist) {
                    ++ptr_;
                }
            }

            WCS_SPARSE_VECTOR_NODISCARD const_pointer operator->() const noexcept {
                return &ptr_->value;
            }

            WCS_SPARSE_VECTOR_NODISCARD const_reference operator*() const noexcept {
                return ptr_->value;
            }

            const_iterator& operator++() noexcept {
                ++ptr_;
                while ((ptr_ != endPtr_) && !ptr_->exist) {
                    ++ptr_;
                }
                return *this;
            }

            WCS_SPARSE_VECTOR_NODISCARD bool operator==(const const_iterator& other) const noexcept {
                return ptr_ == other.ptr_;
            }

            WCS_SPARSE_VECTOR_NODISCARD bool operator!=(const const_iterator& other) const noexcept {
                return ptr_ != other.ptr_;
            }
        };

        WCS_SPARSE_VECTOR_NODISCARD iterator begin() noexcept {
            return iterator(&data_[0], &data_[size_]);
        }

        WCS_SPARSE_VECTOR_NODISCARD iterator end() noexcept {
            return iterator(&data_[size_], &data_[size_]);
        }

        WCS_SPARSE_VECTOR_NODISCARD const_iterator cbegin() const noexcept {
            return const_iterator(&data_[0], &data_[size_]);
        }

        WCS_SPARSE_VECTOR_NODISCARD const_iterator cend() const noexcept {
            return const_iterator(&data_[size_], &data_[size_]);
        }

        WCS_SPARSE_VECTOR_NODISCARD size_type index_of(const iterator& i) const noexcept {
            return static_cast<size_type>(static_cast<ptrdiff_t>(i.ptr_ - data_));
        }

        WCS_SPARSE_VECTOR_NODISCARD size_type index_of(const const_iterator& i) const noexcept {
            return static_cast<size_type>(static_cast<ptrdiff_t>(i.ptr_ - data_));
        }
    };
};
#ifndef WCS_SPARSE_VECTOR_FORMATTER_IMPLEMENTATION
#define WCS_SPARSE_VECTOR_FORMATTER_IMPLEMENTATION 0
#endif

#if WCS_SPARSE_VECTOR_FORMATTER_IMPLEMENTATION
#if WCS_CPP_STANDARD >= 20
#include <format>
namespace std {
    template <class T,
              class AllocatorT = std::allocator<T>,
              template <class...> class ContainerT = SPARSE_VECTOR_DEFAULT_CONTAINER
              class CharT_>
    struct formatter<wcs::sparse_vector<T, AllocatorT, ContainerT>, CharT> {
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
                throw std::format_error("formatter<sparse_vector>::parse invalid format args for sparse_vector");
    
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
#endif // WCS_SPARSE_VECTOR_FORMATTER_IMPLEMENTATION
#undef WCS_CPP_STANDARD
#undef WCS_SPARSE_VECTOR_NODISCARD
#endif // SPARSE_VECTOR_HPP_
