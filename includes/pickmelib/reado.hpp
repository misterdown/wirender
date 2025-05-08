#ifndef WCS_READO_HPP_
#define WCS_READO_HPP_ 1

#ifndef WCS_READO_SIZE_TYPE
#   include <cstddef>
#   define WCS_READO_SIZE_TYPE std::size_t
#endif

#if __cplusplus >= 201703L
# define WCS_READO_NODISCARD [[nodiscard]]
#else
# define WCS_READO_NODISCARD
#endif

#include <algorithm>
#include <iterator>
#include <limits>

namespace wcs {
    template <class T_>
    struct data_view {
        private:
        using constructor_pointer = T_ const*;

        public:
        using value_type = T_;
        using referens = T_&;
        using const_referens = const T_&;
        using pointer = T_*;
        using const_pointer = const T_*;
        using iterator = const T_*;
        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator> ;
        using size_type = WCS_READO_SIZE_TYPE;
        using difference_type = std::ptrdiff_t;

        public:
        data_view() :  size_(0), data_(nullptr) {

        }
        data_view(const data_view& other) : data_(other.data_), size_(other.size_) {

        }
        data_view(constructor_pointer dt, size_type sz) : size_(sz), data_((pointer)dt) {

        }

        public:
        WCS_READO_NODISCARD pointer data() noexcept {
            return data_;
        }
        WCS_READO_NODISCARD const_pointer data() const noexcept {
            return data_;
        }
        WCS_READO_NODISCARD size_type size() const noexcept {
            return size_;
        }
        WCS_READO_NODISCARD bool empty() const noexcept {
            return size_ == 0;
        }

        public:
        WCS_READO_NODISCARD referens operator[](size_type pos) {
            return data_[pos];
        }
        WCS_READO_NODISCARD const_referens operator[](size_type pos) const {
            return data_[pos];
        }
        WCS_READO_NODISCARD referens at(size_type pos) {
            if (pos > size_) {
                throw std::out_of_range("data_view::at");
            }
            return data_[pos];
        }
        WCS_READO_NODISCARD const_referens at(size_type pos) const {
            if (pos > size_) {
                throw std::out_of_range("data_view::at");
            }
            return data_[pos];
        }

        public:
        /**
         * @brief Returns a subview of the data_view.
         *
         * This method creates a new data_view that represents a subset of the original data_view.
         * The subset starts at the position `pos` and has a length of `count` elements.
         * If `pos` is greater than the size of the data_view, an `std::out_of_range` exception is thrown.
         * If `count` is greater than the remaining elements from `pos` to the end of the data_view,
         * `count` is adjusted to the remaining number of elements.
         *
         * @param pos The starting position of the subview.
         * @param count The number of elements in the subview.
         * @return A new data_view representing the subset.
         * @throw std::out_of_range If `pos` is greater than the size of the data_view.
         */
        WCS_READO_NODISCARD data_view subview(size_type pos = 0, size_type count = std::numeric_limits<size_type>::max()) const {
            if (pos > size_) {
                throw std::out_of_range("data_view::subview");
            }
            count = std::min(count, size_ - pos);
            return data_view(data_ + pos, count);
        }

        public:
        WCS_READO_NODISCARD iterator begin() {
            return iterator(data_);
        }
        WCS_READO_NODISCARD const_iterator begin() const {
            return const_iterator(data_);
        }
        WCS_READO_NODISCARD iterator end() {
            return iterator(data_  + size_);
        }
        WCS_READO_NODISCARD const_iterator end() const {
            return const_iterator(data_ + size_);
        }

        WCS_READO_NODISCARD reverse_iterator rbegin() {
            return reverse_iterator(end());
        }
        WCS_READO_NODISCARD const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }
        WCS_READO_NODISCARD reverse_iterator rend() {
            return reverse_iterator(begin());
        }
        WCS_READO_NODISCARD const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        private:
        size_type size_;
        pointer data_;
    };

    template<class CharT_>
    struct tiny_string_view {
        private:
        using constructor_pointer = CharT_ const*;

        public:
        using traits_type = std::char_traits<CharT_>;
        using value_type = CharT_;
        using referens = CharT_&;
        using const_referens = const CharT_&;
        using pointer = CharT_*;
        using const_pointer = const CharT_*;
        using iterator = const CharT_*;
        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator> ;
        using size_type = WCS_READO_SIZE_TYPE;
        using difference_type = std::ptrdiff_t;

        public:
        tiny_string_view() : data_(nullptr), size_(0) {

        }
        tiny_string_view(constructor_pointer dt, size_type sz) : data_((pointer)dt), size_(sz) {

        }
        tiny_string_view(constructor_pointer dt) : data_((pointer)dt), size_(traits_type::length((pointer)dt)) {
            
        }   

        public:
        WCS_READO_NODISCARD pointer data() noexcept {
            return data_;
        }
        WCS_READO_NODISCARD const_pointer data() const noexcept {
            return data_;
        }
        WCS_READO_NODISCARD const_pointer c_str() const noexcept {
            return data_;
        }
        WCS_READO_NODISCARD size_type size() const noexcept {
            return size_;
        }
        WCS_READO_NODISCARD size_type length() const noexcept {
            return size_;
        }

        public:
        WCS_READO_NODISCARD referens operator[](size_type pos) {
            return data_[pos];
        }
        WCS_READO_NODISCARD const_referens operator[](size_type pos) const {
            return data_[pos];
        }
        WCS_READO_NODISCARD referens at(size_type pos) {
            if (pos > size_) {
                throw std::out_of_range("tiny_string_view::at");
            }
            return data_[pos];
        }
        WCS_READO_NODISCARD const_referens at(size_type pos) const {
            if (pos > size_) {
                throw std::out_of_range("tiny_string_view::at");
            }
            return data_[pos];
        }

        public:
        /**
         * @brief Returns a subview of the tiny_string_view.
         *
         * This method creates a new tiny_string_view that represents a subset of the original tiny_string_view.
         * The subset starts at the position `pos` and has a length of `count` elements.
         * If `pos` is greater than the size of the tiny_string_view, an `std::out_of_range` exception is thrown.
         * If `count` is greater than the remaining elements from `pos` to the end of the tiny_string_view,
         * `count` is adjusted to the remaining number of elements.
         *
         * @param pos The starting position of the subview.
         * @param count The number of elements in the subview.
         * @return A new tiny_string_view representing the subset.
         * @throw std::out_of_range If `pos` is greater than the size of the tiny_string_view.
         */
        WCS_READO_NODISCARD tiny_string_view subview(size_type pos = 0, size_type count = std::numeric_limits<size_type>::max()) const {
            if (pos > size_) {
                throw std::out_of_range("tiny_string_view::subview");
            }
            count = std::min(count, size_ - pos);
            return tiny_string_view(data_ + pos, count);
        }
        /**
         * @brief Returns a substring view of the tiny_string_view.
         *
         * This method is an alias for the `subview` method and creates a new tiny_string_view that represents
         * a subset of the original tiny_string_view. The subset starts at the position `pos` and has a length
         * of `count` elements. If `pos` is greater than the size of the tiny_string_view, an `std::out_of_range`
         * exception is thrown. If `count` is greater than the remaining elements from `pos` to the end of the
         * tiny_string_view, `count` is adjusted to the remaining number of elements.
         *
         * @param pos The starting position of the substring view.
         * @param count The number of elements in the substring view.
         * @return A new tiny_string_view representing the subset.
         * @throw std::out_of_range If `pos` is greater than the size of the tiny_string_view.
         */
        WCS_READO_NODISCARD tiny_string_view substr(size_type pos = 0, size_type count = std::numeric_limits<size_type>::max()) const {
            return subview(pos, count);
        }

        public:
        WCS_READO_NODISCARD iterator begin() {
            return iterator(data_);
        }
        WCS_READO_NODISCARD const_iterator begin() const {
            return const_iterator(data_);
        }
        WCS_READO_NODISCARD iterator end() {
            return iterator(data_  + size_);
        }
        WCS_READO_NODISCARD const_iterator end() const {
            return const_iterator(data_ + size_);
        }

        WCS_READO_NODISCARD reverse_iterator rbegin() {
            return reverse_iterator(end());
        }
        WCS_READO_NODISCARD const_reverse_iterator rbegin() const {
            return const_reverse_iterator(end());
        }
        WCS_READO_NODISCARD reverse_iterator rend() {
            return reverse_iterator(begin());
        }
        WCS_READO_NODISCARD const_reverse_iterator rend() const {
            return const_reverse_iterator(begin());
        }

        private:
        pointer data_;
        size_type size_;
    };
    /**
     * @brief Outputs the contents of a tiny_string_view to a stream.
     *
     * This operator overload allows a tiny_string_view to be output to a stream.
     * It iterates over each character in the tiny_string_view and writes it to the stream.
     *
     * @param stream The output stream to write to.
     * @param strview The tiny_string_view to output.
     * @return The output stream.
     */
    template<class StreamT_, class CharT_>
    StreamT_& operator<<(StreamT_& stream, const tiny_string_view<CharT_>& strview) {
        for (auto c : strview) {
            stream << c;
        }
        return stream;
    }
    template <class CharT_>
    WCS_READO_NODISCARD bool operator==(const tiny_string_view<CharT_>& lhs, const tiny_string_view<CharT_>& rhs) {
        return (lhs.size() == rhs.size()) && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    template <class CharT_>
    WCS_READO_NODISCARD bool operator!=(const tiny_string_view<CharT_>& lhs, const tiny_string_view<CharT_>& rhs) {
        return !(lhs == rhs);
    }
    template <class CharT_>
    WCS_READO_NODISCARD bool operator<(const tiny_string_view<CharT_>& lhs, const tiny_string_view<CharT_>& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    template <class CharT_>
    WCS_READO_NODISCARD bool operator>(const tiny_string_view<CharT_>& lhs, const tiny_string_view<CharT_>& rhs) {
        return rhs < lhs;
    }
    template <class CharT_>
    WCS_READO_NODISCARD bool operator<=(const tiny_string_view<CharT_>& lhs, const tiny_string_view<CharT_>& rhs) {
        return !(rhs < lhs);
    }
    template <class CharT_>
    WCS_READO_NODISCARD bool operator>=(const tiny_string_view<CharT_>& lhs, const tiny_string_view<CharT_>& rhs) {
        return !(lhs < rhs);
    }

#ifndef WCS_READO_FORMATTER_IMPLEMENTATION
#define WCS_READO_FORMATTER_IMPLEMENTATION 0
#endif

#if WCS_READO_FORMATTER_IMPLEMENTATION
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
    template <class T, class CharT_>
    struct formatter<wcs::data_view<T>, CharT> {
        bool quoted = false;

        template<class ParseContext>
        constexpr ParseContext::iterator parse(ParseContext& ctx) {
            auto it = ctx.begin();
            if (it == ctx.end())
                return it;
    
            if (*it == (CharT)'#')
            {
                quoted = true;
                ++it;
            }
            if (it != ctx.end() && *it != (CharT)'}')
                throw std::format_error("formatter<data_view>::parse invalid format args for data_view");
    
            return it;
        }

        template<class FmtContext>
        FmtContext::iterator format(const wcs::data_view<T>& s, FmtContext& ctx) const {
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
    template <class svCharT, class CharT_>
    struct formatter<wcs::tiny_string_view<svCharT>, CharT> {
        bool quoted = false;

        template<class ParseContext>
        constexpr ParseContext::iterator parse(ParseContext& ctx) {
            auto it = ctx.begin();
            if (it == ctx.end())
                return it;
    
            if (*it == (CharT)'#')
            {
                quoted = true;
                ++it;
            }
            if (it != ctx.end() && *it != (CharT)'}')
                throw std::format_error("formatter<tiny_string_view>::parse invalid format args for tiny_string_view");
    
            return it;
        }

        template<class FmtContext>
        FmtContext::iterator format(const wcs::tiny_string_view<svCharT><T>& s, FmtContext& ctx) const {
            std::basic_ostringstream<CharT, char_traits<CharT>, allocator<CharT>> out;
            out << s;
    
            return std::ranges::copy(std::move(out).str(), ctx.out()).out;
        }
    };
}
#endif // >= C++20
#undef WCS_CPP_STANDARD
#endif // WCS_READO_FORMATTER_IMPLEMENTATION
};
#undef WCS_READO_NODISCARD
#endif // WCS_READO_HPP_