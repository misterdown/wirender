/*  ps_window_win.hpp
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
// P.S. "ps" = PonoS
#ifndef PS_WINDOW_WIN_HPP_
#define PS_WINDOW_WIN_HPP_ 1

#ifndef PS_WINDOW_FUNCTION 
#   include <functional>
#   define PS_WINDOW_FUNCTION std::function
#endif // PS_WINDOW_FUNCTION

#ifndef PS_WINDOW_ASSERT
#   include <iostream>
#   define PS_WINDOW_ASSERT(expr__) do { if(!!(expr__) == false) { std::cerr << "Assertio fault: " << __FILE__ << ":" << __LINE__ << " " << #expr__; exit(1); } } while(false) 
#endif // PS_WINDOW_ASSERT

#ifndef PS_WINDOW_STRING_CHAR
#   include <string>
#   define PS_WINDOW_STRING_CHAR std::string
#endif // PS_WINDOW_STRING_CHAR

#ifndef PS_MOVE
#   include <utility>
#   define PS_MOVE std::move
#endif // PS_MOVE

#if __cplusplus >= 201703L
# define PS_NODISCARD [[nodiscard]]
#else
# define PS_NODISCARD
#endif

#include <windows.h>
#ifdef _MSC_VER
#   pragma comment(lib, "gdi32.lib")
#   pragma comment(lib, "user32.lib")
#endif

namespace ps_window {
    namespace details {
        static size_t windowCount = 0;
    };
    enum create_window_frag_bits {
        CREATE_WINDOW_FLAGS_BITS_RESIZABLE = WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CREATE_WINDOW_FLAGS_BITS_MENU = WS_SYSMENU,
    };
    enum key_codes {
        KEY_CODES_LBUTTON	            = 0x01,
        KEY_CODES_RBUTTON	            = 0x02,
        KEY_CODES_CANCEL	            = 0x03,
        KEY_CODES_MBUTTON	            = 0x04,
        KEY_CODES_XBUTTON1	            = 0x05,
        KEY_CODES_XBUTTON2	            = 0x06,
        KEY_CODES_BACK	                = 0x08,
        KEY_CODES_TAB	                = 0x09,
        KEY_CODES_CLEAR	                = 0x0C,
        KEY_CODES_RETURN	            = 0x0D,
        KEY_CODES_SHIFT	                = 0x10,
        KEY_CODES_CONTROL	            = 0x11,
        KEY_CODES_MENU	                = 0x12,
        KEY_CODES_PAUSE	                = 0x13,
        KEY_CODES_CAPITAL	            = 0x14,
        KEY_CODES_KANA	                = 0x15,
        KEY_CODES_HANGUL	            = 0x15,
        KEY_CODES_IME_ON	            = 0x16,
        KEY_CODES_JUNJA	                = 0x17,
        KEY_CODES_FINAL	                = 0x18,
        KEY_CODES_HANJA	                = 0x19,
        KEY_CODES_KANJI	                = 0x19,
        KEY_CODES_IME_OFF	            = 0x1A,
        KEY_CODES_ESCAPE	            = 0x1B,
        KEY_CODES_CONVERT	            = 0x1C,
        KEY_CODES_NONCONVERT	        = 0x1D,
        KEY_CODES_ACCEPT	            = 0x1E,
        KEY_CODES_MODECHANGE	        = 0x1F,
        KEY_CODES_SPACE	                = 0x20,
        KEY_CODES_PRIOR	                = 0x21,
        KEY_CODES_NEXT	                = 0x22,
        KEY_CODES_END	                = 0x23,
        KEY_CODES_HOME	                = 0x24,
        KEY_CODES_LEFT	                = 0x25,
        KEY_CODES_UP	                = 0x26,
        KEY_CODES_RIGHT	                = 0x27,
        KEY_CODES_DOWN	                = 0x28,
        KEY_CODES_SELECT	            = 0x29,
        KEY_CODES_PRINT	                = 0x2A,
        KEY_CODES_EXECUTE	            = 0x2B,
        KEY_CODES_SNAPSHOT	            = 0x2C,
        KEY_CODES_INSERT	            = 0x2D,
        KEY_CODES_DELETE	            = 0x2E,
        KEY_CODES_HELP	                = 0x2F,
        KEY_CODES_0                     = 0x30,
        KEY_CODES_1                     = 0x31,
        KEY_CODES_2                     = 0x32,
        KEY_CODES_3                     = 0x33,
        KEY_CODES_4                     = 0x34,
        KEY_CODES_5                     = 0x35,
        KEY_CODES_6                     = 0x36,
        KEY_CODES_7                     = 0x37,
        KEY_CODES_8                     = 0x38,
        KEY_CODES_9                     = 0x39,
        KEY_CODES_A                     = 0x41,
        KEY_CODES_B                     = 0x42,
        KEY_CODES_C                     = 0x43,
        KEY_CODES_D                     = 0x44,
        KEY_CODES_E                     = 0x45,
        KEY_CODES_F                     = 0x46,
        KEY_CODES_G                     = 0x47,
        KEY_CODES_H                     = 0x48,
        KEY_CODES_I                     = 0x49,
        KEY_CODES_J                     = 0x4A,
        KEY_CODES_K                     = 0x4B,
        KEY_CODES_L                     = 0x4C,
        KEY_CODES_M                     = 0x4D,
        KEY_CODES_N                     = 0x4E,
        KEY_CODES_O                     = 0x4F,
        KEY_CODES_P                     = 0x50,
        KEY_CODES_Q                     = 0x51,
        KEY_CODES_R                     = 0x52,
        KEY_CODES_S                     = 0x53,
        KEY_CODES_T                     = 0x54,
        KEY_CODES_U                     = 0x55,
        KEY_CODES_V                     = 0x56,
        KEY_CODES_W                     = 0x57,
        KEY_CODES_X                     = 0x58,
        KEY_CODES_Y                     = 0x59,
        KEY_CODES_Z                     = 0x5A,
        KEY_CODES_LWIN	                = 0x5B,
        KEY_CODES_RWIN	                = 0x5C,
        KEY_CODES_APPS	                = 0x5D,
        KEY_CODES_SLEEP	                = 0x5F,
        KEY_CODES_NUMPAD0	            = 0x60,
        KEY_CODES_NUMPAD1	            = 0x61,
        KEY_CODES_NUMPAD2	            = 0x62,
        KEY_CODES_NUMPAD3	            = 0x63,
        KEY_CODES_NUMPAD4	            = 0x64,
        KEY_CODES_NUMPAD5	            = 0x65,
        KEY_CODES_NUMPAD6	            = 0x66,
        KEY_CODES_NUMPAD7	            = 0x67,
        KEY_CODES_NUMPAD8	            = 0x68,
        KEY_CODES_NUMPAD9	            = 0x69,
        KEY_CODES_MULTIPLY	            = 0x6A,
        KEY_CODES_ADD	                = 0x6B,
        KEY_CODES_SEPARATOR	            = 0x6C,
        KEY_CODES_SUBTRACT	            = 0x6D,
        KEY_CODES_DECIMAL	            = 0x6E,
        KEY_CODES_DIVIDE	            = 0x6F,
        KEY_CODES_F1	                = 0x70,
        KEY_CODES_F2	                = 0x71,
        KEY_CODES_F3	                = 0x72,
        KEY_CODES_F4	                = 0x73,
        KEY_CODES_F5	                = 0x74,
        KEY_CODES_F6	                = 0x75,
        KEY_CODES_F7	                = 0x76,
        KEY_CODES_F8	                = 0x77,
        KEY_CODES_F9	                = 0x78,
        KEY_CODES_F10	                = 0x79,
        KEY_CODES_F11	                = 0x7A,
        KEY_CODES_F12	                = 0x7B,
        KEY_CODES_F13	                = 0x7C,
        KEY_CODES_F14	                = 0x7D,
        KEY_CODES_F15	                = 0x7E,
        KEY_CODES_F16	                = 0x7F,
        KEY_CODES_F17	                = 0x80,
        KEY_CODES_F18	                = 0x81,
        KEY_CODES_F19	                = 0x82,
        KEY_CODES_F20	                = 0x83,
        KEY_CODES_F21	                = 0x84,
        KEY_CODES_F22	                = 0x85,
        KEY_CODES_F23	                = 0x86,
        KEY_CODES_F24	                = 0x87,
        KEY_CODES_NUMLOCK	            = 0x90,
        KEY_CODES_SCROLL	            = 0x91,
        KEY_CODES_LSHIFT	            = 0xA0,
        KEY_CODES_RSHIFT	            = 0xA1,
        KEY_CODES_LCONTROL	            = 0xA2,
        KEY_CODES_RCONTROL	            = 0xA3,
        KEY_CODES_LMENU	                = 0xA4,
        KEY_CODES_RMENU	                = 0xA5,
        KEY_CODES_BROWSER_BACK	        = 0xA6,
        KEY_CODES_BROWSER_FORWARD	    = 0xA7,
        KEY_CODES_BROWSER_REFRESH	    = 0xA8,
        KEY_CODES_BROWSER_STOP	        = 0xA9,
        KEY_CODES_BROWSER_SEARCH	    = 0xAA,
        KEY_CODES_BROWSER_FAVORITES	    = 0xAB,
        KEY_CODES_BROWSER_HOME	        = 0xAC,
        KEY_CODES_VOLUME_MUTE	        = 0xAD,
        KEY_CODES_VOLUME_DOWN	        = 0xAE,
        KEY_CODES_VOLUME_UP	            = 0xAF,
        KEY_CODES_MEDIA_NEXT_TRACK	    = 0xB0,
        KEY_CODES_MEDIA_PREV_TRACK	    = 0xB1,
        KEY_CODES_MEDIA_STOP	        = 0xB2,
        KEY_CODES_MEDIA_PLAY_PAUSE	    = 0xB3,
        KEY_CODES_LAUNCH_MAIL	        = 0xB4,
        KEY_CODES_LAUNCH_MEDIA_SELECT   = 0xB5,
        KEY_CODES_LAUNCH_APP1	        = 0xB6,
        KEY_CODES_LAUNCH_APP2	        = 0xB7,
        KEY_CODES_OEM_1	                = 0xBA,
        KEY_CODES_OEM_PLUS	            = 0xBB,
        KEY_CODES_OEM_COMMA	            = 0xBC,
        KEY_CODES_OEM_MINUS	            = 0xBD,
        KEY_CODES_OEM_PERIOD	        = 0xBE,
        KEY_CODES_OEM_2	                = 0xBF,
        KEY_CODES_OEM_3	                = 0xC0,
        KEY_CODES_OEM_4	                = 0xDB,
        KEY_CODES_OEM_5	                = 0xDC,
        KEY_CODES_OEM_6	                = 0xDD,
        KEY_CODES_OEM_7	                = 0xDE,
        KEY_CODES_OEM_8	                = 0xDF,
    };
    typedef int create_window_frags;
    struct windows_handles {
        HWND hwnd{};
        HINSTANCE hInstance{};
        HICON hIcon{};
        void* iconDataPointer{nullptr};
        HBITMAP iconData{};
    };
    class default_window {
        private:
        static LRESULT WINAPI mysypurproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
            const LRESULT result = DefWindowProcA(hWnd, Msg, wParam, lParam);
            switch (Msg) {
                case WM_DESTROY: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->destroy_callback();
                    break;
                }
                case WM_KEYDOWN: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->key_down_callback(wParam);
                    break;
                }
                case WM_KEYUP: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->key_up_callback(wParam);
                    break;
                }
                case WM_LBUTTONDOWN: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->lmb_down_callback(LOWORD(lParam), HIWORD(lParam));
                    break;
                }
                case WM_LBUTTONUP: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->lmb_up_callback(LOWORD(lParam), HIWORD(lParam));
                    break;
                }
                case WM_RBUTTONDOWN: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->rmb_down_callback(LOWORD(lParam), HIWORD(lParam));
                    break;
                }
                case WM_RBUTTONUP: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->lmb_up_callback(LOWORD(lParam), HIWORD(lParam));
                    break;
                }
                case WM_MOUSEMOVE: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->mouse_move_callback(LOWORD(lParam), HIWORD(lParam));
                    break;
                }
                case WM_MOVE: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->key_move_callback(LOWORD(lParam), HIWORD(lParam));
                    break;
                }
                case WM_SIZE: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->key_resize_callback(LOWORD(lParam), HIWORD(lParam));
                    break;
                }
                case WM_MOUSEWHEEL: {
                    default_window* me = reinterpret_cast<default_window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
                    if (me == nullptr)
                        break;

                    me->mouse_wheel_callback(GET_WHEEL_DELTA_WPARAM(wParam) / 120);
                    break;
                }
            }
            return result;
        }
        private:
        windows_handles handles_;
        PS_WINDOW_STRING_CHAR name_;
        int posX_, posY_;
        int mouseX_, mouseY_;
        int width_, height_;
        int mousewheelD_;

        public:
        PS_WINDOW_FUNCTION<void(default_window*)> userDestroyCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int)> userMouseWheelCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int)> userKeyDownCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int)> userKeyUpCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int, int)> userLmbDownCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int, int)> userLmbUpCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int, int)> userRbDownCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int, int)> userRbUpCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int, int)> userMouseMoveCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int, int)> userMoveCallback;
        PS_WINDOW_FUNCTION<void(default_window*, int, int)> userResizeCallback;
        void* userPointer;

        private:
        void destroy_self_platform_spec() {
            if (handles_.hwnd != 0) {
                DestroyWindow(handles_.hwnd);
                handles_.hwnd = 0;
            }
        }
        PS_NODISCARD bool is_open_platform_spec() const {
            return handles_.hwnd != 0;
        }
        void wait_event_platform_spec() const {
            MSG msg;
            if (GetMessageA(&msg, handles_.hwnd, 0, 0) != 0) {
                DispatchMessageA(&msg);
                TranslateMessage(&msg);
            }
        }
        void pool_events_platform_spec() const {
            MSG msg;
            while (PeekMessageA(&msg, handles_.hwnd, 0, 0, PM_REMOVE)) {
                DispatchMessageA(&msg);
                TranslateMessage(&msg);
            }
        }
        void set_name_platform_spec(const char* newName) const {
            SetWindowTextA(handles_.hwnd, newName);
        }
        void show_platform_spec() {
            ShowWindow(handles_.hwnd, SW_SHOW);
        }
        void set_icon_platform_spec(int w, int h, const void* colorData, bool isrgba) {
            PS_WINDOW_ASSERT(colorData != nullptr);
            PS_WINDOW_ASSERT(w > 0);
            PS_WINDOW_ASSERT(h > 0);

            if (handles_.iconData == 0) {
                BITMAPINFO bmi{};
                    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bmi.bmiHeader.biWidth = w;
                    bmi.bmiHeader.biHeight = -h; // invert y axi;
                    bmi.bmiHeader.biPlanes = 1;
                    bmi.bmiHeader.biBitCount = 24;
                    bmi.bmiHeader.biCompression = BI_RGB;
                 // bmi.bmiHeader.biSizeImage = 0,
                 // bmi.bmiHeader.biXPelsPerMeter = 0,
                 // bmi.bmiHeader.biYPelsPerMeter = 0,
                 // bmi.bmiHeader.biClrUsed = 0,
                 // bmi.bmiHeader.biClrImportant = 0,
                 // bmi.bmiColors = {};
                handles_.iconData = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &handles_.iconDataPointer, NULL, 0);
                PS_WINDOW_ASSERT(handles_.iconData != 0);
            }
            char* bits = (char*)handles_.iconDataPointer;

            if (isrgba) {
                for (int rgbI = 0, rgbaI = 0; rgbaI < (w * h * 4); rgbaI += 4, rgbI += 3) {
                    bits[rgbI] = ((char*)colorData)[rgbaI];
                    bits[rgbI + 1] = ((char*)colorData)[rgbaI + 1];
                    bits[rgbI + 2] = ((char*)colorData)[rgbaI + 2];
                }
            } else {
                memcpy(bits, colorData, w * h * 3);
            }
            

            HBITMAP hbmMask = CreateBitmap(w, h, 1, 1, NULL);
            PS_WINDOW_ASSERT(hbmMask != 0);

            ICONINFO ii{};
            ii.fIcon = true;
         // ii.xHotspot = 0;
         // ii.yHotspot = 0;
            ii.hbmMask = hbmMask;
            ii.hbmColor = handles_.iconData;

            if (handles_.hIcon) {
                DeleteObject((HGDIOBJ)handles_.hIcon);
                handles_.hIcon = 0;
            }

            handles_.hIcon = CreateIconIndirect(&ii);
            PS_WINDOW_ASSERT(handles_.hIcon != 0);

            SendMessageA(handles_.hwnd, WM_SETICON, ICON_BIG, (LPARAM)handles_.hIcon);
            SendMessageA(handles_.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)handles_.hIcon);

            DeleteObject((HGDIOBJ)hbmMask);
        }

        public:
        default_window() = default;
        default_window(PS_WINDOW_STRING_CHAR name, create_window_frags flags = CREATE_WINDOW_FLAGS_BITS_RESIZABLE | CREATE_WINDOW_FLAGS_BITS_MENU, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int w = CW_USEDEFAULT, int h = CW_USEDEFAULT) :
                handles_{},
                name_(name),
                posX_(x), posY_(y), 
                mouseX_(0), mouseY_(0), 
                width_(w), height_(h),
                mousewheelD_(0),
                userDestroyCallback(),
                userMouseWheelCallback(),
                userKeyDownCallback(), userKeyUpCallback(),
                userLmbDownCallback(), userLmbUpCallback(),
                userRbDownCallback(), userRbUpCallback(),
                userPointer(nullptr) {
            
            handles_.hInstance = GetModuleHandleA(0);
            PS_WINDOW_ASSERT(handles_.hInstance); // WHAT

            char classNameBuffer[128]{}; // 30 + std::numeric_limits<size_t>::digits10 < 128. Everything OK.
            sprintf(classNameBuffer, "ps_window_win32_window_class%zu", details::windowCount);
            ++details::windowCount;

            WNDCLASSA wc{};
         // wc.style = 0;
            wc.lpfnWndProc = (WNDPROC)mysypurproc;
         // wc.cbClsExtra = 0;
         // wc.cbWndExtra = 0;
            wc.hInstance = handles_.hInstance;
            wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszMenuName =  "ps_window_win32_menu";
            wc.lpszClassName = classNameBuffer;

            PS_WINDOW_ASSERT(RegisterClassA(&wc));
            
            handles_.hwnd = CreateWindowA(classNameBuffer, name.c_str(), flags, x, y, w, h, 0, 0, 0, 0);
            PS_WINDOW_ASSERT(handles_.hwnd);

            SetWindowLongPtrA(handles_.hwnd, GWLP_USERDATA, (LONG_PTR)(void*)this);

        }
        default_window(const default_window&) = delete;
        default_window(default_window&& other) {
            memcpy(this, &other, sizeof(default_window));
            other.handles_.iconData = 0;
            other.handles_.hIcon = 0;
            other.handles_.hwnd = 0;
            SetWindowLongPtrA(handles_.hwnd, GWLP_USERDATA, (LONG_PTR)(void*)this); // VERY IMPORTANT
        }
        ~default_window() {
            if (handles_.iconData != 0) {
                DeleteObject(handles_.iconData);
                handles_.iconData = 0;
                handles_.iconDataPointer = nullptr;
            }
            if (handles_.hIcon != 0) {
                DeleteObject(handles_.hIcon);
                handles_.hIcon = 0;
            }
            if (handles_.hwnd != 0) {
                SendMessageA(handles_.hwnd, WM_CLOSE, 0, 0);
                handles_.hwnd = 0;
            }
        }
        default_window& operator=(const default_window&) = delete;
        default_window& operator=(default_window&& other)  {
            this->~default_window();
            new(this)default_window(PS_MOVE(other));
            return *this;
        }

        private:
        void destroy_callback() {
            if (userDestroyCallback)
                userDestroyCallback(this);
            destroy_self_platform_spec();
        }
        void key_down_callback(int key) {
            if (userKeyDownCallback)
                userKeyDownCallback(this, key);
        }
        void key_up_callback(int key) {
            if (userKeyUpCallback)
                userKeyUpCallback(this, key);
        }
        void lmb_down_callback(int x, int y) {
            mouseX_ = x;
            mouseY_ = y;
            if (userLmbDownCallback)
                userLmbDownCallback(this, x, y);
        }
        void lmb_up_callback(int x, int y) {
            mouseX_ = x;
            mouseY_ = y;
            if (userLmbUpCallback)
                userLmbUpCallback(this, x, y);
        }
        void rmb_down_callback(int x, int y) {
            mouseX_ = x;
            mouseY_ = y;
            if (userRbDownCallback)
                userRbDownCallback(this, x, y);
        }
        void rmb_up_callback(int x, int y) {
            mouseX_ = x;
            mouseY_ = y;
            if (userRbUpCallback)
                userRbUpCallback(this, x, y);
        }
        void mouse_move_callback(int x, int y) {
            mouseX_ = x;
            mouseY_ = y;
            if (userMouseMoveCallback)
                userMouseMoveCallback(this, x, y);
        }
        void key_move_callback(int x, int y) {
            posX_ = x;
            posY_ = y;
            if (userMoveCallback)
                userMoveCallback(this, x, y);
        }
        void key_resize_callback(int w, int h) {
            width_ = w;
            height_ = h;
            if (userResizeCallback)
                userResizeCallback(this, w, h);
        }
        void mouse_wheel_callback(int d) {
            mousewheelD_ = d;
            if (userMouseWheelCallback)
                userMouseWheelCallback(this, d);
        }
        void pre_event_handle_update() {
            mousewheelD_ = 0;
        }

        public:
        PS_NODISCARD const windows_handles& get_handles() {
            return handles_;
        }
        PS_NODISCARD bool is_open() const {
            return is_open_platform_spec();
        }
        void wait_event() {
            pre_event_handle_update();
            wait_event_platform_spec();
        }
        void pool_events() {
            pre_event_handle_update();
            pool_events_platform_spec();
        }
        void set_name_str(const char* newName) {
            PS_WINDOW_ASSERT(newName != nullptr);
            name_ = newName;
            set_name_platform_spec(newName);
        }
        void set_name(const PS_WINDOW_STRING_CHAR& newName) {
            name_ = newName;
            set_name_platform_spec(name_.c_str());
        }
        void set_name(PS_WINDOW_STRING_CHAR&& newName) {
            name_ = PS_MOVE(newName);
            set_name_platform_spec(name_.c_str());
        }
        /**
        * @brief Sets the window icon using raw RGB or RGBA color data.
        *
        * @param w The width of the icon.
        * @param h The height of the icon.
        * @param colorData Pointer to the raw RGB or RGBA color data.
        * @param isrgba Flag indicating whether the color data is in RGBA format. `true` by default.
        *
        * @note If you set icon 4992 times you will die.
        *       The function assumes that the color data is in the correct format
        *       and size. For RGB format, the size should be w * h * 3 bytes.
        *       For RGBA format, the size should be w * h * 4 bytes.
        */
        void set_icon(int w, int h, const void* colorData, bool isrgba = true) {
            set_icon_platform_spec(w, h, colorData, isrgba);
        }
        PS_NODISCARD const PS_WINDOW_STRING_CHAR& get_name() const {
            return name_;
        }
        PS_NODISCARD int get_mouse_x() const noexcept {
            return mouseX_;
        }
        PS_NODISCARD int get_mouse_y() const noexcept {
            return mouseY_;
        }
        PS_NODISCARD int get_width() const noexcept {
            return width_;
        }
        PS_NODISCARD int get_heigth() const noexcept {
            return height_;
        }
        PS_NODISCARD int get_x() const noexcept {
            return posX_;
        }
        PS_NODISCARD int get_y() const noexcept {
            return posY_;
        }
        PS_NODISCARD int get_mouse_wheel_scroll_delta() const noexcept { // -1 - down 0 - calm 1 - up
            return mousewheelD_;
        }
        void show() {
            show_platform_spec();
        }
    };
};
#undef PS_NODISCARD
#endif// ifndef PS_WINDOW_WIN_HPP_