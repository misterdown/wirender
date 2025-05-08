#ifndef WIENDER_MAC_OS_HPP_
#define WIENDER_MAC_OS_HPP_ 1

#include "wiender_core.hpp"

#include <Cocoa/Cocoa.h>

namespace wiender {
    struct mac_os_window_handle final : public window_handle {
        private:
        NSView* view_;

        public:
        mac_os_window_handle(NSView* view): view_(view) {

        }

        WIENDER_NODISCARD void* get_window_handle() const override {
            return view_;
        }

        WIENDER_NODISCARD void* get_display_handle() const override {
            return nullptr;
        }
    };
} // namespace wiender

#endif // WIENDER_WINDOWS_HPP_