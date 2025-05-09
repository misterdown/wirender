#include <wiender.hpp>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>

using namespace wiender;
using vertex_input_attribute = shader::vertex_input_attribute;
using stage = shader::stage;
using primitive_topology = shader::primitive_topology;
using polygon_mode = shader::polygon_mode;
using cull_mode = shader::cull_mode;
using vec2 = float[2];
struct vertex {
    vec2 pos;
    vec2 uv;
};

namespace {
    bool windowIsOpen;
}

std::vector<uint32_t> read_binary_file(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open file: " + filePath);
    }

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize % 4 != 0) {
        throw std::runtime_error("File size is not a multiple of 4");
    }

    std::vector<uint32_t> data(fileSize / 4);

    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    if (!file) {
        throw std::runtime_error("Error reading file");
    }

    return data;
}

HWND create_window(HINSTANCE hInstance) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
        switch (message) {
            case WM_DESTROY:
                PostQuitMessage(0);
                windowIsOpen = false;
                return 0;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
    };
    wc.hInstance = hInstance;
    wc.lpszClassName = "wienderWindowClass";
    RegisterClass(&wc);

    return CreateWindow(wc.lpszClassName, "wiender Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                       0, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);
}


void fill_buffer(buffer* bf) {
    vertex* verteces = (vertex*)bf->map();

    verteces[0].pos[0] = -1.0f, verteces[0].pos[1] = -1.0f;
    verteces[1].pos[0] = -1.0f, verteces[1].pos[1] = 1.0f;
    verteces[2].pos[0] = 1.0f,  verteces[2].pos[1] = 1.0f;

    verteces[0].uv[0] = 0.0f,   verteces[0].uv[1] = 0.0f;
    verteces[1].uv[0] = 0.0f,   verteces[1].uv[1] = 1.0f;
    verteces[2].uv[0] = 1.0f,   verteces[2].uv[1] = 1.0f;

    bf->update_data();
    bf->unmap();
}
void fill_texture(texture* tetr) {
    const texture::extent ext = tetr->get_extent();

    uint32_t* data = (uint32_t*)tetr->map();
        for (size_t x = 0; x < ext.width; ++x) {
            for (size_t y = 0; y < ext.height; ++y) {
                uint8_t* color = (uint8_t*)&data[x + y * 10];
                color[0] = (x + 5) * 32;
                color[1] = (x + 5) * 16;
                color[2] = (x + 5) * 4;
                color[3] = (x + 5);
            }
        }
    tetr->update_data();
    tetr->unmap();
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    HWND hWnd = create_window(hInstance);
    
    if (hWnd == nullptr) {
        std::cerr << "test failed on creating window" << '\n';
        return 1;
    }
    std::cout << "window created\n";
    
    try {
        auto wienderer = create_wienderer(backend_type::VULKAN, windows_window_handle(hWnd, hInstance));
        std::cout << "wienderer created\n";

        auto vertexgpub = wienderer->create_buffer(buffer::type::GPU_SIDE_VERTEX, sizeof(vertex) * 128);
        std::cout << "vertexgpub created\n";

        fill_buffer(vertexgpub.get());
        vertexgpub->bind();
        
        auto shader = wienderer->create_shader(
            shader::create_info(
                std::vector<stage> {
                    stage(stage::kind::VERTEX, read_binary_file("assets/texturev.spirv")),
                    stage(stage::kind::FRAGMENT, read_binary_file("assets/texturef.spirv"))
                },
                std::vector<vertex_input_attribute>{
                    vertex_input_attribute(vertex_input_attribute::format::FLOAT_VEC2, 0, 0, 0),
                    vertex_input_attribute(vertex_input_attribute::format::FLOAT_VEC2, 1, offsetof(vertex, vertex::uv), 0)
                },
                primitive_topology::TRIANGLES_LIST,
                polygon_mode::FILL,
                cull_mode::NONE,
                true,
                false
            )
        );
        std::cout << "shader created\n";
        
        vec2* g = (vec2*)shader->get_uniform_buffer_info(0).data;
        g[0][0] = 0.0f, g[0][1] = 0.0f;
        g[1][0] = 1.0f, g[1][1] = 1.0f;
        std::cout << "uniform buffer filled\n";
        
        shader->set();
        auto tetrn = wienderer->create_texture(
            texture::create_info(
                texture::sampler_filter::NEAREST,
                texture::extent(10, 10)
            )
        );
        std::cout << "nearest texture created\n";
        
        auto tetrl = wienderer->create_texture(
            texture::create_info(
                texture::sampler_filter::LINEAR,
                texture::extent(10, 10)
            )
        );
        std::cout << "linear texture created\n";
        shader->bind_texture(1, 0, tetrl.get());

        fill_texture(tetrn.get());
        fill_texture(tetrl.get());
        std::cout << "textures filled\n";

        shader->bind_texture(1, 0, tetrl.get());

        wienderer->begin_record();
        wienderer->begin_render();
        wienderer->draw_verteces(3, 0, 1);
        wienderer->end_render();
        wienderer->end_record();
        std::cout << "recording was begun and ended\n";

        MSG msg;
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        double maxDeltaMs = 0.0;
        double sf = 0.0;
        double sch = 0.0;
        long frame = 0;
        
        windowIsOpen = true;
        ShowWindow(hWnd, SW_SHOW);
        std::cout << "hello!\n";

        bool linear = true;
        while (windowIsOpen) {
            auto start = std::chrono::high_resolution_clock::now();

            PeekMessageA(&msg, hWnd, 0, 0, PM_REMOVE);
            DispatchMessageA(&msg);
            wienderer->execute();

            if (sch >= 500.0f) {
                linear = !linear;
                if (linear) {
                    shader->bind_texture(1, 0, tetrl.get());
                } else {
                    shader->bind_texture(1, 0, tetrn.get());
                }
                sch = 0.0f;
            }

            std::chrono::duration<double, std::milli> delta = start - lastFrameTime;
            sf += delta.count();
            sch += delta.count();
            maxDeltaMs = std::max(maxDeltaMs, delta.count());
            g[0][0] = cos(sf * 3.1415 * 2.0f / 1000.0f) * 0.5f, g[0][1] = sin(sf * 3.1415 * 2.0f / 1000.0f) * 0.5f;
            lastFrameTime = start;
            ++frame;
            if (sf >= 1000.0f) {
                std::cout   << "max delta ms: " << maxDeltaMs << "\tactual fps: " << (double)frame << "\n\n";
                sf = 0.0;
                maxDeltaMs = 0.0;
                frame = 0;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "test failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "passed\n";
    return 0;
}