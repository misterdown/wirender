#ifndef WIENDER_WINDOWS_HPP_
#define WIENDER_WINDOWS_HPP_ 1

#include "wiender_core.hpp"

#include <windows.h>

namespace wiender {
    struct windows_window_handle final : public window_handle {
        private:
        HWND hwnd_;
        HINSTANCE hinstance_;

        public:
        windows_window_handle(HWND hwnd, HINSTANCE hinstance)
            : hwnd_(hwnd), hinstance_(hinstance) {}

        WIENDER_NODISCARD void* get_window_handle() const override {
            return hwnd_;
        }

        WIENDER_NODISCARD void* get_display_handle() const override {
            return hinstance_;
        }
    };
} // namespace wiender

#endif // WIENDER_WINDOWS_HPP_