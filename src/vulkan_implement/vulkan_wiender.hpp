
#include <pickmelib/inplacevector.hpp>
#include <pickmelib/reado.hpp>
#include <vulkan/vulkan.h>
#include <vector>

#include "../wiender_implement_core.hpp"
#include "spirv_reflection_support.hpp"
#include "../include/wiender.hpp"

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#elif defined(__linux__)

#if defined(USE_X11)
#include <vulkan/vulkan_xlib.h>
#endif // USE_X11

#if defined(USE_WAYLAND)
#include <vulkan/vulkan_wayland.h>
#endif // USE_WAYLAND

#elif defined(__APPLE__)
#include <vulkan/vulkan_macos.h>
#endif

#define WIENDER_ARRSIZE(arr__) (sizeof(arr__) / sizeof(arr__[0])) 
#define __WIENDER_STRINGIFY(x) #x
#define WIENDER_TOSTRING(x) __WIENDER_STRINGIFY(x)

#define WIENDER_SMALL_ARRAY_SIZE 8
#define WIENDER_DEFAULT_ARRAY_SIZE 16
#define WIENDER_BIG_ARRAY_SIZE 64
#define WIENDER_HUGE_ARRAY_SIZE 256

#define WIENDER_UNIFORM_BUFFER_MAX_COUNT WIENDER_SMALL_ARRAY_SIZE
#define WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT WIENDER_SMALL_ARRAY_SIZE
// #define WIENDER_COMMAND_MAX_COUNT WIENDER_HUGE_ARRAY_SIZE

#define WIENDER_VK_INVALID_FAMILY_INDEX ~0UL

#define WIENDER_ALLOCATOR_NAME nullptr
#define WIENDER_CHILD_ALLOCATOR_NAME nullptr

/*
    I use `accurate_destroy` instead `std::unique_ptr` and RAII wrapper cuz

    Many Vulkan resources depend on the logical device (ldevice_). Using std::unique_ptr for these resources would require passing the logical device to their custom deleters.
        and
    The accurate_destroy method provides a straightforward way to manage resource cleanup. Just simple

*/

namespace wiender {
 // constants
    namespace {
        const struct {
            const char* deviceExtensions[2] { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME };
            const char* validationLayers[1] { "VK_LAYER_KHRONOS_validation" };
#ifdef _WIN32
            const char* instanceExtensions[3] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, "VK_EXT_debug_utils" };
#elif defined(__linux__)
#   if defined(USE_WAYLAND)
            const char* instanceExtensions[3] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, "VK_EXT_debug_utils" };
#   elif defined(USE_X11)
            const char* instanceExtensions[3] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XLIB_SURFACE_EXTENSION_NAME, "VK_EXT_debug_utils" };
#   endif // USE_WAYLAND
#elif defined(__APPLE__)
            const char* instanceExtensions[3] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_MVK_MACOS_SURFACE_EXTENSION_NAME, "VK_EXT_debug_utils" };
#else
        const char* instanceExtensions[2] = { VK_KHR_SURFACE_EXTENSION_NAME, "VK_EXT_debug_utils" };
#endif
        } stConstants;
    }

    VkShaderStageFlagBits shader_stage_kind_to_vk_shader_stage(shader::stage::kind k) {
        switch (k) {
            case shader::stage::kind::VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
            case shader::stage::kind::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case shader::stage::kind::COMPUTE:  return VK_SHADER_STAGE_COMPUTE_BIT;
            default: throw std::runtime_error("wiender::shader_stage_kind_to_vk_shader_stage unknown shader stage kind");
        }
        // unreachable
    }
    VkFormat shader_vertex_input_attribute_format_to_vk_format(shader::vertex_input_attribute::format f) {
        switch (f) {
            case shader::vertex_input_attribute::format::FLOAT_SCALAR:  return VK_FORMAT_R32_SFLOAT;
            case shader::vertex_input_attribute::format::FLOAT_VEC2:    return VK_FORMAT_R32G32_SFLOAT;
            case shader::vertex_input_attribute::format::FLOAT_VEC3:    return VK_FORMAT_R32G32B32_SFLOAT;
            case shader::vertex_input_attribute::format::FLOAT_VEC4:    return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: throw std::runtime_error("wiender::shader_vertex_input_attribute_format_to_vk_format unknown vertex input attribute format");
        }
        // unreachable
    }
    size_t sizeof_shader_vertex_input_attribute_format(shader::vertex_input_attribute::format f) {
        switch (f) {
            case shader::vertex_input_attribute::format::FLOAT_SCALAR:  return sizeof(float);
            case shader::vertex_input_attribute::format::FLOAT_VEC2:    return sizeof(float) * 2;
            case shader::vertex_input_attribute::format::FLOAT_VEC3:    return sizeof(float) * 3;
            case shader::vertex_input_attribute::format::FLOAT_VEC4:    return sizeof(float) * 4;
            default: throw std::runtime_error("wiender::sizeof_shader_vertex_input_attribute_format unknown vertex input attribute format");
        }
        // unreachable
    }
    VkPolygonMode shader_polygon_mode_to_vk_polygon_mode(wiender::shader::polygon_mode pm) {
        switch (pm) {
            case wiender::shader::polygon_mode::FILL:    return VK_POLYGON_MODE_FILL;
            case wiender::shader::polygon_mode::LINE:    return VK_POLYGON_MODE_LINE;
            case wiender::shader::polygon_mode::POINT:   return VK_POLYGON_MODE_POINT;
            default: throw std::runtime_error("wiender::shader_polygon_mode_to_vk_polygon_mode unknown shader polygon mode");
        }
        // unreachable
    }
    VkPrimitiveTopology shader_primitive_topology_to_vk_primitive_topology(wiender::shader::primitive_topology pm) {
        switch (pm) {
            case wiender::shader::primitive_topology::TRIANGLES_LIST:    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case wiender::shader::primitive_topology::TRIANGLES_FAN:     return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            case wiender::shader::primitive_topology::LINES:             return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case wiender::shader::primitive_topology::POINTS:            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            default: throw std::runtime_error("wiender::shader_primitive_topology_to_vk_primitive_topology unknown shader primitive topology");
        }
        // unreachable
    }
    VkCullModeFlags shader_cull_mode_to_vk_cull_mode(shader::cull_mode cm) {
        switch (cm) {
            case shader::cull_mode::NONE:   return VK_CULL_MODE_NONE;
            case shader::cull_mode::BACK:   return VK_CULL_MODE_BACK_BIT;
            case shader::cull_mode::FRONT:  return VK_CULL_MODE_FRONT_BIT;
            case shader::cull_mode::ALL:    return VK_CULL_MODE_BACK_BIT | VK_CULL_MODE_BACK_BIT;
            default: throw std::runtime_error("wiender::shader_cull_mode_to_vk_cull_mode unknown shader cull mode");
        }
        // unreachable
    }
    VkFilter texture_sampler_filter_to_vk_filter(texture::sampler_filter sf) {
        switch (sf) {
            case texture::sampler_filter::LINEAR:  return VK_FILTER_LINEAR;
            case texture::sampler_filter::NEAREST: return VK_FILTER_NEAREST;
            default: throw std::runtime_error("wiender::texture_sampler_filter_to_vk_filter unknown texture sampler filter");
        }
        // unreachable
    }

    struct binded_buffer_state {
        VkBuffer buffer;
    };
    struct active_shader_state {
        VkPipeline pipeline;
        VkPipelineLayout layout;
        VkRenderPass renderPass;
        VkDescriptorSet descriptorSet;
    };
    struct vulkan_image {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    };

    void vulkan_check(VkResult vkr, const wcs::tiny_string_view<char>& strv) {
        wiender_assert(vkr == VK_SUCCESS, strv);
    }

    struct vulkan_wienderer;
    WIENDER_NODISCARD std::unique_ptr<buffer> create_gpu_side_buffer(vulkan_wienderer* owner, std::size_t sizeb, VkBufferUsageFlags usage);
    WIENDER_NODISCARD std::unique_ptr<buffer> create_cpu_side_buffer(vulkan_wienderer* owner, std::size_t sizeb, VkBufferUsageFlags usage);
    WIENDER_NODISCARD std::unique_ptr<shader> create_vulkan_shader(vulkan_wienderer* owner, const shader::create_info& createInfo);
    WIENDER_NODISCARD std::unique_ptr<texture> create_image_texture(vulkan_wienderer* owner, const texture::create_info& createInfo);
    
    struct vulkan_wienderer final : public wienderer {
        private:
        struct queue_family_indices {
            public:
            union {
                struct {
                    uint32_t graphicsFamily;
                    uint32_t presentFamily;
                };
                uint32_t indeces[2]{WIENDER_VK_INVALID_FAMILY_INDEX, WIENDER_VK_INVALID_FAMILY_INDEX};
            };
            uint32_t falimiesCount;

            bool is_complete() const noexcept {
                return (graphicsFamily != WIENDER_VK_INVALID_FAMILY_INDEX) && (presentFamily != WIENDER_VK_INVALID_FAMILY_INDEX);
            }
        };
        struct physical_device_info {
            public:
            VkPhysicalDevice device;
            VkPhysicalDeviceFeatures2 features;
            VkPhysicalDeviceProperties2 properties;
            queue_family_indices queueIndeces;

            VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures;

            public:
            operator const VkPhysicalDevice& () const noexcept{
                return device;
            }
            operator VkPhysicalDevice& () noexcept {
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
        struct swapchain_support_info {
            uint32_t imageCount;
            VkSurfaceFormatKHR imageFormat;
            VkSurfaceFormatKHR surfaceFormat;
            VkPresentModeKHR presentMode;
            VkExtent2D extent;
            VkSurfaceCapabilitiesKHR capabilities;
        };
        struct swapchain_image {
            VkImage image;
            VkImageView view;
            VkFramebuffer framebuffer;
        };
        using swapchain_images = wcs::inplace_vector<swapchain_image, WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT>;
        using command_buffers = wcs::inplace_vector<VkCommandBuffer, WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT>;

        struct sync_object {
            VkFence fence;
            VkSemaphore semaphore;
        };
        enum struct render_command_type {
            SET_SHADER,             // data: [ activeShaderState ]
            BIND_VERTEX_BUFFER,     // data: [ bindedBufferState ]
            BIND_INDEX_BUFFER,      // data: [ bindedBufferState ]
            BEGIN_RECORD,           // data: null
            RECORD_UPDATE_SCISSOR,  // data: null
            RECORD_UPDATE_VIEWPORT, // data: null
            RECORD_BEGIN_RENDER,    // data: null
            RECORD_DRAW_VERTECES,   // data: [ drawData ]
            RECORD_DRAW_INDEXED,    // data: [ drawData ]
            RECORD_END_RENDER,      // data: null
            END_RECORD,             // data: null
        };
        struct render_command {
            render_command_type commandType;
            union {
                binded_buffer_state bindedBufferState;
                active_shader_state activeShaderState;
                struct {
                    uint32_t count;
                    uint32_t first;
                    uint32_t instanceCount;
                } drawData;
            } data;
        };
        using render_commands = std::vector<render_command>;// wcs::inplace_vector<render_command, WIENDER_COMMAND_MAX_COUNT>;
        struct commands_frame : public wiender_commands_frame, render_commands {
            commands_frame() : wiender_commands_frame{}, render_commands() {

            }
            commands_frame(const commands_frame& other) : wiender_commands_frame{}, render_commands(other.begin(), other.end()) {

            }
            commands_frame(const render_commands& other) : wiender_commands_frame{}, render_commands(other) {

            }
        };


        private:
        bool validationEnable_;
     // VkAllocationCallbacks allocationCallbacks_;
        VkInstance instance_;
        physical_device_info pdevice_;
        VkSampleCountFlagBits msaaSamples_;
        logical_device_info ldevice_;
        VkSurfaceKHR surface_;
        swapchain_support_info swapchainSupportInfo_;
        vulkan_image colorRenderTarget_;
        VkRenderPass defaultRenderPass_;
        VkSwapchainKHR swapchain_;
        swapchain_images swapchainImages_;
        VkCommandPool commandPool_;
        command_buffers commandBuffers_;
        sync_object syncObject_;
        vulkan_image defaultTextureImage_;
        VkSampler defaultSampler_;
        active_shader_state currentShader_;
        binded_buffer_state vertexBindedBuffer_;
        binded_buffer_state indexBindedBuffer_;
        uint32_t imageIndex_;
        render_commands appliedCommands_;
        bool recording_;

        public:
        vulkan_wienderer(const window_handle& whandle) 
                        :   validationEnable_(true),
                            instance_{},
                            pdevice_{},
                            msaaSamples_{},
                            ldevice_{},
                            surface_{},
                            swapchainSupportInfo_{},
                            colorRenderTarget_{},
                            defaultRenderPass_{},
                            swapchain_{},
                            swapchainImages_{},
                            commandPool_{},
                            commandBuffers_{},
                            syncObject_{},
                            defaultTextureImage_{},
                            defaultSampler_{},
                            currentShader_{},
                            vertexBindedBuffer_{},
                            indexBindedBuffer_{},
                            imageIndex_{},
                            appliedCommands_{},
                            recording_(false) {

            try {
                instance_ = create_vulkan_instance();

                pdevice_ = get_physical_device();

                msaaSamples_ = get_max_usable_sample_count(VK_SAMPLE_COUNT_4_BIT);

                surface_ = create_platform_spec_surface(whandle);

                pdevice_.queueIndeces = create_falimy_indices();

                ldevice_ = create_logical_device();

                swapchainSupportInfo_ = create_swapchain_info();

                colorRenderTarget_ = create_color_render_target();

                defaultRenderPass_ = create_default_render_pass(VK_ATTACHMENT_LOAD_OP_DONT_CARE);

                swapchain_ = create_swapchain();

                initialize_swapchain_images(swapchainImages_);

                commandPool_ = create_command_pool();

                allocate_command_buffers(commandBuffers_);

                intitialize_sync_object(syncObject_);

                defaultTextureImage_ = create_default_texture_image();

                defaultSampler_ = create_default_texture_sampler();

            } catch (...) {
                accurate_destroy();
                throw;
            }
        }

        public:
        ~vulkan_wienderer() override {
            accurate_destroy();
        }

        public:
        WIENDER_NODISCARD VkSampler get_default_sampler() const noexcept {
            return defaultSampler_;
        }
        WIENDER_NODISCARD const vulkan_image& get_default_texture_image() const noexcept {
            return defaultTextureImage_;
        }
        WIENDER_NODISCARD std::unique_ptr<buffer> create_buffer(buffer::type type, std::size_t sizeb) override {
            switch (type) {
            case buffer::type::GPU_SIDE_VERTEX :
                return create_gpu_side_buffer(this, sizeb, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            case buffer::type::CPU_SIDE_VERTEX :
                return create_cpu_side_buffer(this, sizeb, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

            case buffer::type::GPU_SIDE_INDEX :
                return create_gpu_side_buffer(this, sizeb, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            case buffer::type::CPU_SIDE_INDEX :
                return create_cpu_side_buffer(this, sizeb, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

            default:
                throw std::runtime_error("wiender::vulkan_wienderer::create_buffer unknown buffer type");
            }
         // unreachable
        }
        WIENDER_NODISCARD std::unique_ptr<shader> create_shader(const shader::create_info& createInfo) override {
            return create_vulkan_shader(this, createInfo);
        }
        WIENDER_NODISCARD std::unique_ptr<texture> create_texture(const texture::create_info& createInfo) override {
            return create_image_texture(this, createInfo);
        }
        WIENDER_NODISCARD std::unique_ptr<texture> get_postproc_texture() override {
            return nullptr;
        }
        WIENDER_NODISCARD std::unique_ptr<wiender_commands_frame> get_commands_frame() const override {
            return std::unique_ptr<commands_frame>(new commands_frame(appliedCommands_));
        }
        void clear_commands_frame() override {
            for (const auto& buffer : commandBuffers_)
                vkResetCommandBuffer(buffer, static_cast<VkFlags>(0));
            appliedCommands_.clear();
        }
        void set_commands_frame(const wiender_commands_frame* frame) override {
            wiender_assert(frame != nullptr, "wiender::vulkan_wienderer::set_commands failed to set BRAAAND new command frame");
            wiender_assert(!recording_, "wiender::vulkan_wienderer::set_commands failed to set BRAAAND new command frame while buffers are recording");

            const commands_frame& aframe = *(commands_frame*)frame;

            appliedCommands_.assign(aframe.begin(), aframe.end());
            for (const auto& buffer : commandBuffers_)
                vkResetCommandBuffer(buffer, static_cast<VkFlags>(0));
            concat_vulkan_buffers(appliedCommands_);
        }
        void concat_commands_frame(const wiender_commands_frame* frame) override {
            wiender_assert(frame != nullptr, "wiender::vulkan_wienderer::concat_commands failed to cancat BRAAAND new command frame");

            const commands_frame& aframe = *(commands_frame*)frame;

            appliedCommands_.insert(appliedCommands_.end(), aframe.begin(), aframe.end());
            concat_vulkan_buffers(aframe);
        }
        void begin_record() override {
            wiender_assert(!recording_, "wiender::vulkan_wenerer::begin_record buffers already in record state");

            for (uint32_t i = 0; i < commandBuffers_.size(); ++i) {
                const auto& buffer = commandBuffers_[i];

                VkCommandBufferInheritanceInfo inheritanceInfo{};
                inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
             // inheritanceInfo.pNext = nullptr;
                inheritanceInfo.renderPass = currentShader_.renderPass;
             // inheritanceInfo.subpass = {};
                inheritanceInfo.framebuffer = swapchainImages_[i].framebuffer;
                inheritanceInfo.occlusionQueryEnable = VK_FALSE;
             // inheritanceInfo.queryFlags = static_cast<VkQueryControlFlags>(0);
             // inheritanceInfo.pipelineStatistics = {};
                
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
             // beginInfo.pNext = nullptr;
             // beginInfo.flags = static_cast<VkFlags>(0);
                beginInfo.pInheritanceInfo = &inheritanceInfo;

                vulkan_check(vkBeginCommandBuffer(buffer, &beginInfo), "wiender::vulkan_wienderer::begin_record failed to begin recording buffers");
            }
            appliedCommands_.emplace_back(render_command{ render_command_type::BEGIN_RECORD, { }});
            recording_ = true;
        }
        void begin_render() override {
            if ((swapchainSupportInfo_.extent.width == 0) || (swapchainSupportInfo_.extent.height == 0))
                return;
            wiender_assert(currentShader_.pipeline != 0, "wiender::vulkan_wienderer::begin_render you should set shader before render");

            for (uint32_t i = 0; i < commandBuffers_.size(); ++i) {
                const auto& buffer = commandBuffers_[i];

                const VkClearValue clearVal{};
                VkRenderPassBeginInfo beginRenderPassInfo{};
                beginRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
             // beginRenderPassInfo.pNext = nullptr;
                beginRenderPassInfo.renderPass = currentShader_.renderPass;
                beginRenderPassInfo.framebuffer = swapchainImages_[i].framebuffer;
                beginRenderPassInfo.renderArea = {{0, 0}, swapchainSupportInfo_.extent};
                beginRenderPassInfo.clearValueCount = 1;
                beginRenderPassInfo.pClearValues = &clearVal;

                vkCmdBeginRenderPass(buffer, &beginRenderPassInfo, {});

                vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentShader_.layout, 0, 1, &currentShader_.descriptorSet, 0, nullptr );
            }
            appliedCommands_.emplace_back(render_command{ render_command_type::RECORD_BEGIN_RENDER, { }});

        }
        void draw_verteces(uint32_t vertexCount, uint32_t firstVertex, uint32_t instanceCount) override {
            if ((swapchainSupportInfo_.extent.width == 0) || (swapchainSupportInfo_.extent.height == 0))
                return;

            for (const auto& buffer : commandBuffers_) {
                const VkDeviceSize offsets[] {
                    0,
                };
                vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentShader_.pipeline);
                vkCmdBindVertexBuffers(buffer, 0, 1, &vertexBindedBuffer_.buffer, offsets);

                vkCmdDraw(buffer, vertexCount, instanceCount, firstVertex, 0);
            }

            appliedCommands_.emplace_back(render_command{ render_command_type::RECORD_DRAW_VERTECES, { }});
            appliedCommands_.back().data.drawData = {vertexCount, firstVertex, instanceCount};
        }
        void draw_indexed(uint32_t indecesCount, uint32_t firstIndex, uint32_t instanceCount) override {
            if ((swapchainSupportInfo_.extent.width == 0) || (swapchainSupportInfo_.extent.height == 0))
                return;

            for (const auto& buffer : commandBuffers_) {
                const VkDeviceSize offsets[] {
                    0,
                };
                vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentShader_.pipeline);
                vkCmdBindVertexBuffers(buffer, 0, 1, &vertexBindedBuffer_.buffer, offsets);
                vkCmdBindIndexBuffer(buffer, indexBindedBuffer_.buffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(buffer, indecesCount, instanceCount, firstIndex, 0, 0);
            }

            appliedCommands_.emplace_back(render_command{ render_command_type::RECORD_DRAW_VERTECES, { }});
            appliedCommands_.back().data.drawData = {indecesCount, firstIndex, instanceCount};
        }
        void end_render() override {
            if ((swapchainSupportInfo_.extent.width == 0) || (swapchainSupportInfo_.extent.height == 0))
                return;

            for (const auto& buffer : commandBuffers_)
                vkCmdEndRenderPass(buffer);
            appliedCommands_.emplace_back(render_command{ render_command_type::RECORD_END_RENDER, { }});
        }
        void end_record() override {
            wiender_assert(recording_, "wiender::vulkan_wenerer::end_record buffers are not in record state");

            for (const auto& buffer : commandBuffers_)
                vulkan_check(vkEndCommandBuffer(buffer), "wiender::vulkan_wienderer::end_record failed to end recording buffers");
            appliedCommands_.emplace_back(render_command{ render_command_type::END_RECORD, { }});
            recording_ = false;
        }
        void execute() override {
            if ((swapchainSupportInfo_.extent.width == 0) || (swapchainSupportInfo_.extent.height == 0))
                return;

            const VkSemaphore& semaphore = syncObject_.semaphore;
            const VkFence& fence = syncObject_.fence;
            vkWaitForFences(ldevice_, 1, &fence, VK_TRUE, UINT64_MAX);
            vkAcquireNextImageKHR(ldevice_, swapchain_, UINT64_MAX, semaphore, 0, &imageIndex_);

            vkResetFences(ldevice_, 1, &fence);
            const VkPipelineStageFlags waitStages[] { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            VkSubmitInfo submit{};
            submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            // submit.pNext = nullptr;
            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &semaphore;
            submit.pWaitDstStageMask = waitStages;
            submit.commandBufferCount = static_cast<uint32_t>(WIENDER_ARRSIZE(waitStages));
            submit.pCommandBuffers = &commandBuffers_[imageIndex_];
         // submit.signalSemaphoreCount = 0;
         // submit.pSignalSemaphores = nullptr;

            vkQueueSubmit(ldevice_.graphicsQueue, 1, &submit, fence);
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            // presentInfo.pNext = nullptr;
            // presentInfo.waitSemaphoreCount = 0;
            // presentInfo.pWaitSemaphores = nullptr;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain_;
            presentInfo.pImageIndices = &imageIndex_;
            // presentInfo.pResults = nullptr; // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentInfoKHR.html

            vkQueuePresentKHR(ldevice_.presentQueue, &presentInfo);
        }
        void wait_executing() override {
            const VkFence& fence = syncObject_.fence;
            vkWaitForFences(ldevice_, 1, &fence, VK_TRUE, UINT64_MAX);

         // vkQueueWaitIdle(ldevice_.graphicsQueue); // SLOW SLOW SLOW
         // vkQueueWaitIdle(ldevice_.presentQueue);
        }

        public:
        void destroy_vulkan_image(vulkan_image& image) const {
            if (image.view != 0) {
                vkDestroyImageView(ldevice_, image.view, WIENDER_ALLOCATOR_NAME);
            }
            if (image.image != 0) {
                vkDestroyImage(ldevice_, image.image, WIENDER_ALLOCATOR_NAME);
            }
            if (image.memory != 0) {
                vkFreeMemory(ldevice_, image.memory, WIENDER_ALLOCATOR_NAME);
            }
        }
        WIENDER_NODISCARD bool is_multisampling_enabled() const {
            return msaaSamples_ != VK_SAMPLE_COUNT_1_BIT;
        }
        WIENDER_NODISCARD VkSampleCountFlagBits get_msaa_samples() const {
            return msaaSamples_;
        }
        WIENDER_NODISCARD VkSurfaceFormatKHR get_swapcahin_image_format() const noexcept {
            return swapchainSupportInfo_.imageFormat;
        }
        WIENDER_NODISCARD VkExtent2D get_swapchain_extent() const noexcept {
            return swapchainSupportInfo_.extent;
        }
        WIENDER_NODISCARD uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(pdevice_, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }

            throw std::runtime_error("wiender::vulkan_wienderer::find_memory_type failed to find a suitable memory type");
        }
        WIENDER_NODISCARD const logical_device_info& get_ldevice() const {
            return ldevice_;
        }
        WIENDER_NODISCARD const physical_device_info& get_pdevice() const {
            return pdevice_;
        }
        void copy_buffer(VkCommandBuffer cmdbuff, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const {
            VkBufferCopy copyRegion{};
         // copyRegion.srcOffset = 0;
         // copyRegion.dstOffset = 0;
            copyRegion.size = size;
            vkCmdCopyBuffer(cmdbuff, srcBuffer, dstBuffer, 1, &copyRegion);
        }
        void copy_buffer_to_image(VkCommandBuffer cmdbuff, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth) const {
            VkBufferImageCopy region{};
         // region.bufferOffset = 0;
         // region.bufferRowLength = 0;
         // region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
         // region.imageSubresource.mipLevel = 0;
         // region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
         // region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                std::max(1u, width),
                std::max(1u, height),
                std::max(1u, depth),
            };

            vkCmdCopyBufferToImage(
                cmdbuff,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        }
        WIENDER_NODISCARD VkCommandBuffer begin_single_time_commands() const {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool_;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(ldevice_, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }
        void end_single_time_commands(VkCommandBuffer commandBuffer) const {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(ldevice_.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(ldevice_.graphicsQueue);

            vkFreeCommandBuffers(ldevice_, commandPool_, 1, &commandBuffer);
        }
        void transition_image_layout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) const {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = 0;

                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            } else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }
        void set_shader_state(const active_shader_state& newCurrentShader) noexcept {
            appliedCommands_.emplace_back(render_command{ render_command_type::SET_SHADER, { }});
            appliedCommands_.back().data.activeShaderState = newCurrentShader;
            currentShader_ = newCurrentShader;
        }
        void bind_vertex_buffer_state(const binded_buffer_state& newBindedBuffer) noexcept {
            appliedCommands_.emplace_back(render_command{ render_command_type::BIND_VERTEX_BUFFER, { }});
            appliedCommands_.back().data.bindedBufferState = newBindedBuffer;
            vertexBindedBuffer_ = newBindedBuffer;
        }
        void bind_index_buffer_state(const binded_buffer_state& newBindedBuffer) noexcept {
            appliedCommands_.emplace_back(render_command{ render_command_type::BIND_INDEX_BUFFER, { }});
            appliedCommands_.back().data.bindedBufferState = newBindedBuffer;
            indexBindedBuffer_ = newBindedBuffer;
        }
        VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
         // viewInfo.pNext = nullptr;
         // viewInfo.flags = static_cast<VkFlags>(0);
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.components = VkComponentMapping {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            };
            viewInfo.subresourceRange = VkImageSubresourceRange{
                /*aspectMask      */   aspectFlags,
                /*baseMipLevel    */   0,
                /*levelCount      */   mipLevels,
                /*baseArrayLayer  */   0,
                /*layerCount      */   1,
            };

            VkImageView result;
            vulkan_check(vkCreateImageView(ldevice_, &viewInfo, WIENDER_ALLOCATOR_NAME, &result), "wiender::vulkan_wienderer::create_image_view failed to create image view");
            return result;
        }
        vulkan_image create_vulkan_image(uint32_t width, uint32_t height, uint32_t mipLevels, VkImageAspectFlags aspectFlags, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
         // pNext = nullptr;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = numSamples;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            VkImage image;
            vulkan_check(vkCreateImage(ldevice_, &imageInfo, WIENDER_ALLOCATOR_NAME, &image), "wiender::vulkan_wienderer::create_vulkan_image_memory failed to create image");

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(ldevice_, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
         // pNext = nullptr;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

            VkDeviceMemory memory;
            vulkan_check(vkAllocateMemory(ldevice_, &allocInfo, WIENDER_ALLOCATOR_NAME, &memory), "wiender::vulkan_wienderer::create_vulkan_image_memory failed to allocate image memory");

            vkBindImageMemory(ldevice_, image, memory, 0);

            VkImageView view = create_image_view(image, format, aspectFlags, mipLevels);

            return vulkan_image{ image, memory, view };
        }

        private:
        void accurate_destroy() {
            if (recording_) {
                end_record();
            }
            
            if (ldevice_ != 0)
                vkDeviceWaitIdle(ldevice_);

            if (defaultSampler_ != 0)
                vkDestroySampler(ldevice_, defaultSampler_, WIENDER_ALLOCATOR_NAME);

            destroy_vulkan_image(defaultTextureImage_);

            if (syncObject_.semaphore != 0)
                vkDestroySemaphore(ldevice_, syncObject_.semaphore, WIENDER_ALLOCATOR_NAME);
            if (syncObject_.fence != 0)
                vkDestroyFence(ldevice_, syncObject_.fence, WIENDER_ALLOCATOR_NAME);

            if (commandPool_ != 0)
                vkDestroyCommandPool(ldevice_, commandPool_, WIENDER_ALLOCATOR_NAME);

            destroy_swapchain_images(swapchainImages_);
            destroy_vulkan_image(colorRenderTarget_);

            if (swapchain_ != 0)
                vkDestroySwapchainKHR(ldevice_, swapchain_, WIENDER_ALLOCATOR_NAME);
            swapchain_ = 0;

            if (defaultRenderPass_ != 0)
                vkDestroyRenderPass(ldevice_, defaultRenderPass_, WIENDER_ALLOCATOR_NAME);
            defaultRenderPass_ = 0;

            if (ldevice_ != 0)
                vkDestroyDevice(ldevice_, WIENDER_ALLOCATOR_NAME);
            ldevice_ = {};

            if (surface_ != 0)
                vkDestroySurfaceKHR(instance_, surface_, WIENDER_ALLOCATOR_NAME);
            surface_ = 0;

            if (instance_ != 0)
                vkDestroyInstance(instance_, WIENDER_ALLOCATOR_NAME);
            instance_ = 0;
        }
        void concat_vulkan_buffers(const render_commands& commands) {
            for (const auto& command : commands) {
                switch (command.commandType) {
                    case render_command_type::SET_SHADER                : set_shader_state(command.data.activeShaderState); break;
                    case render_command_type::BIND_VERTEX_BUFFER        : bind_vertex_buffer_state(command.data.bindedBufferState); break;
                    case render_command_type::BIND_INDEX_BUFFER         : bind_index_buffer_state(command.data.bindedBufferState); break;
                    case render_command_type::BEGIN_RECORD              : begin_record(); break;
                 // case render_command_type::RECORD_UPDATE_SCISSOR     : record_update_scissor(); break;
                 // case render_command_type::RECORD_UPDATE_VIEWPORT    : record_update_viewport(); break;
                    case render_command_type::RECORD_BEGIN_RENDER       : begin_render(); break;
                    case render_command_type::RECORD_DRAW_VERTECES      : draw_verteces(command.data.drawData.count, command.data.drawData.first, command.data.drawData.instanceCount); break;
                    case render_command_type::RECORD_DRAW_INDEXED       : draw_indexed(command.data.drawData.count, command.data.drawData.first, command.data.drawData.instanceCount); break;
                    case render_command_type::RECORD_END_RENDER         : end_render(); break;
                    case render_command_type::END_RECORD                : end_record(); break;
                    
                default:
                    break;
                }
            }
        }

        private:
        WIENDER_NODISCARD VkSampler create_default_texture_sampler() const {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
         // samplerInfo.pNext = nullptr;
         // samplerInfo.flags = static_cast<VkFlags>(0);
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.mipLodBias = 0.0f;
            // samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            // samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 1.0f;
            samplerInfo.borderColor = {};
            // samplerInfo.unnormalizedCoordinates = VK_FALSE;

            VkSampler result;
            vulkan_check(vkCreateSampler(ldevice_, &samplerInfo, 0, &result), "wiender::vulkan_wienderer::create_default_texture_sampler failed to create default sampler");
            return result;
        }
        WIENDER_NODISCARD vulkan_image create_default_texture_image() const {
            vulkan_image result = create_vulkan_image(
                swapchainSupportInfo_.extent.width, swapchainSupportInfo_.extent.height,
                1,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_SAMPLE_COUNT_1_BIT,
                swapchainSupportInfo_.imageFormat.format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            VkCommandBuffer cmdbuff = begin_single_time_commands();
            transition_image_layout(cmdbuff, result.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            end_single_time_commands(cmdbuff);

            return result;
        }
        void intitialize_sync_object(sync_object& syncObjectsToInitialize) const {
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
         // fenceInfo.pNext = nullptr;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            vulkan_check(vkCreateFence(ldevice_, &fenceInfo, WIENDER_ALLOCATOR_NAME, &syncObjectsToInitialize.fence), "wiender::vulkan_wienderer::intitialize_sync_object failed to create fence for sync object");

            VkSemaphoreCreateInfo semapforeInfo{};
            semapforeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
         // semapforeInfo.pNext = nullptr,
            semapforeInfo.flags = static_cast<VkFlags>(0),
            vulkan_check(vkCreateSemaphore(ldevice_, &semapforeInfo, WIENDER_ALLOCATOR_NAME, &syncObjectsToInitialize.semaphore), "wiender::vulkan_wienderer::intitialize_sync_object failed to create semaphore for sync object");
        }
        void allocate_command_buffers(command_buffers& commandBuffers_) const {
            commandBuffers_.resize(swapchainImages_.size());

            VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
         // commandBufferAllocateInfo.pNext = nullptr;
            commandBufferAllocateInfo.commandPool = commandPool_;
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = commandBuffers_.size();


            vulkan_check(vkAllocateCommandBuffers(ldevice_, &commandBufferAllocateInfo, commandBuffers_.data()), "wiender::vulkan_wienderer::allocate_command_buffers failed to allocate command buffers");
        }
        WIENDER_NODISCARD VkCommandPool create_command_pool() const {
            VkCommandPoolCreateInfo commandPoolCreateInfo{};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
         // commandPoolCreateInfo.pNext = nullptr;
            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            commandPoolCreateInfo.queueFamilyIndex = pdevice_.queueIndeces.graphicsFamily;

            VkCommandPool newCommandPool;
            vulkan_check(vkCreateCommandPool(ldevice_, &commandPoolCreateInfo, WIENDER_ALLOCATOR_NAME, &newCommandPool), "wiender::vulkan_wienderer::create_command_pool failed to create command pool");

            return newCommandPool;
        }
        void destroy_swapchain_images(swapchain_images& swapchainImagesToDestroy) const {
            for (const auto& image : swapchainImagesToDestroy) {
                if (image.framebuffer != 0)
                    vkDestroyFramebuffer(ldevice_, image.framebuffer, WIENDER_ALLOCATOR_NAME);
                if (image.view != 0)
                    vkDestroyImageView(ldevice_, image.view, WIENDER_ALLOCATOR_NAME);
            }
            swapchainImagesToDestroy.clear();
        }
        void initialize_swapchain_images(swapchain_images& swapchainImagesToInitialize) const {
            uint32_t swapchainImageCount;
            VkImage images[WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT];

            vulkan_check(vkGetSwapchainImagesKHR(ldevice_, swapchain_, &swapchainImageCount, nullptr), "wiender::vulkan_wienderer::initialize_swapchain_images failed to get swapchain images 1");
            wiender_assert(swapchainImageCount > 0, "wiender::vulkan_wienderer::initialize_swapchain_images no images from swapchain");
            if (swapchainImageCount > WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT)
                swapchainImageCount = WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT;
            swapchainImagesToInitialize.resize(swapchainImageCount);
            vulkan_check(vkGetSwapchainImagesKHR(ldevice_, swapchain_, &swapchainImageCount, images), "wiender::vulkan_wienderer::initialize_swapchain_images failed to get swapchain images 2");

            for (uint32_t i = 0; i < swapchainImageCount; ++i) {
                swapchainImagesToInitialize[i].image = images[i];
            }


            for (auto& swimage : swapchainImagesToInitialize) {
                swimage.view = create_image_view(swimage.image, swapchainSupportInfo_.imageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

                const VkImageView msaaAttachments[] = {colorRenderTarget_.view, swimage.view};
                const VkImageView noMsaaAttachments[] = {swimage.view};

                swimage.framebuffer = create_framebuffer(
                    static_cast<uint32_t>(is_multisampling_enabled() ? WIENDER_ARRSIZE(msaaAttachments) : WIENDER_ARRSIZE(noMsaaAttachments)),
                    (is_multisampling_enabled() ? msaaAttachments : noMsaaAttachments)
                );
            }
        }
        WIENDER_NODISCARD VkFramebuffer create_framebuffer(uint32_t attachmentCount, const VkImageView* attachments) const {
            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
         // framebufferCreateInfo.pNext = nullptr;
         // framebufferCreateInfo.flags = static_cast<VkFlags>(0);
            framebufferCreateInfo.renderPass = defaultRenderPass_;
            framebufferCreateInfo.attachmentCount = attachmentCount;
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = swapchainSupportInfo_.extent.width;
            framebufferCreateInfo.height = swapchainSupportInfo_.extent.height;
            framebufferCreateInfo.layers = 1;

            VkFramebuffer result;
            vulkan_check(vkCreateFramebuffer(ldevice_, &framebufferCreateInfo, WIENDER_ALLOCATOR_NAME, &result), "wiender::vulkan_wienderer::create_framebuffer failed to create framebuffer");
            return result;
        }
        WIENDER_NODISCARD VkSwapchainKHR create_swapchain() const {
            VkSwapchainCreateInfoKHR swapchainCreateInfo{};
            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
         // swapchainCreateInfo.pNext = nullptr;
         // swapchainCreateInfo.flags = static_cast<VkFlags>(0);
            swapchainCreateInfo.surface = surface_;
            swapchainCreateInfo.minImageCount = swapchainSupportInfo_.imageCount;
            swapchainCreateInfo.imageFormat = swapchainSupportInfo_.imageFormat.format;
            swapchainCreateInfo.imageColorSpace = swapchainSupportInfo_.imageFormat.colorSpace;
            swapchainCreateInfo.imageExtent = swapchainSupportInfo_.extent;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapchainCreateInfo.imageSharingMode = (pdevice_.queueIndeces.falimiesCount == 2u) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = (pdevice_.queueIndeces.falimiesCount == 2u) ? 2u : 0u;
            swapchainCreateInfo.pQueueFamilyIndices = pdevice_.queueIndeces.indeces;
            swapchainCreateInfo.preTransform = swapchainSupportInfo_.capabilities.currentTransform;
            swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainCreateInfo.presentMode = swapchainSupportInfo_.presentMode;
            swapchainCreateInfo.clipped = VK_TRUE;
         // swapchainCreateInfo.oldSwapchain = 0;

            VkSwapchainKHR newSwapchain;
            vulkan_check(vkCreateSwapchainKHR(ldevice_, &swapchainCreateInfo, WIENDER_ALLOCATOR_NAME, &newSwapchain), "wiender::vulkan_wienderer::create_swapchain failed to create swapchain");

            return newSwapchain;
        }

        public:
        WIENDER_NODISCARD VkRenderPass create_default_render_pass(VkAttachmentLoadOp loadOp) const {
            if (is_multisampling_enabled()) {
                return create_msaa_render_pass(loadOp);
            } else {
                return create_no_msaa_render_pass(loadOp);
            }
        }

        private:
        WIENDER_NODISCARD VkRenderPass create_no_msaa_render_pass(VkAttachmentLoadOp loadOp) const {
            VkAttachmentDescription colorAttachment{};
         // colorAttachment.flags = static_cast<VkFlags>(0);
            colorAttachment.format = swapchainSupportInfo_.imageFormat.format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

         // VkAttachmentDescription depthAttachment{}; TODO
         // depthAttachment.format = findDepthFormat();
         // depthAttachment.samples = get_msaa_samples();
         // depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
         // depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
         // depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
         // depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
         // depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
         // depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

         // VkAttachmentReference depthAttachmentRef{};
         // depthAttachmentRef.attachment = 1;
         // depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
         // subpass.flags = static_cast<VkFlags>(0);
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
         // subpass.inputAttachmentCount = 0;
         // subpass.pInputAttachments = 0;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pResolveAttachments = nullptr;
         // subpass.pDepthStencilAttachment = nullptr;
         // subpass.preserveAttachmentCount = 0;
         // subpass.pPreserveAttachments = nullptr;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
         // dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
         // dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
         // dependency.dependencyFlags = static_cast<VkDependencyFlags>(0);

            const VkAttachmentDescription attachments[] = { colorAttachment };

            VkRenderPassCreateInfo renderPassCreateInfo{};
            renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
         // renderPassCreateInfo.pNext = nullptr;
         // renderPassCreateInfo.flags = static_cast<VkFlags>(0);
            renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(WIENDER_ARRSIZE(attachments));
            renderPassCreateInfo.pAttachments = attachments;
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpass;
            renderPassCreateInfo.dependencyCount = 1;
            renderPassCreateInfo.pDependencies = &dependency;

            VkRenderPass newRenderPass;
            vulkan_check(vkCreateRenderPass(ldevice_, &renderPassCreateInfo, WIENDER_ALLOCATOR_NAME, &newRenderPass), "wiender::vulkan_wienderer::create_msaa_render_pass failed to create render pass");
            return newRenderPass;
        }
        WIENDER_NODISCARD VkRenderPass create_msaa_render_pass(VkAttachmentLoadOp loadOp) const {
            VkAttachmentDescription colorAttachment{};
         // colorAttachment.flags = static_cast<VkFlags>(0);
            colorAttachment.format = swapchainSupportInfo_.imageFormat.format;
            colorAttachment.samples = get_msaa_samples();
            colorAttachment.loadOp = loadOp;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // VkAttachmentDescription depthAttachment{}; TODO
            // depthAttachment.format = findDepthFormat();
            // depthAttachment.samples = get_msaa_samples();
            // depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            // depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            // depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            // depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription colorAttachmentResolve{};
         // colorAttachment.flags = static_cast<VkFlags>(0);
            colorAttachmentResolve.format = swapchainSupportInfo_.imageFormat.format;
            colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // VkAttachmentReference depthAttachmentRef{};
            // depthAttachmentRef.attachment = 1;
            // depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentResolveRef{};
            colorAttachmentResolveRef.attachment = 1;
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
         // subpass.flags = static_cast<VkFlags>(0);
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
         // subpass.inputAttachmentCount = 0;
         // subpass.pInputAttachments = 0;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pResolveAttachments = &colorAttachmentResolveRef;
         // subpass.pDepthStencilAttachment = nullptr;
         // subpass.preserveAttachmentCount = 0;
         // subpass.pPreserveAttachments = nullptr;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
         // dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
         // dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
         // dependency.dependencyFlags = static_cast<VkDependencyFlags>(0);

            const VkAttachmentDescription attachments[] = { colorAttachment, colorAttachmentResolve };

            VkRenderPassCreateInfo renderPassCreateInfo{};
            renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
         // renderPassCreateInfo.pNext = nullptr;
         // renderPassCreateInfo.flags = static_cast<VkFlags>(0);
            renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(WIENDER_ARRSIZE(attachments));
            renderPassCreateInfo.pAttachments = attachments;
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpass;
            renderPassCreateInfo.dependencyCount = 1;
            renderPassCreateInfo.pDependencies = &dependency;

            VkRenderPass newRenderPass;
            vulkan_check(vkCreateRenderPass(ldevice_, &renderPassCreateInfo, WIENDER_ALLOCATOR_NAME, &newRenderPass), "wiender::vulkan_wienderer::create_msaa_render_pass failed to create render pass");
            return newRenderPass;
        }

        private:
        WIENDER_NODISCARD vulkan_image create_color_render_target() const {
            return create_vulkan_image(
                swapchainSupportInfo_.extent.width, swapchainSupportInfo_.extent.height,
                1,
                VK_IMAGE_ASPECT_COLOR_BIT,
                get_msaa_samples(),
                swapchainSupportInfo_.imageFormat.format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
        }
        WIENDER_NODISCARD swapchain_support_info create_swapchain_info() const {
            swapchain_support_info newSwapchainSupportInfo;
            vulkan_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice_, surface_, &newSwapchainSupportInfo.capabilities), "wiender::vulkan_wienderer::create_swapchain_info failed to get physical device surface capabilities");

            uint32_t formatCount = 0;
            VkSurfaceFormatKHR formats[WIENDER_DEFAULT_ARRAY_SIZE] = {};
            vulkan_check(vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice_, surface_, &formatCount, nullptr), "wiender::vulkan_wienderer::create_swapchain_info failed to get surface formats 1");
            wiender_assert(formatCount > 0, "wiender::vulkan_wienderer::create_swapchain_info no supported formats found");
            if (formatCount > WIENDER_DEFAULT_ARRAY_SIZE)
                formatCount = WIENDER_DEFAULT_ARRAY_SIZE;
            vulkan_check(vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice_, surface_, &formatCount, formats), "wiender::vulkan_wienderer::create_swapchain_info failed to get surface formats 2");

            uint32_t presentModeCount = 0;
            VkPresentModeKHR presentModes[WIENDER_DEFAULT_ARRAY_SIZE] = {};
            vulkan_check(vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice_, surface_, &presentModeCount, nullptr), "wiender::vulkan_wienderer::create_swapchain_info failed to get surface present mods 1");
            wiender_assert(presentModeCount > 0, "wiender::vulkan_wienderer::create_swapchain_info no supported present modes");
            if (presentModeCount > WIENDER_DEFAULT_ARRAY_SIZE)
                presentModeCount = WIENDER_DEFAULT_ARRAY_SIZE;
            vulkan_check(vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice_, surface_, &presentModeCount, presentModes), "wiender::vulkan_wienderer::create_swapchain_info failed to get surface present mods 2");

            newSwapchainSupportInfo.imageFormat = formats[0];
            for (uint32_t i = 1; i < formatCount; ++i) {
                if ((formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) && (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)) { // rgba PLEASE.
                    newSwapchainSupportInfo.imageFormat = formats[i]; 
                    break;
                }
            }

            newSwapchainSupportInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            for (uint32_t i = 0; i < presentModeCount; ++i) {
                if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                    newSwapchainSupportInfo.presentMode = presentModes[i]; 
                    break;
                }
            }
            newSwapchainSupportInfo.extent = newSwapchainSupportInfo.capabilities.minImageExtent;
            newSwapchainSupportInfo.imageCount = newSwapchainSupportInfo.capabilities.minImageCount + 2;
            if (newSwapchainSupportInfo.capabilities.maxImageCount > 0 && newSwapchainSupportInfo.imageCount > newSwapchainSupportInfo.capabilities.maxImageCount)
                newSwapchainSupportInfo.imageCount = newSwapchainSupportInfo.capabilities.maxImageCount;

            if (newSwapchainSupportInfo.imageCount > WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT)
                newSwapchainSupportInfo.imageCount = WIENDER_SWAPCHAIN_IMAGE_MAX_COUNT;

            return newSwapchainSupportInfo;
        }
        WIENDER_NODISCARD VkSurfaceKHR create_platform_spec_surface(const window_handle& whandle) const { // https://cplusplus.com/forum/general/87885/ - reference polymorphism
            VkSurfaceKHR surface = VK_NULL_HANDLE;
#ifdef _WIN32
            VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hinstance = (HINSTANCE)whandle.get_display_handle();
            createInfo.hwnd = (HWND)whandle.get_window_handle();
            vulkan_check(vkCreateWin32SurfaceKHR(instance_, &createInfo, WIENDER_ALLOCATOR_NAME, &surface), "wiender::vulkan_wienderer::create_platform_spec_surface failed to create Vulkan surface");

#elif defined(__linux__)
            const char* sessionType = getenv("XDG_SESSION_TYPE");
#if defined(USE_WAYLAND)
            if (sessionType && strcmp(sessionType, "wayland") == 0) {
                VkWaylandSurfaceCreateInfoKHR createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
                createInfo.display = (wl_display*)whandle.get_display_handle();
                createInfo.surface = (wl_surface*)whandle.get_window_handle();

                if (vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create Wayland surface!");
                }
            } 
#endif
#if 0       
            if (false) {
                VkXlibSurfaceCreateInfoKHR createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
                createInfo.dpy = (Display*)whandle.get_display_handle();
                createInfo.window = (Window)whandle.get_window_handle();

                if (vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create Xlib surface!");
                }
            }
#endif
#elif defined(__APPLE__)
            VkMacOSSurfaceCreateInfoMVK createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
            createInfo.pView = (NSView*)whandle.get_window_handle();
            vulkan_check(vkCreateMacOSSurfaceMVK(instance_, &createInfo, WIENDER_ALLOCATOR_NAME, &surface), "wiender::vulkan_wienderer::create_platform_spec_surface failed to create Vulkan surface");

#else
            throw std::runtime_error("wiender::vulkan_wienderer::create_platform_spec_surface unsupported platform");
#endif
            return surface;
        }
        WIENDER_NODISCARD queue_family_indices create_falimy_indices() const {
            queue_family_indices queueIndeces = find_queue_families(pdevice_, surface_);
            wiender_assert(queueIndeces.is_complete(), "wiender::vulkan_wienderer::create_falimy_indices physical device families indices aren't complete");
            return queueIndeces;
        }
        WIENDER_NODISCARD logical_device_info create_logical_device() const {
            VkDeviceQueueCreateInfo queueCreateInfos[2];

            const float queuePriorities[] =  { 1.0f };
            for (uint32_t i = 0; i < pdevice_.queueIndeces.falimiesCount; ++i) {
                VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[i];
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.pNext = nullptr;
                queueCreateInfo.flags = static_cast<VkFlags>(0);
                queueCreateInfo.queueFamilyIndex = pdevice_.queueIndeces.indeces[i];
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = queuePriorities;
            }

            VkDeviceCreateInfo deviceInfo{};
            deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceInfo.pNext = &pdevice_.features;
            deviceInfo.flags = static_cast<VkFlags>(0);
            deviceInfo.queueCreateInfoCount = pdevice_.queueIndeces.falimiesCount;
            deviceInfo.pQueueCreateInfos = queueCreateInfos;
            deviceInfo.enabledLayerCount = 0;
            deviceInfo.ppEnabledLayerNames = nullptr;
            deviceInfo.enabledExtensionCount = static_cast<uint32_t>(WIENDER_ARRSIZE(stConstants.deviceExtensions));
            deviceInfo.ppEnabledExtensionNames =  stConstants.deviceExtensions;
            deviceInfo.pEnabledFeatures = nullptr;

            logical_device_info result{};
            vulkan_check(vkCreateDevice(pdevice_, &deviceInfo, WIENDER_ALLOCATOR_NAME, &result.device), "wiender::vulkan_wienderer::create_logical_device failed to create logical device");
            vkGetDeviceQueue(result.device, pdevice_.queueIndeces.graphicsFamily, 0, &result.graphicsQueue);
            vkGetDeviceQueue(result.device, pdevice_.queueIndeces.presentFamily, 0, &result.presentQueue);

            return result;

        }
        WIENDER_NODISCARD VkSampleCountFlagBits get_max_usable_sample_count(VkSampleCountFlagBits target) const {
            const auto& properties = pdevice_.properties.properties;

            const VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
            if (counts & target)                    { return target; }
            if (counts & VK_SAMPLE_COUNT_64_BIT)    { return VK_SAMPLE_COUNT_64_BIT; }
            if (counts & VK_SAMPLE_COUNT_32_BIT)    { return VK_SAMPLE_COUNT_32_BIT; }
            if (counts & VK_SAMPLE_COUNT_16_BIT)    { return VK_SAMPLE_COUNT_16_BIT; }
            if (counts & VK_SAMPLE_COUNT_8_BIT)     { return VK_SAMPLE_COUNT_8_BIT; }
            if (counts & VK_SAMPLE_COUNT_4_BIT)     { return VK_SAMPLE_COUNT_4_BIT; }
            if (counts & VK_SAMPLE_COUNT_2_BIT)     { return VK_SAMPLE_COUNT_2_BIT; }

            return VK_SAMPLE_COUNT_1_BIT;
        }
        WIENDER_NODISCARD physical_device_info get_physical_device() const {
            VkPhysicalDevice devices[WIENDER_DEFAULT_ARRAY_SIZE] = {};
            uint32_t deviceCount = 0;
            vulkan_check(vkEnumeratePhysicalDevices(instance_, &deviceCount, 0), "wiender::vulkan_wienderer::get_physical_device failed to enumerate physical devices 1");
            wiender_assert(deviceCount > 0, "wiender::vulkan_wienderer::get_physical_device passed zero devices");
            if (deviceCount > WIENDER_DEFAULT_ARRAY_SIZE)
                deviceCount = WIENDER_DEFAULT_ARRAY_SIZE;
            vulkan_check(vkEnumeratePhysicalDevices(instance_, &deviceCount, devices), "wiender::vulkan_wienderer::get_physical_device failed to enumerate physical devices 2");

            return choose_best_physical_device(devices, deviceCount);
        }
        WIENDER_NODISCARD VkInstance create_vulkan_instance() const {
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
            appInfo.pApplicationName = "_";
            appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
            appInfo.pEngineName = "wiender";
            appInfo.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo instanceInfo{};
            instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceInfo.pApplicationInfo = &appInfo;
            instanceInfo.enabledLayerCount = static_cast<uint32_t>(WIENDER_ARRSIZE(stConstants.validationLayers) - (validationEnable_ ? 0u : 1u));
            instanceInfo.ppEnabledLayerNames = stConstants.validationLayers;
            instanceInfo.enabledExtensionCount = static_cast<uint32_t>(WIENDER_ARRSIZE(stConstants.instanceExtensions) - (validationEnable_ ? 0u : 1u));
            instanceInfo.ppEnabledExtensionNames = stConstants.instanceExtensions;

            VkInstance result;
            vulkan_check(vkCreateInstance(&instanceInfo, WIENDER_ALLOCATOR_NAME, &result), "wiender::vulkan_wienderer::create_vulkan_instance failed to create vulkan instance");
            return result;
        }

        private:
        WIENDER_NODISCARD static queue_family_indices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
            queue_family_indices indices;
            VkQueueFamilyProperties queueFamilies[WIENDER_DEFAULT_ARRAY_SIZE] = {};
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            if (queueFamilyCount > WIENDER_DEFAULT_ARRAY_SIZE)
                queueFamilyCount = WIENDER_DEFAULT_ARRAY_SIZE;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

            for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                VkQueueFamilyProperties queueFamily = queueFamilies[i];
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    indices.graphicsFamily = i;

                VkBool32 presentSupport = false;
                vulkan_check(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport), "wiender::vulkan_wienderer::find_queue_families failed to get surface support");
                if (presentSupport)
                    indices.presentFamily = i;

                if (indices.is_complete()) 
                    break;
                ++i;
            }

            indices.falimiesCount = (indices.graphicsFamily == indices.presentFamily) ? 1 : 2;
            return indices;
        }
        WIENDER_NODISCARD static physical_device_info choose_best_physical_device(VkPhysicalDevice* devices, uint32_t deviceCount) {
            const auto scoreFromDeviceType = [](VkPhysicalDeviceType type) {
                switch (type) {
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    return 1;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return 3;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return 15;
                default:
                    return 0;
                    break;
                }
                return 0;
            };
            physical_device_info bestDevice{};
            int bestDeviceScore = 0;
            const VkPhysicalDevice* end = devices + deviceCount;
            for (;devices != end; ++devices) {
                VkPhysicalDevice device = *devices;

                VkPhysicalDeviceFeatures2 features2{};
                features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                const VkPhysicalDeviceFeatures& features = features2.features;

                VkPhysicalDeviceProperties2 properties2{};
                properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                const VkPhysicalDeviceProperties& properties = properties2.properties;

                vkGetPhysicalDeviceFeatures2(device, &features2);
                vkGetPhysicalDeviceProperties2(device, &properties2);
                const int currentDeviceScore = 
                    (features.largePoints)  + (features.wideLines * 3) + (features.tessellationShader * 4) + (features.geometryShader * 5) +
                    scoreFromDeviceType(properties.deviceType) + (properties.limits.maxUniformBufferRange / sizeof(float[4])) + (properties.limits.maxVertexInputBindings);
                if (currentDeviceScore > bestDeviceScore) {
                    bestDeviceScore = currentDeviceScore;
                    bestDevice = {device, features2, properties2, {}};
                }
            }
            wiender_assert(bestDeviceScore > 0, "wiender::vulkan_wienderer::choose_best_physical_device physical device not found");

            bestDevice.indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES; // TODO: feature check
            bestDevice.indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
            bestDevice.features.pNext = &bestDevice.indexingFeatures;

            return bestDevice;
        }

    };

    struct cpu_side_buffer final : public buffer {
        private:
        vulkan_wienderer* owner_;
        VkDeviceMemory CPUMemory_;
        VkBuffer CPUBuffer_;
        std::size_t size_;
        VkBufferUsageFlags usage_;
        bool mappedFlag_;

        public:
        cpu_side_buffer(vulkan_wienderer* owner, std::size_t sizeb, VkBufferUsageFlags usage) : owner_(owner), CPUMemory_{}, CPUBuffer_{}, size_(sizeb), usage_(usage), mappedFlag_(false) {
            wiender_assert(owner_ != nullptr, "wiender::cpu_side_buffer::cpu_side_buffer owner cannot be nullptr");

            try {
                CPUMemory_ = create_cpu_memory();

                CPUBuffer_ = create_cpu_buffer();
            } catch (...) {
                accurate_destroy();
                throw;
            }
        }

        public:
        ~cpu_side_buffer() override {
            accurate_destroy();
        }

        public:
        WIENDER_NODISCARD bool is_mapped() const noexcept override {
            return mappedFlag_;
        }
        void bind() override {
            if (usage_ & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
                owner_->bind_vertex_buffer_state(create_buffer_state());
            }
            if (usage_ & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
                owner_->bind_index_buffer_state(create_buffer_state());
            }
        }
        void* map() override  {
            wiender_assert(!mappedFlag_, "wiender::cpu_side_buffer::map buffer already mapped");
            void* data;
            vulkan_check(vkMapMemory(owner_->get_ldevice(), CPUMemory_, 0, VK_WHOLE_SIZE, 0, &data), "wiender::cpu_side_buffer::map failed to map memory");
            mappedFlag_ = true;
            return data;
        }
        void unmap() override  {
            wiender_assert(mappedFlag_, "wiender::cpu_side_buffer::unmap stagingMemory_ is not mapped");
            vkUnmapMemory(owner_->get_ldevice(), CPUMemory_);
            mappedFlag_ = false;
        }
        void update_data() override {
            // nothing here
        }

        private:
        WIENDER_NODISCARD binded_buffer_state create_buffer_state() const {
            return binded_buffer_state{ CPUBuffer_ };
        }

        private:
        void accurate_destroy() {
            vkDeviceWaitIdle(owner_->get_ldevice());

            if (CPUBuffer_ != 0) {
                vkDestroyBuffer(owner_->get_ldevice(), CPUBuffer_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
            if (CPUMemory_ != 0) {
                vkFreeMemory(owner_->get_ldevice(), CPUMemory_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
        }
        WIENDER_NODISCARD VkBuffer create_cpu_buffer() const {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size_;
            bufferInfo.pQueueFamilyIndices = &owner_->get_pdevice().queueIndeces.graphicsFamily;
            bufferInfo.queueFamilyIndexCount = 1;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            bufferInfo.usage = usage_;

            VkBuffer result;
            vulkan_check(vkCreateBuffer(owner_->get_ldevice(), &bufferInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::cpu_side_buffer::create_cpu_buffer failed to create buffer");
            vulkan_check(vkBindBufferMemory(owner_->get_ldevice(), result, CPUMemory_, 0), "wiender::cpu_side_buffer::create_cpu_buffer failed to bind cpu memory");

            return result;
        }
        WIENDER_NODISCARD VkDeviceMemory create_cpu_memory() const {
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.memoryTypeIndex = owner_->find_memory_type(usage_, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            allocInfo.allocationSize = size_;

            VkDeviceMemory result;
            vulkan_check(vkAllocateMemory(owner_->get_ldevice(), &allocInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::cpu_side_buffer::create_cpu_memory failed to create cpu memory");
            return result;
        }
    };
    WIENDER_NODISCARD std::unique_ptr<buffer> create_cpu_side_buffer(vulkan_wienderer* owner, std::size_t sizeb, VkBufferUsageFlags usage) {
        return std::unique_ptr<cpu_side_buffer>(new cpu_side_buffer(owner, sizeb, usage));
    }



    struct gpu_side_buffer final : public buffer {
        private:
        vulkan_wienderer* owner_;
        VkDeviceMemory GPUMemory_;
        VkBuffer GPUBuffer_;
        VkDeviceMemory stagingMemory_;
        VkBuffer stagingBuffer_;
        std::size_t size_;
        VkBufferUsageFlags usage_;
        bool mappedFlag_;

        public:
        gpu_side_buffer(vulkan_wienderer* owner, std::size_t sizeb, VkBufferUsageFlags usage) : owner_(owner), GPUMemory_{}, GPUBuffer_{}, stagingMemory_{}, stagingBuffer_{}, size_(sizeb), usage_(usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT), mappedFlag_(false) {
            wiender_assert(owner_ != nullptr, "wiender::gpu_side_buffer::gpu_side_buffer owner cannot be nullptr");

            try {
                GPUMemory_ = create_gpu_memory();

                GPUBuffer_ = create_gpu_buffer();
            } catch (...) {
                accurate_destroy();
                throw;
            }
        }

        public:
        ~gpu_side_buffer() override {
            accurate_destroy();
        }

        public:
        WIENDER_NODISCARD bool is_mapped() const noexcept override {
            return mappedFlag_;
        }
        void bind() override {
            if (usage_ & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
                owner_->bind_vertex_buffer_state(create_buffer_state());
            }
            if (usage_ & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
                owner_->bind_index_buffer_state(create_buffer_state());
            }
        }
        void* map() override  {
            wiender_assert(!is_mapped(), "wiender::gpu_side_buffer::map buffer already mapped");
            if (stagingBuffer_ == 0) {
                stagingMemory_ = create_staging_memory();
                stagingBuffer_ = create_staging_buffer();
            }
            void* data;
            vulkan_check(vkMapMemory(owner_->get_ldevice(), stagingMemory_, 0, VK_WHOLE_SIZE, 0, &data), "wiender::gpu_side_buffer::map failed to map memory");
            mappedFlag_ = true;
            return data;
        }
        void unmap() override  {
            wiender_assert(is_mapped(), "wiender::gpu_side_buffer::unmap buffer is not mapped");
            vkUnmapMemory(owner_->get_ldevice(), stagingMemory_);
            mappedFlag_ = false;
        }
        void update_data() override {
            VkCommandBuffer cmdbuff = owner_->begin_single_time_commands();

            owner_->copy_buffer(cmdbuff, stagingBuffer_, GPUBuffer_, size_);

            owner_->end_single_time_commands(cmdbuff);
        }

        private:
        WIENDER_NODISCARD binded_buffer_state create_buffer_state() const {
            return binded_buffer_state{ GPUBuffer_ };
        }

        private:
        void accurate_destroy() {
            vkDeviceWaitIdle(owner_->get_ldevice());

            if (stagingBuffer_ != 0) {
                vkDestroyBuffer(owner_->get_ldevice(), stagingBuffer_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
            if (stagingMemory_ != 0) {
                vkFreeMemory(owner_->get_ldevice(), stagingMemory_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
            if (GPUBuffer_ != 0) {
                vkDestroyBuffer(owner_->get_ldevice(), GPUBuffer_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
            if (GPUMemory_ != 0) {
                vkFreeMemory(owner_->get_ldevice(), GPUMemory_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
        }
        WIENDER_NODISCARD VkBuffer create_gpu_buffer() const {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size_;
            bufferInfo.pQueueFamilyIndices = &owner_->get_pdevice().queueIndeces.graphicsFamily;
            bufferInfo.queueFamilyIndexCount = 1;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            bufferInfo.usage = usage_;

            VkBuffer result;
            vulkan_check(vkCreateBuffer(owner_->get_ldevice(), &bufferInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::gpu_side_buffer::create_gpu_buffer failed to create buffer");
            vulkan_check(vkBindBufferMemory(owner_->get_ldevice(), result, GPUMemory_, 0), "wiender::gpu_side_buffer::create_gpu_buffer failed to bind gpu memory");

            return result;
        }
        WIENDER_NODISCARD VkDeviceMemory create_gpu_memory() const {
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.memoryTypeIndex = owner_->find_memory_type(usage_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            allocInfo.allocationSize = size_;

            VkDeviceMemory result;
            vulkan_check(vkAllocateMemory(owner_->get_ldevice(), &allocInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::gpu_side_buffer::create_gpu_memory failed to create gpu memory");
            return result;
        }
        WIENDER_NODISCARD VkDeviceMemory create_staging_memory() const {
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = size_;
            allocInfo.memoryTypeIndex = owner_->find_memory_type(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            VkDeviceMemory result;
            vulkan_check(vkAllocateMemory(owner_->get_ldevice(), &allocInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::gpu_side_buffer::create_staging_memory failed to allocate staging memory");

            return result;
        }
        WIENDER_NODISCARD VkBuffer create_staging_buffer() const {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size_;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkBuffer result;
            vulkan_check(vkCreateBuffer(owner_->get_ldevice(), &bufferInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::gpu_side_buffer::create_staging_buffer failed to create staging buffer");
            vulkan_check(vkBindBufferMemory(owner_->get_ldevice(), result, stagingMemory_, 0), "wiender::gpu_side_buffer::create_staging_buffer failed to bind staging memory");

            return result;
        }
    };
    std::unique_ptr<buffer> create_gpu_side_buffer(vulkan_wienderer* owner, std::size_t sizeb, VkBufferUsageFlags usage) {
        return std::unique_ptr<gpu_side_buffer>(new gpu_side_buffer(owner, sizeb, usage));
    }

    struct image_texture : public texture {
        public:
        vulkan_wienderer* owner_;
        vulkan_image image_;
        VkSampler sampler_;
        VkDeviceMemory stagingMemory_;
        VkBuffer stagingBuffer_;
        VkExtent3D extent_;

        public:
        image_texture(vulkan_wienderer* owner, const create_info& createInfo) 
            :   owner_(owner),
                image_{},
                sampler_{},
                stagingMemory_{},
                stagingBuffer_{},
                extent_{ createInfo.textureExtent.width, createInfo.textureExtent.height, createInfo.textureExtent.depth } {
            wiender_assert(owner_ != nullptr, "wiender::image_texture::image_texture owner cannot be nullptr");

            try {
                image_ = create_image();

                sampler_ = create_sampler(createInfo);
                
                VkCommandBuffer cmdbuff = owner_->begin_single_time_commands();
                owner_->transition_image_layout(cmdbuff, image_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                owner_->end_single_time_commands(cmdbuff);


            } catch (...) {
                accurate_destroy();
                throw;
            }
        }

        public:
        WIENDER_NODISCARD extent get_extent() const noexcept override {
            return extent(extent_.width, extent_.height, extent_.depth);
        }
        WIENDER_NODISCARD bool is_mapped() const noexcept override {
            return stagingMemory_ != 0;
        }
        WIENDER_NODISCARD void* map() override {
            wiender_assert(!is_mapped(), "wiender image_texture::map texture staging memory already mapped");

            stagingMemory_ = create_staging_memory();

            stagingBuffer_ = create_staging_buffer();

            void* data;
            vkMapMemory(owner_->get_ldevice(), stagingMemory_, 0, get_size(), static_cast<VkFlags>(0), &data);
            return data;

        }
        void unmap() override {
            wiender_assert(is_mapped(), "wiender image_texture::unmap texture staging memory not mapped");

            vkUnmapMemory(owner_->get_ldevice(), stagingMemory_);
            vkFreeMemory(owner_->get_ldevice(), stagingMemory_, WIENDER_CHILD_ALLOCATOR_NAME);
            vkDestroyBuffer(owner_->get_ldevice(), stagingBuffer_, WIENDER_CHILD_ALLOCATOR_NAME);
            stagingMemory_ = 0;
            stagingBuffer_ = 0;
        }
        void update_data() override {
            VkCommandBuffer cmdbuff = owner_->begin_single_time_commands();

            owner_->transition_image_layout(cmdbuff, image_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            owner_->copy_buffer_to_image(cmdbuff, stagingBuffer_, image_.image, extent_.width, extent_.height, extent_.depth);
            owner_->transition_image_layout(cmdbuff, image_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            owner_->end_single_time_commands(cmdbuff);
        }

        public:
        ~image_texture() override {
            accurate_destroy();
        }

        private:
        void accurate_destroy() {
            vkDeviceWaitIdle(owner_->get_ldevice());

            if (stagingBuffer_ != 0) {
                vkDestroyBuffer(owner_->get_ldevice(), stagingBuffer_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
            if (stagingMemory_ != 0) {
                vkUnmapMemory(owner_->get_ldevice(), stagingMemory_);
                vkFreeMemory(owner_->get_ldevice(), stagingMemory_, WIENDER_CHILD_ALLOCATOR_NAME);
            }

            if (sampler_ != 0) {
                vkDestroySampler(owner_->get_ldevice(), sampler_, WIENDER_CHILD_ALLOCATOR_NAME);
            }
            owner_->destroy_vulkan_image(image_);
        }

        public:
        WIENDER_NODISCARD VkSampler get_sampler() const noexcept {
            return sampler_;
        }
        WIENDER_NODISCARD VkImageView get_view() const noexcept {
            return image_.view;
        }

        private:
        WIENDER_NODISCARD VkDeviceMemory create_staging_memory() const {
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = get_size();
            allocInfo.memoryTypeIndex = owner_->find_memory_type(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            VkDeviceMemory result;
            vulkan_check(vkAllocateMemory(owner_->get_ldevice(), &allocInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::image_texture::create_staging_memory failed to allocate staging memory");

            return result;
        }
        
        WIENDER_NODISCARD VkBuffer create_staging_buffer() const {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = get_size();
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkBuffer result;
            vulkan_check(vkCreateBuffer(owner_->get_ldevice(), &bufferInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::image_texture::create_staging_buffer failed to create staging buffer");
            vulkan_check(vkBindBufferMemory(owner_->get_ldevice(), result, stagingMemory_, 0), "wiender::image_texture::create_staging_buffer failed to bind staging memory");

            return result;
        }
        WIENDER_NODISCARD VkDeviceSize get_size() const noexcept {
            return extent_.width * (std::max(extent_.height, 1u)) * (std::max(extent_.depth, 1u)) * 4;
        }
        VkSampler create_sampler(const create_info& createInfo) const {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
         // samplerInfo.pNext = nullptr;
         // samplerInfo.flags = static_cast<VkFlags>(0);
            samplerInfo.magFilter = texture_sampler_filter_to_vk_filter(createInfo.filter);
            samplerInfo.minFilter = texture_sampler_filter_to_vk_filter(createInfo.filter);
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.mipLodBias = 0.0f;
         // samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
         // samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 1.0f;
            samplerInfo.borderColor = {};
         // samplerInfo.unnormalizedCoordinates = VK_FALSE;

            VkSampler result;
            vulkan_check(vkCreateSampler(owner_->get_ldevice(), &samplerInfo, 0, &result), "wiender::image_texture::create_sampler failed to create sampler");
            return result;
        }
        vulkan_image create_image() const {
            return owner_->create_vulkan_image(
                extent_.width, extent_.height,
                1,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_SAMPLE_COUNT_1_BIT,
                owner_->get_swapcahin_image_format().format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
        }

    };
    WIENDER_NODISCARD std::unique_ptr<texture> create_image_texture(vulkan_wienderer* owner, const texture::create_info& createInfo) {
        return std::unique_ptr<image_texture>(new image_texture(owner, createInfo));
    }

    struct vulkan_shader final : public shader {
        private:
        struct uniform_buffers_info {
            VkDeviceMemory memory; // single memory block for each buffer but with different offsets
            struct {
                VkBuffer buffer;
                VkDeviceSize size;
            } buffers[WIENDER_UNIFORM_BUFFER_MAX_COUNT]{};
            void* mappedMemory = nullptr;
        };

        public:
        vulkan_wienderer* owner_;
        uniform_buffers_info uniformBuffers_;
        VkDescriptorPool descriptorPool_;
        VkDescriptorSetLayout descriptorSetLayout_;
        VkDescriptorSet descriptorSet_;
        VkRenderPass renderPass_;
        VkPipelineLayout pipelineLayout_;
        VkPipeline pipeline_;

        public:
        vulkan_shader(vulkan_wienderer* owner, const create_info& createInfo) :
            owner_(owner),
            uniformBuffers_{},
            descriptorPool_{},
            descriptorSetLayout_{},
            descriptorSet_{},
            renderPass_{},
            pipelineLayout_{},
            pipeline_{} {

            if (owner_ == nullptr) {
                throw std::runtime_error("wiender::vulkan_shader::vulkan_shader owner cannot be nullptr");
            }

            try {
                std::vector<descriptor_set_layout_data> descriptorsInfos;
                for (const auto& stage : createInfo.stages) {
                    get_descriptor_sets(descriptorsInfos, stage.data.size() * sizeof(uint32_t), stage.data.data());
                }
                wiender_assert(descriptorsInfos.size() == 1, "wiender::vulkan_shader::vulkan_shader multiple sets not supported");

                descriptor_set_layout_data& descriptorsInfo = descriptorsInfos.front();

                uniformBuffers_ = create_uniform_buffers(descriptorsInfo);

                descriptorPool_ = create_descriptor_pool(descriptorsInfo);

                descriptorSetLayout_ = create_descriptor_set_layout(descriptorsInfo);

                descriptorSet_ = create_descriptor_set(descriptorsInfo);
                
                renderPass_ = create_render_pass(createInfo);

                pipelineLayout_ = create_pipeline_layout();

                pipeline_ = create_pipeline(createInfo);

            } catch (...) {
                accurate_destroy();
                throw;
            }

        }

        public:
        ~vulkan_shader() override {
            accurate_destroy();
        }

        public:
        void set() override {
            owner_->set_shader_state(get_shader_state());
        }
        WIENDER_NODISCARD uniform_buffer_info get_uniform_buffer_info(std::size_t binding) override {
            wiender_assert(binding < WIENDER_UNIFORM_BUFFER_MAX_COUNT, "wiender::vulkan_shader::get_uniform_buffer_info binding has to be less than " WIENDER_TOSTRING(WIENDER_UNIFORM_BUFFER_MAX_COUNT));
            VkDeviceSize offset = 0;
            uint32_t i = 0;
            for (; i < binding; ++i) 
                offset += uniformBuffers_.buffers[i].size;
            wiender_assert(uniformBuffers_.buffers[i].size != 0, "wiender::vulkan_shader::get_uniform_buffer_info buffer on this binding does not exist");
            return uniform_buffer_info{ uniformBuffers_.buffers[i].size, reinterpret_cast<void*>((char*)uniformBuffers_.mappedMemory + offset) }; 
        }
        void bind_texture(std::size_t binding, std::size_t arrayIndex, const texture* tetr) override {
            wiender_assert(tetr != nullptr, "wiender::vulkan_shader::bind_texture failed to bind invalid texture");

            image_texture* itetr = (image_texture*)tetr;

            VkDescriptorImageInfo imageInfo = { itetr->get_sampler(), itetr->get_view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet descriptorWrites[1];
            descriptorWrites[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrites[0].dstSet = descriptorSet_;
            descriptorWrites[0].dstBinding = binding;
            descriptorWrites[0].dstArrayElement = arrayIndex;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[0].pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(owner_->get_ldevice(), WIENDER_ARRSIZE(descriptorWrites), descriptorWrites, 0, 0);
        }

        private:
        WIENDER_NODISCARD active_shader_state get_shader_state() const noexcept {
            return active_shader_state {
                pipeline_,
                pipelineLayout_,
                renderPass_,
                descriptorSet_,
            };
        }

        private:
        void accurate_destroy() {
            vkDeviceWaitIdle(owner_->get_ldevice());

            if (pipeline_ != 0)
                vkDestroyPipeline(owner_->get_ldevice(), pipeline_, WIENDER_CHILD_ALLOCATOR_NAME);
            if (pipelineLayout_ != 0)
                vkDestroyPipelineLayout(owner_->get_ldevice(), pipelineLayout_, WIENDER_CHILD_ALLOCATOR_NAME);
            if (renderPass_ != 0)
                vkDestroyRenderPass(owner_->get_ldevice(), renderPass_, WIENDER_CHILD_ALLOCATOR_NAME);
            
            for (auto i : uniformBuffers_.buffers) {
                if (i.buffer != 0)
                    vkDestroyBuffer(owner_->get_ldevice(), i.buffer, WIENDER_CHILD_ALLOCATOR_NAME);
            }
            if (uniformBuffers_.mappedMemory != 0)
                vkUnmapMemory(owner_->get_ldevice(), uniformBuffers_.memory);
            if (uniformBuffers_.memory != 0)
                vkFreeMemory(owner_->get_ldevice(), uniformBuffers_.memory, WIENDER_CHILD_ALLOCATOR_NAME);
                
            if (descriptorSetLayout_ != 0)
                vkDestroyDescriptorSetLayout(owner_->get_ldevice(), descriptorSetLayout_, WIENDER_CHILD_ALLOCATOR_NAME);
            if (descriptorPool_ != 0)
                vkDestroyDescriptorPool(owner_->get_ldevice(), descriptorPool_, WIENDER_CHILD_ALLOCATOR_NAME);
        }

        private:
        WIENDER_NODISCARD VkPipeline create_pipeline(const create_info& createInfo) const {
            const VkAllocationCallbacks* allocationCallbacks = nullptr;
            wiender_assert(!createInfo.stages.empty(), "wiender::vulkan_shader::create_pipeline no shader stages for shader program");

            std::vector<VkPipelineShaderStageCreateInfo> shaderStages(createInfo.stages.size());

            for (uint32_t i = 0; i < createInfo.stages.size(); ++i) {
                VkShaderModuleCreateInfo shaderModuleCreateInfo{};
                shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
             // shaderModuleCreateInfo.pNext = nullptr;
             // shaderModuleCreateInfo.flags = static_cast<VkFlags>(0);
                shaderModuleCreateInfo.codeSize = createInfo.stages[i].data.size() * sizeof(uint32_t);
                shaderModuleCreateInfo.pCode = createInfo.stages[i].data.data();

                vulkan_check(vkCreateShaderModule(owner_->get_ldevice(), &shaderModuleCreateInfo, allocationCallbacks, &shaderStages[i].module), "wiender::vulkan_shader::create_pipeline failed to create shader module");
                shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
             // shaderStages[i].pNext = nullptr;
             // shaderStages[i].flags = static_cast<VkFlags>(0);
                shaderStages[i].stage = shader_stage_kind_to_vk_shader_stage(createInfo.stages[i].stageKind);
                shaderStages[i].pName = "main";
                shaderStages[i].pSpecializationInfo = {};
            }
            uint32_t inputSize = 0;
            for (const auto& vinputAttribute: createInfo.vertexInputAttributes)
                inputSize += static_cast<uint32_t>(sizeof_shader_vertex_input_attribute_format(vinputAttribute.inputFormat));

            VkVertexInputBindingDescription inputBinding{};
         // inputBinding.binding = 0;
            inputBinding.stride = inputSize;
            inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributes(createInfo.vertexInputAttributes.size());
            for (size_t i = 0; i < vkVertexInputAttributes.size(); ++i) {
                const auto& iattr = createInfo.vertexInputAttributes[i];

                auto& vkdesc = vkVertexInputAttributes[i];
                vkdesc.location = iattr.location;
                vkdesc.binding = iattr.binding;
                vkdesc.format = shader_vertex_input_attribute_format_to_vk_format(iattr.inputFormat);
                vkdesc.offset = iattr.offset;
            }

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
         // vertexInputInfo.pNext = nullptr;
         // vertexInputInfo.flags = static_cast<VkFlags>(0);
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &inputBinding;
            vertexInputInfo.vertexAttributeDescriptionCount = vkVertexInputAttributes.size();
            vertexInputInfo.pVertexAttributeDescriptions = vkVertexInputAttributes.data();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
         // inputAssembly.pNext = nullptr;
         // inputAssembly.flags = static_cast<VkFlags>(0);
            inputAssembly.topology = shader_primitive_topology_to_vk_primitive_topology(createInfo.topology);
            inputAssembly.primitiveRestartEnable = VK_FALSE;
            
            const VkRect2D scissors {
                {0, 0},
                owner_->get_swapchain_extent(),
            };
            const VkViewport view {
                0.0f, 0.0f,
                (float)owner_->get_swapchain_extent().width, (float)owner_->get_swapchain_extent().height,
                0.0f, 1.0f,
            };
            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
         // viewportState.pNext = nullptr;
         // viewportState.flags = static_cast<VkFlags>(0);
            viewportState.viewportCount = 1;
            viewportState.pViewports = &view;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissors;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
         // rasterizer.pNext = nullptr;
         // rasterizer.flags = static_cast<VkFlags>(0);
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = shader_polygon_mode_to_vk_polygon_mode(createInfo.polygonMode);
            rasterizer.cullMode = shader_cull_mode_to_vk_cull_mode(createInfo.cullMode);
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;
         // rasterizer.depthBiasConstantFactor = 0;
         // rasterizer.depthBiasClamp = 0;
         // rasterizer.depthBiasSlopeFactor = 0;
            rasterizer.lineWidth = 1.0f; //createInfo.lineWidth;

            
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
         // multisampling.pNext = nullptr;
         // multisampling.flags = static_cast<VkFlags>(0); 
            multisampling.rasterizationSamples = owner_->get_msaa_samples();
            multisampling.sampleShadingEnable = owner_->is_multisampling_enabled();
         // multisampling.minSampleShading = 0;
         // multisampling.pSampleMask = 0;
            multisampling.alphaToCoverageEnable = VK_FALSE;
            multisampling.alphaToOneEnable = VK_FALSE;

            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
         // depthStencil.pNext = nullptr;
         // depthStencil.flags = static_cast<VkFlags>(0);
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;
         // depthStencil.front = {};
         // depthStencil.back  = {};
         // depthStencil.minDepthBounds = 0.0f;
            depthStencil.maxDepthBounds = 1.0f;


            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.blendEnable = static_cast<VkBool32>(createInfo.alphaBlend);
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
         // colorBlending.pNext = nullptr;
         // colorBlending.flags = static_cast<VkFlags>(0);
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
         // colorBlending.blendConstants = {};

            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
         // dynamicState.pNext = nullptr;
         // dynamicState.flags = static_cast<VkFlags>(0);
         // dynamicState.dynamicStateCount = createInfo.dynamicStateCount;   TODO
         // dynamicState.pDynamicStates = createInfo.dynamicStates;

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
         // pipelineInfo.pNext = nullptr;
         // pipelineInfo.flags = static_cast<VkFlags>(0);
            pipelineInfo.stageCount = shaderStages.size();
            pipelineInfo.pStages = shaderStages.data();
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pTessellationState = nullptr;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = false ? &depthStencil : nullptr; // TODO
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.layout = pipelineLayout_;
            pipelineInfo.renderPass = renderPass_;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.basePipelineIndex = 0;

            VkPipeline newPipeline;
            vulkan_check(vkCreateGraphicsPipelines(owner_->get_ldevice(), VK_NULL_HANDLE, 1, &pipelineInfo, allocationCallbacks, &newPipeline), "wienderer::vulkan_shader::create_pipeline failed to create create pipeline");
            for (const auto& stage : shaderStages)
                vkDestroyShaderModule(owner_->get_ldevice(), stage.module, allocationCallbacks);

            return newPipeline;
        }
        WIENDER_NODISCARD VkPipelineLayout create_pipeline_layout() const {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
         // pipelineLayoutCreateInfo.pNext = nullptr;
         // pipelineLayoutCreateInfo.flags = static_cast<VkFlags>(0);
            pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayout_ ? 1u : 0u;
            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout_ ? &descriptorSetLayout_ : nullptr;
         // pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
         // pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

            VkPipelineLayout newPipelineLayout;
            vulkan_check(vkCreatePipelineLayout(owner_->get_ldevice(), &pipelineLayoutCreateInfo, WIENDER_CHILD_ALLOCATOR_NAME, &newPipelineLayout), "wienderer::vulkan_shader::create_pipeline_layout failed to create create pipeline layout");
            return newPipelineLayout;
        }
        WIENDER_NODISCARD VkRenderPass create_render_pass(const create_info& createInfo) const {
            return owner_->create_default_render_pass(createInfo.clearScreen ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE);
        }
        WIENDER_NODISCARD VkDescriptorSet create_descriptor_set(const descriptor_set_layout_data& descriptorInfo) {
            VkDescriptorSetAllocateInfo allocationInfo{};
            allocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
         // allocationInfo.pNext = nullptr;
            allocationInfo.descriptorPool = descriptorPool_;
            allocationInfo.descriptorSetCount = 1;
            allocationInfo.pSetLayouts = &descriptorSetLayout_;

            VkDescriptorSet result;
            vulkan_check(vkAllocateDescriptorSets(owner_->get_ldevice(), &allocationInfo, &result), "wiender::vulkan_shader::create_descriptor_set failed to allocate descriptor set");

            for (size_t i = 0; i < descriptorInfo.bindings.size(); ++i) {
                const auto& setBinding = descriptorInfo.bindings[i];
                const auto& bindingBufferInfo = descriptorInfo.bufferSizes[i];

                const uint32_t binding = setBinding.binding;

                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.range = bindingBufferInfo;
                bufferInfo.offset = 0;
                bufferInfo.buffer = uniformBuffers_.buffers[binding].buffer;

                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = owner_->get_default_sampler();
                imageInfo.imageView = owner_->get_default_texture_image().view;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkWriteDescriptorSet write{};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
             // write.pNext = nullptr;
                write.dstSet = result;
                write.dstBinding = binding;
                write.dstArrayElement = 0;
                write.descriptorCount = setBinding.descriptorCount;
                write.pTexelBufferView = nullptr;
                write.descriptorType = setBinding.descriptorType;

                if (
                    (setBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
                    (setBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                    (setBinding.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) ||
                    (setBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) ||
                    (setBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)) {
                    write.pBufferInfo = &bufferInfo;

                } else if (setBinding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
                    write.pImageInfo = &imageInfo;
                    
                } else {
                    continue;
                }

                vkUpdateDescriptorSets(owner_->get_ldevice(), 1, &write, 0, nullptr);
            }
            return result;
        }

        WIENDER_NODISCARD VkDescriptorSetLayout create_descriptor_set_layout(const descriptor_set_layout_data& descriptorInfo) { // TODO: multiple sets
            const auto& bindings = descriptorInfo.bindings;
            std::vector<VkDescriptorBindingFlags> flags;

            flags.resize(bindings.size());
            for (size_t i = 0; i < flags.size(); ++i) {
                if (bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                    flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
                } else {
                    flags[i] = static_cast<VkFlags>(0);
                }
            }

            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingsFlags{};
            bindingsFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            bindingsFlags.pBindingFlags = flags.data();
            bindingsFlags.bindingCount = flags.size();

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = descriptorInfo.createInfo;
            descriptorSetLayoutCreateInfo.pNext = &bindingsFlags;
            descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

            VkDescriptorSetLayout result;
            vulkan_check(vkCreateDescriptorSetLayout(owner_->get_ldevice(), &descriptorSetLayoutCreateInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::vulka_shader::create_descriptor_set_layout failed to create descriptors set layout");
            return result;
        }
        WIENDER_NODISCARD VkDescriptorPool create_descriptor_pool(const descriptor_set_layout_data& descriptorInfo) const { // TODO: multiple sets
            std::vector<VkDescriptorPoolSize> poolSizes;

            for (const auto& bindingInfo : descriptorInfo.bindings) {
                poolSizes.emplace_back(VkDescriptorPoolSize{bindingInfo.descriptorType, bindingInfo.descriptorCount});
            }
            
            VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
            descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
         // descriptorPoolCreateInfo.pNext = nullptr;
            descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;//static_cast<VkFlags>(0); WATCH
            descriptorPoolCreateInfo.maxSets = 1;
            descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
            descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

            VkDescriptorPool result;
            vulkan_check(vkCreateDescriptorPool(owner_->get_ldevice(), &descriptorPoolCreateInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result), "wiender::vulkan_shader::create_descriptor_pool failed to create descriptor pool");
            return result;
        }
        WIENDER_NODISCARD uniform_buffers_info create_uniform_buffers(const descriptor_set_layout_data& descriptorsInfo) const { // TODO: multiple sets
            uniform_buffers_info result;
            for (size_t i = 0; i < descriptorsInfo.bindings.size(); ++i) {
                const auto& bindingInfo = descriptorsInfo.bindings[i];
                const auto& bufferSize = descriptorsInfo.bufferSizes[i];
                if (bindingInfo.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    continue;
                wiender_assert(bindingInfo.binding < WIENDER_UNIFORM_BUFFER_MAX_COUNT, "wiender::vulkan_shader::create_uniform_buffers uniform buffer binding has to be less than" WIENDER_TOSTRING(WIENDER_UNIFORM_BUFFER_MAX_COUNT));
                wiender_assert(bufferSize != 0, "wiender::vulkan_shader::create_uniform_buffers uniform buffer size must be more than zero");
                result.buffers[bindingInfo.binding].size = bufferSize >= 128 ? bufferSize : 128; // 128 - maximum align
            }
            
            VkDeviceSize totalBufferSize = 0;
            for (uint32_t i = 0; i < WIENDER_UNIFORM_BUFFER_MAX_COUNT; ++i)
                totalBufferSize += result.buffers[i].size;
            if (totalBufferSize == 0) {
                return uniform_buffers_info{};
            }

            VkMemoryAllocateInfo allocationInfo{};
            allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
         // allocationInfo.pNext = nullptr;
            allocationInfo.allocationSize = totalBufferSize;
            allocationInfo.memoryTypeIndex = owner_->find_memory_type(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            
            vulkan_check(vkAllocateMemory(owner_->get_ldevice(), &allocationInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result.memory), "wiender::vulkan_shader::create_uniform_buffers failed to allocate memoey for uniform buffers");

            VkDeviceSize offset = 0;
            for (uint32_t i = 0; i < WIENDER_UNIFORM_BUFFER_MAX_COUNT; ++i)  {
                if (result.buffers[i].size == 0)
                    continue;
                
                VkBufferCreateInfo bufferCreateInfo{};
                bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
             // bufferCreateInfo.pNext = nullptr;
             // bufferCreateInfo.flags = static_cast<VkFlags>(0); //VK_BUFFER_CREATE_SPARSE_ALIASED_BIT | VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
                bufferCreateInfo.size = result.buffers[i].size;
                bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                bufferCreateInfo.queueFamilyIndexCount = 1;
                bufferCreateInfo.pQueueFamilyIndices = &owner_->get_pdevice().queueIndeces.graphicsFamily;

                vulkan_check(vkCreateBuffer(owner_->get_ldevice(), &bufferCreateInfo, WIENDER_CHILD_ALLOCATOR_NAME, &result.buffers[i].buffer), "wiender::vulkan_shader::create_uniform_buffers");
                vulkan_check(vkBindBufferMemory(owner_->get_ldevice(), result.buffers[i].buffer, result.memory, offset), "wiender::vulkan_shader::create_uniform_buffers");
                offset += result.buffers[i].size;
            }
            vulkan_check(vkMapMemory(owner_->get_ldevice(), result.memory, 0, totalBufferSize, static_cast<VkFlags>(0), &result.mappedMemory), "wiender::vulkan_shader::create_uniform_buffers");
            return result;
        }
    };
    WIENDER_NODISCARD std::unique_ptr<shader> create_vulkan_shader(vulkan_wienderer* owner, const shader::create_info& createInfo) {
        return std::unique_ptr<vulkan_shader>(new vulkan_shader(owner, createInfo));
    }
    
} // namespace wiender