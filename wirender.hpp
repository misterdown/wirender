/*  wirender.hpp
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
#include <vulkan/vulkan.h>
#if ((defined __WIN32) || (defined _WIN32) || (defined _WIN32_) || (defined __WIN32__))
#   include <windows.h>
#endif

#define RENDER_VK_INVALID_FAMILY_INDEX ~0u
#define RENDER_DEFAULT_MAX_COUNT 16
#define RENDER_UNIFORM_BUFFER_MAX_COUNT RENDER_DEFAULT_MAX_COUNT
#define RENDER_SAMPLED_IMAGE_MAX_COUNT RENDER_DEFAULT_MAX_COUNT
#define RENDER_DYNAMIC_STATE_MAX_COUNT RENDER_DEFAULT_MAX_COUNT
#define RENDER_INPUT_ATTRIBUTE_MAX_COUNT RENDER_DEFAULT_MAX_COUNT
#define RENDER_COMMAND_MAX_COUNT 256
#define RENDER_STAGE_MAX_COUNT RENDER_DEFAULT_MAX_COUNT
#define RENDER_SWAPCHAIN_IMAGE_MAX_COUNT 8
#define RENDER_DESCRIPTOR_MAX_COUNT (RENDER_UNIFORM_BUFFER_MAX_COUNT + RENDER_SAMPLED_IMAGE_MAX_COUNT)
#define RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT (RENDER_DESCRIPTOR_MAX_COUNT * 2)

static_assert(RENDER_STAGE_MAX_COUNT >= 2, "RENDER_STAGE_MAX_COUNT must be more or equals 2");

namespace wirender {
    namespace RenderVulkanUtils {
        struct swapchain_images {
            uint32_t imageCount;
            VkImage images[RENDER_SWAPCHAIN_IMAGE_MAX_COUNT];
            VkImageView views[RENDER_SWAPCHAIN_IMAGE_MAX_COUNT];
            VkFramebuffer framebuffers[RENDER_SWAPCHAIN_IMAGE_MAX_COUNT];
        };
        struct swapchain_support_info {
            uint32_t imageCount;
            VkSurfaceFormatKHR imageFormat;
            VkSurfaceFormatKHR surfaceFormat;
            VkPresentModeKHR presentMode;
            VkExtent2D extent;
            VkSurfaceCapabilitiesKHR capabilities;
        };
        struct queue_family_indices {
            public:
            union {
                struct {
                    uint32_t graphicsFamily;
                    uint32_t presentFamily;
                };
                uint32_t indeces[2]{RENDER_VK_INVALID_FAMILY_INDEX, RENDER_VK_INVALID_FAMILY_INDEX};
            };
            uint32_t falimiesCount;

            [[nodiscard]] bool is_complete() const {
                return (graphicsFamily != RENDER_VK_INVALID_FAMILY_INDEX) && (presentFamily != RENDER_VK_INVALID_FAMILY_INDEX);
            }
        };
        struct physical_device_info {
            public:
            VkPhysicalDevice device;
            VkPhysicalDeviceFeatures2 features;
            VkPhysicalDeviceProperties2 properties;
            queue_family_indices queueIndeces;

            public:
            operator const VkPhysicalDevice& () const {
                return device;
            }
            operator VkPhysicalDevice& () {
                return device;
            }
        };
        struct logical_device_info {
            public:
            VkDevice device;
            VkQueue graphicsQueue;
            VkQueue presentQueue;

            public:
            operator const VkDevice& () const {
                return device;
            }
            operator VkDevice& () {
                return device;
            }
        };
        struct active_shader_state {
            VkPipeline pipeline;
            VkPipelineLayout layout;
            VkRenderPass renderPass;
            VkDescriptorSet descriptorSet;
        };
        struct binded_buffer_state {
            VkBuffer buffer;
        };
        struct graphics_buffer {
            VkBuffer buffer;
            VkDeviceMemory memory;
        };
        struct uniform_buffers_info {
            VkDeviceMemory memory; // single memory block for each buffer but with different offsets
            struct {
                VkBuffer buffer;
                VkDeviceSize size;
            } buffers[RENDER_UNIFORM_BUFFER_MAX_COUNT]{};
            void* mappedMemory = nullptr;
        };
        struct sync_object {
            VkFence fence;
            VkSemaphore semaphore;
        };
        struct public_spirv_variable_declaration {
            uint32_t binding;
            uint32_t descriptorSet;
            uint32_t size;
            uint32_t type;
            VkShaderStageFlagBits stageFlags;
        };
    };
    struct shader_stage final {
        uint32_t codeSize; // in bytes
        const uint32_t* code;
        VkShaderStageFlagBits stage;
    };

    #if ((defined __WIN32) || (defined _WIN32) || (defined _WIN32_) || (defined __WIN32__))
    struct window_info final {
        HWND hwnd;
        HINSTANCE hInstance;
    };
    #endif // defined __WIN32
    // | null | - nothing.
    // | [word1, word2] | - member name in render_command::data

    enum render_command_type {
        RENDER_COMMAND_TYPE_CLEAR_COMMAND_BUFFERS, // data: null 
        RENDER_COMMAND_TYPE_SET_SHADER,            // data: [ activeShaderState ]
        RENDER_COMMAND_TYPE_BIND_BUFFER,           // data: [ bindedBufferState ]
        RENDER_COMMAND_TYPE_START_RECORD,          // data: null
        RENDER_COMMAND_TYPE_RECORD_UPDATE_SCISSOR, // data: null
        RENDER_COMMAND_TYPE_RECORD_UPDATE_VIEWPORT,// data: null
        RENDER_COMMAND_TYPE_RECORD_START_RENDER,   // data: null
        RENDER_COMMAND_TYPE_RECORD_DRAW_VERTECES,  // data: [ drawData[0..2] ]
        RENDER_COMMAND_TYPE_RECORD_DRAW_INDEXED,   // data: [ drawData[0..2] ]
        RENDER_COMMAND_TYPE_RECORD_END_RENDER,     // data: null
        RENDER_COMMAND_TYPE_END_RECORD,            // data: null
    };
    struct render_command {
        render_command_type commandType;
        union {
            RenderVulkanUtils::active_shader_state activeShaderState;
            RenderVulkanUtils::binded_buffer_state bindedBufferState;
            uint32_t drawData[3];
        } data;
    };
    struct render_commands {
        render_command commands[RENDER_COMMAND_MAX_COUNT];
        uint32_t count;
    };

    struct render_manager final {
        public:
        friend struct shader;
        friend struct graphics_buffer;
        friend struct buffer_host_mapped_memory;

        private:
        window_info windowInfo;
        VkInstance vulkanInstance;
        VkDebugUtilsMessengerEXT debugMessenger;
        RenderVulkanUtils::physical_device_info physicalDevice;
        RenderVulkanUtils::logical_device_info logicalDevice;
        VkSurfaceKHR surface;
        RenderVulkanUtils::swapchain_support_info swapchainSupportInfo;
        VkRenderPass defaultRenderPass;
        VkSwapchainKHR swapchain;
        RenderVulkanUtils::swapchain_images swapchainImages;
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffers[RENDER_SWAPCHAIN_IMAGE_MAX_COUNT]; // one buffer on each image
        RenderVulkanUtils::sync_object syncObject;
        RenderVulkanUtils::active_shader_state currentShader;
        RenderVulkanUtils::binded_buffer_state bindedBuffer;
        uint32_t imageIndex;
        render_commands appliedCommands;
        bool validationEnable;

        public:
        render_manager() = delete;
        render_manager(const window_info&, bool enableValidation = false);
        render_manager(const render_manager&) = delete;
        render_manager(render_manager&&);

        public:
        ~render_manager();

        public:
        render_manager& set_commands(const render_commands& commands);
        render_manager& clear_command_buffers();
        render_manager& set_shader(const RenderVulkanUtils::active_shader_state& shaderState);
        render_manager& bind_buffer(const RenderVulkanUtils::binded_buffer_state& bufferState);
        render_manager& wait_executing();
        render_manager& start_record();
        render_manager& record_update_viewport();
        render_manager& record_update_scissor();
        render_manager& record_start_render();
        render_manager& record_draw_verteces(uint32_t vertexCount, uint32_t offset, uint32_t instanceCount);
        render_manager& record_draw_indexed(uint32_t indexCount, uint32_t offset, uint32_t instanceCount);
        render_manager& record_end_render();
        render_manager& end_record();
        render_manager& execute();
        render_manager& resize();

        private:
        void set_members_zero();

        private:
        [[nodiscard]] VkInstance create_vulkan_instance() const;
        [[nodiscard]] VkDebugUtilsMessengerEXT create_debug_messenger() const;
        [[nodiscard]] RenderVulkanUtils::physical_device_info get_physical_device() const;
        [[nodiscard]] VkSurfaceKHR create_surface() const;
        [[nodiscard]] RenderVulkanUtils::queue_family_indices create_falimy_indices() const;
        [[nodiscard]] RenderVulkanUtils::logical_device_info create_logical_device() const;
        [[nodiscard]] RenderVulkanUtils::swapchain_support_info create_swapchain_info() const;
        [[nodiscard]] VkSwapchainKHR create_swapchain() const;
        void initialize_swapchain_images(RenderVulkanUtils::swapchain_images&) const;
        [[nodiscard]] const render_commands& get_applied_commands() const noexcept;
        [[nodiscard]] VkCommandPool create_command_pool() const;
        void allocate_command_buffers(VkCommandBuffer[RENDER_SWAPCHAIN_IMAGE_MAX_COUNT]) const;
        void destroy_swapchain_images(RenderVulkanUtils::swapchain_images&) const;
        void intitialize_sync_object(RenderVulkanUtils::sync_object&) const;
    };
    struct shader_create_info {
        shader_stage stages[RENDER_STAGE_MAX_COUNT]{};
        uint32_t stageCount{0};
        VkDynamicState dynamicStates[RENDER_DYNAMIC_STATE_MAX_COUNT]{VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
        uint32_t vertexInputAttributeCount{0};
        VkVertexInputAttributeDescription vertexInputAtributes[RENDER_INPUT_ATTRIBUTE_MAX_COUNT]{};
        VkPrimitiveTopology primitiveTopology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        uint32_t dynamicStateCount{2};
        VkPolygonMode polygonMode{VK_POLYGON_MODE_FILL};
        VkSampleCountFlagBits rasterizationSampleCount{VK_SAMPLE_COUNT_1_BIT};
        VkCullModeFlagBits cullMode{VK_CULL_MODE_NONE};
        float lineWidth{1.0f};
        bool clearScreen = false;
    };
    struct shader final {
        private:
        render_manager* owner;
        RenderVulkanUtils::uniform_buffers_info uniformBuffers;
        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;

        public:
        shader() = delete;
        shader(render_manager*, const shader_create_info&);

        public:
        ~shader();

        public:
        [[nodiscard]] RenderVulkanUtils::active_shader_state get_state() const;
        [[nodiscard]] void* get_uniform_buffer_memory_on_binding(uint32_t);

        private:
        void set_members_zero();

        private:
        [[nodiscard]] VkDescriptorPool create_descriptor_pool(const shader_create_info&, const RenderVulkanUtils::public_spirv_variable_declaration[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t) const;
        [[nodiscard]] RenderVulkanUtils::uniform_buffers_info create_uniform_buffers(const shader_create_info&, const RenderVulkanUtils::public_spirv_variable_declaration[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t) const;
        [[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(const shader_create_info&, const RenderVulkanUtils::public_spirv_variable_declaration[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t);
        [[nodiscard]] VkDescriptorSet create_descriptor_set(const shader_create_info&, const RenderVulkanUtils::public_spirv_variable_declaration[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t);
        [[nodiscard]] VkRenderPass create_render_pass(const shader_create_info&) const;
        [[nodiscard]] VkPipelineLayout create_pipeline_layout() const;
        [[nodiscard]] VkPipeline create_pipeline(const shader_create_info&) const;
    };
    struct shader_builder final {
        private:
        render_manager* owner;
        shader_create_info createInfo;

        public:
        shader_builder() = delete;
        shader_builder(render_manager*);

        public:
        shader_builder& add_stage(const shader_stage&);
        shader_builder& add_vertex_input_attribute(const VkVertexInputAttributeDescription&);
        shader_builder& add_dynamic_state(VkDynamicState);
        shader_builder& pop_dynamic_state();
        shader_builder& set_primitive_topology(VkPrimitiveTopology);
        shader_builder& set_line_width(float);
        shader_builder& set_polygon_mode(VkPolygonMode);
        shader_builder& set_rasterization_sample_count(VkSampleCountFlagBits);
        shader_builder& set_cull_mode(VkCullModeFlagBits);
        shader_builder& set_clear_screen(bool);
        [[nodiscard]] shader build() const;
    };
    struct buffer_create_info {
        VkDeviceSize size;// in bytes
        VkBufferUsageFlags usage;
    };
    struct buffer_host_mapped_memory {
        private:
        render_manager* owner;
        VkDeviceSize size;
        RenderVulkanUtils::graphics_buffer buffer;
        VkBufferUsageFlags usage;
        bool mapped;

        public:
        buffer_host_mapped_memory() = delete;
        buffer_host_mapped_memory(render_manager*, const buffer_create_info&);

        public:
        ~buffer_host_mapped_memory();

        public:
        void* map_memory();
        void unmap_memory();
        [[nodiscard]] RenderVulkanUtils::binded_buffer_state get_state() const;

        private:
        void set_members_zero();
    };
    //struct buffer_device_memory {
    //    render_manager* owner;
    //    VkDeviceSize size;
    //    RenderVulkanUtils::graphics_buffer mainBuffer;
    //    RenderVulkanUtils::graphics_buffer stageBuffer;
    //    VkBufferUsageFlags usage;
    //    bool stageMapped;
    //    . . .
    //};
    struct buffer_builder {
        private:
        render_manager* owner;
        buffer_create_info createInfo;

        public:
        buffer_builder() = delete;
        buffer_builder(render_manager*);

        public:
        buffer_builder& set_size(VkDeviceSize);
        buffer_builder& set_usage(VkBufferUsageFlags);
        [[nodiscard]] buffer_host_mapped_memory build_host_mapped_memory_buffer() const;
        //[[nodiscard]] buffer_host_mapped_memory buffer_device_memory() const;
    };
};