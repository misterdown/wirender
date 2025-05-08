#ifndef WIENDER_IMPLEMENT_CORE_HPP_
#define WIENDER_IMPLEMENT_CORE_HPP_ 1

#include <pickmelib/reado.hpp>
#include <stdexcept>

namespace wiender {
    void wiender_assert(bool v, const wcs::tiny_string_view<char>& strv) {
        if (!v) {
            throw std::runtime_error(strv.c_str());
        }
    } 
} // namespace wiender

#endif // WIENDER_IMPLEMENT_CORE_HPP_