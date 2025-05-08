#include <memory>

#include "vulkan_implement/vulkan_wiender.hpp"
#include "../include/wiender.hpp"

std::unique_ptr<wiender::wienderer> wiender::create_wienderer(wiender::backend_type backendType, const wiender::window_handle& whandle) {
    switch (backendType) {
        case wiender::backend_type::VULKAN: {
            return std::unique_ptr<vulkan_wienderer>(new vulkan_wienderer(whandle));
        } default: {
            throw std::runtime_error("wiender::create_wienderer unknown backend_type");
        }
    }
    // unreachable
}
