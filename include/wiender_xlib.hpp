#ifndef WIENDER_XLIB_HPP_
#define WIENDER_XLIB_HPP_ 1

#include "wiender_core.hpp"

#include <X11/Xlib.h>

namespace wiender {
    struct Xlib_window_handler final : public window_handle {
        private:
        Display* display_;
        Window window_;

        public:
        Xlib_window_handler(Display* display, Window window)
            : display(display_), window(window_) {}

        WIENDER_NODISCARD void* get_window_handle() const override {
            return (void*)window_;
        }

        WIENDER_NODISCARD void* get_display_handle() const override {
            return (void*)display_;
        }
    };
} // namespace wiender

#endif // WIENDER_XLIB_HPP_