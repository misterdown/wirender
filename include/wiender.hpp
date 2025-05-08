#ifndef WIENDER_HPP_
#define WIENDER_HPP_ 1

#ifdef _WIN32
#include "wiender_windows.hpp"
#elif defined(__linux__)

#ifdef USE_X11
#include "wiender_x11.hpp"
#endif // USE_X11

#ifdef USE_WAYLAND
#include "wiender_wayland.hpp"
#endif // USE_WAYLAND

#elif defined(__APPLE__)
#include "wiender_mac_os.hpp"
#endif

namespace wiender {

    enum struct backend_type {
        VULKAN
        // ... OPENGL, DIRECTX(probably no), METAL
    };
    /** 
     * @brief Creates a wienderer instance for the specified backend type.
     * This function initializes and returns a unique pointer to a wienderer instance
     * based on the specified backend type. The wienderer instance is responsible for
     * managing graphics resources and rendering operations.
     * 
     * @param backendType The type of backend to use for the wienderer instance.
     * @throw std::exeption If the creation of the wienderer instance fails.
     * @return A unique pointer to the created wienderer instance.
     */ 
    std::unique_ptr<wienderer> create_wienderer(backend_type backendType, const window_handle& whandle);
} // namespace wiender

#endif