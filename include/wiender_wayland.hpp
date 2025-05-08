#ifndef WIENDER_WAYLAND_HPP_
#define WIENDER_WAYLAND_HPP_ 1

#include "wiender_core.hpp"

#include <wayland-client.h>

namespace wiender {
    struct wayland_window_handler final : public window_handle {
        private:
        wl_display* display_;
        wl_surface* surface_;

        public:
        Xlib_window_handler(wl_display* display, wl_surface* surface)
            : display(display_), surface_(surface) {}

        WIENDER_NODISCARD void* get_window_handle() const override {
            return (void*)surface_;
        }

        WIENDER_NODISCARD void* get_display_handle() const override {
            return (void*)display_;
        }
    };
} // namespace wiender

#endif // WIENDER_WAYLAND_HPP_