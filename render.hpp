
#include <vulkan/vulkan.h>
#if (defined __WIN32)
#include <windows.h>
#else
#include <X11/Xlib.h>
//#include <X11/Xlib-xcb.h>
//  use XLib for creating surface
//#   error "NOT SUPPORTED"
#endif

#define RENDER_VK_INVALID_FAMILY_INDEX ~0u
#define RENDER_DEFAULT_MAX_VALUE 16
#define RENDER_SWAPCHAIN_IMAGE_MAX_COUNT 8
#define RENDER_DESCRIPTORS_MAX_VALUE 16
namespace render {
    //using namespace render::platform; LOL

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
            VkRenderPass renderPass;
            uint32_t descriptorSetCount;
            VkDescriptorSet* descriptorSets;
        };
        struct binded_buffer_state {
            VkBuffer buffer;
        };
        struct graphics_buffer {
            VkBuffer buffer;
            VkDeviceMemory memory;
        };
        struct sync_object {
            VkFence fence;
            VkSemaphore semaphore;
        };
    };
    struct shader_stage final {
        uint32_t codeSize; // in bytes
        const uint32_t* code;
        VkShaderStageFlagBits stage;
    };

    #if (defined __WIN32)
    struct window_info final {
        HWND hwnd;
        HINSTANCE hInstance;
    };
    #else
    struct window_info final {
        Display* dpy;
        Window window;
    };
    #endif // defined __WIN32

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
        bool validationEnable;

        public:
        render_manager() = delete;
        render_manager(const window_info&);
        render_manager(const render_manager&) = delete;
        render_manager(render_manager&&);

        public:
        ~render_manager();

        public:
        render_manager& clear_command_list();
        render_manager& set_shader(const RenderVulkanUtils::active_shader_state& shaderState);
        render_manager& bind_buffer(const RenderVulkanUtils::binded_buffer_state& bufferState);
        render_manager& wait_executing();
        render_manager& start_record();
        //render_manager& record_set_topology(VkPrimitiveTopology); // not supported yet
        //render_manager& record_set_polygon_move(VkPolygonMode);   // not supported yet
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
        [[nodiscard]] VkCommandPool create_command_pool() const;
        void allocate_command_buffers(VkCommandBuffer[RENDER_SWAPCHAIN_IMAGE_MAX_COUNT]) const;
        void destroy_swapchain_images(RenderVulkanUtils::swapchain_images&) const;
        void intitialize_sync_object(RenderVulkanUtils::sync_object&) const;
    };
    struct shader_create_info {
        shader_stage stages[RENDER_DEFAULT_MAX_VALUE]{};
        uint32_t stageCount{0};
        VkDynamicState dynamicStates[RENDER_DEFAULT_MAX_VALUE]{VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
        VkPrimitiveTopology primitiveTopology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        uint32_t dynamicStateCount{2};
        float lineWidth{1.0f};
        VkPolygonMode polygonMode{VK_POLYGON_MODE_FILL};
    };
    struct shader final {
        private:
        render_manager* owner;
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

        private:
        void set_members_zero();

        private:
        [[nodiscard]] VkRenderPass create_render_pass() const;
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
        shader_builder& add_dynamic_state(VkDynamicState);
        shader_builder& pop_dynamic_state();
        shader_builder& set_primitive_topology(VkPrimitiveTopology);
        shader_builder& set_line_width(float);
        shader_builder& set_polygon_mode(VkPolygonMode);
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
        void map_memory(void**);
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