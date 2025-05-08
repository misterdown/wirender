#ifndef WIENDER_CORE_HPP_
#define WIENDER_CORE_HPP_ 1
#include <exception>
#include <cstddef>
#include <memory>
#include <vector>

#if __cplusplus >= 201703L
# define WIENDER_NODISCARD [[nodiscard]]
#else
# define WIENDER_NODISCARD
#endif

namespace wiender {
    struct window_handle {
        public:
        virtual ~window_handle() {}

        WIENDER_NODISCARD virtual void* get_window_handle() const = 0;
        WIENDER_NODISCARD virtual void* get_display_handle() const = 0;
    };

    // segment: Buffers
    struct buffer { 
        public:
        enum struct type : int {
            SSO,
            CPU_SIDE_VERTEX,
            GPU_SIDE_VERTEX,
            CPU_SIDE_INDEX,
            GPU_SIDE_INDEX,
        };

        public:
        virtual ~buffer() {}

        public:
        WIENDER_NODISCARD virtual bool is_mapped() const noexcept = 0;
        virtual void bind() = 0;
        WIENDER_NODISCARD virtual void* map() = 0;
        virtual void unmap() = 0;
        virtual void update_data() = 0;

    };

    // segment: Textures

    struct texture {
        public:
        struct extent {
            public:
            uint32_t width;
            uint32_t height;    // 0 for 1d image
            uint32_t depth;     // 0 for 2d image

            public:
            extent() : width(1), height(1), depth(0) {

            }
            extent(uint32_t width) : width(width), height(0), depth(0) {

            }
            extent(uint32_t width, uint32_t height, uint32_t depth = 0) : width(width), height(height), depth(depth) {

            }
        };
        enum struct sampler_filter {
            NEAREST,
            LINEAR,
        };
        struct create_info {
            public:
            sampler_filter filter;
            extent textureExtent; 

            public:
            create_info() : filter(sampler_filter::LINEAR), textureExtent() {

            }
            create_info(sampler_filter filter, const extent& textureExtent) : filter(filter), textureExtent(textureExtent) {

            }
        };

        public:
        virtual ~texture() {}

        public:
        WIENDER_NODISCARD virtual extent get_extent() const noexcept = 0;
        WIENDER_NODISCARD virtual bool is_mapped() const noexcept = 0;
        WIENDER_NODISCARD virtual void* map() = 0;
        virtual void unmap() = 0;
        virtual void update_data() = 0;
        

    };

    // segment: Shaders
    struct shader {
        public:
        struct uniform_buffer_info {
            public:
            std::size_t size;
            void* data;

        };

        public:
        struct stage {
            enum struct kind {
                VERTEX,
                FRAGMENT,
                COMPUTE,
            } stageKind;
            std::vector<uint32_t> data;
            public:
            stage() : stageKind(kind::VERTEX), data() {}
            stage(kind stageKind, const std::vector<uint32_t>& data) : stageKind(stageKind), data(data) {}
            stage(kind stageKind, const uint32_t* dataPtr, std::size_t dataCount) : stageKind(stageKind), data(dataPtr, dataPtr + dataCount) {}
        };
        struct vertex_input_attribute {
            public:
            enum struct format {
                FLOAT_SCALAR,
                FLOAT_VEC2,
                FLOAT_VEC3,
                FLOAT_VEC4,
            } inputFormat;
            uint32_t location;
            uint32_t binding;
            uint32_t offset;

            public:
            vertex_input_attribute() : inputFormat(format::FLOAT_SCALAR), location(0), binding(0), offset(0) {}

            // Конструктор с параметрами
            vertex_input_attribute(format inputFormat, uint32_t location, uint32_t offset, uint32_t binding) 
                :   inputFormat(inputFormat),
                    location(location),
                    binding(binding),
                    offset(offset) {} 

        };
        enum struct primitive_topology {
            TRIANGLES_LIST,
            TRIANGLES_FAN,
            POINTS,
            LINES,
        };
        enum struct polygon_mode {
            FILL,
            LINE,
            POINT,
        };
        enum struct cull_mode {
            NONE,
            BACK,
            FRONT,
            ALL
        };
        struct create_info {
            std::vector<stage> stages;
            std::vector<vertex_input_attribute> vertexInputAttributes;  // for graphics shaders only
            primitive_topology topology;                                // for graphics shaders only
            polygon_mode polygonMode;                                   // for graphics shaders only
            cull_mode cullMode;                                         // for graphics shaders only
            bool clearScreen;                                           // for graphics shaders only
            bool alphaBlend;                                            // for graphics shaders only

            public:
            create_info()
                : topology(primitive_topology::TRIANGLES_LIST),
                polygonMode(polygon_mode::FILL),
                cullMode(cull_mode::BACK),
                clearScreen(false),
                alphaBlend(false) {}

            create_info(const std::vector<stage>& stages,
                        const std::vector<vertex_input_attribute>& vertexInputAttributes,
                        primitive_topology topology,
                        polygon_mode polygonMode,
                        cull_mode cullMode,
                        bool clearScreen,
                        bool alphaBlend)
                :   stages(stages),
                    vertexInputAttributes(vertexInputAttributes),
                    topology(topology),
                    polygonMode(polygonMode),
                    cullMode(cullMode),
                    clearScreen(clearScreen),
                    alphaBlend(alphaBlend) {}

            create_info(const std::vector<stage>& stages)
                :   stages(stages),
                    topology(primitive_topology::TRIANGLES_LIST),   // default
                    polygonMode(polygon_mode::FILL),                // default
                    cullMode(cull_mode::BACK),                      // default
                    clearScreen(false),                                // default
                    alphaBlend(false) {}                            // default
        };

        public:
        virtual ~shader() {}

        public:
        virtual void set() = 0;
        WIENDER_NODISCARD virtual uniform_buffer_info get_uniform_buffer_info(std::size_t binding) = 0;
        virtual void bind_texture(std::size_t binding, std::size_t arrayIndex, const texture* tetr) = 0;
    };

    // segment: Wenderer
    /**
     * @brief A placeholder structure, an abstract class without an interface.
     *
     * This structure is used to allow the user to save commands recorded in a wienderer
     * and re-record them into the same wienderer.
     */
    struct wiender_commands_frame {
        public:
        virtual ~wiender_commands_frame() {}
    };
    // I HATE p******ism
    class wienderer {
        public:
        virtual ~wienderer() {}

        public:
        WIENDER_NODISCARD virtual std::unique_ptr<buffer> create_buffer(buffer::type type, std::size_t byte) = 0;
        WIENDER_NODISCARD virtual std::unique_ptr<shader> create_shader(const shader::create_info& createInfo) = 0;
        WIENDER_NODISCARD virtual std::unique_ptr<texture> create_texture(const texture::create_info& createInfo) = 0;
        WIENDER_NODISCARD virtual std::unique_ptr<texture> get_postproc_texture() = 0;
        WIENDER_NODISCARD virtual std::unique_ptr<wiender_commands_frame> get_commands_frame() const = 0;
        virtual void clear_commands_frame() = 0;
        virtual void set_commands_frame(const wiender_commands_frame* frame) = 0;
        virtual void concat_commands_frame(const wiender_commands_frame* frame) = 0;
        virtual void begin_record() = 0;
        virtual void begin_render() = 0;
        virtual void draw_verteces(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount) = 0;
        virtual void draw_indexed(uint32_t indecesCount, uint32_t firstIndex, uint32_t instanceCount) = 0;
        virtual void end_render() = 0;
        virtual void end_record() = 0;
        virtual void execute() = 0;
        virtual void wait_executing() = 0;
    };

} // namespace wiender

#endif // WIENDER_CORE_HPP_