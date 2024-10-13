/*  wirender.cpp
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
#include "wirender.hpp"
#include <cassert>
#include <iostream>
#include <unordered_map>
//#include <vk_enum_string_helper.h>
#if (defined __WIN32)
#   include <vulkan/vulkan_win32.h>
#else
#   include <vulkan/vulkan_xlib.h>
#endif
using namespace wirender;

#define RENDER_ASSERT(expr__, msg) do { if ((expr__) == 0) { _assert(msg, __FILE__, __LINE__); } } while(0)
#define RENDER_ARRAY_SIZE(arr__) (sizeof(arr__) / sizeof(arr__[0])) 

/// watch: https://registry.khronos.org/vulkan/specs/1.3-instanceExtensions/man/html/VkResult.html
#define RENDER_VK_CHECK(vkrexpr___) do { const VkResult vkr__ = vkrexpr___; RENDER_ASSERT(\
        (vkr__ == VK_SUCCESS) || (vkr__ == VK_NOT_READY) || (vkr__ == VK_TIMEOUT) ||\
        (vkr__ == VK_EVENT_SET) || (vkr__ == VK_EVENT_RESET) || (vkr__ == VK_INCOMPLETE ) ||\
        (vkr__ == VK_SUBOPTIMAL_KHR ) || (vkr__ == VK_THREAD_IDLE_KHR ) || (vkr__ == VK_THREAD_DONE_KHR) ||\
        (vkr__ == VK_OPERATION_DEFERRED_KHR) || (vkr__ == VK_OPERATION_NOT_DEFERRED_KHR) || (vkr__ == VK_THREAD_DONE_KHR), /*string_VkResult(vkr__)*/ "demn");}\
    while(0);
//(vkr__ == VK_PIPELINE_COMPILE_REQUIRED) || (vkr__ == VK_PIPELINE_BINARY_MISSING_KHR) || (vkr__ == VK_INCOMPATIBLE_SHADER_BINARY_EXT ) ||

namespace wirender {
    namespace RenderVulkanUtils {
        [[nodiscard]] physical_device_info choose_best_physical_device(VkPhysicalDevice* devices, uint32_t deviceCount);
        [[nodiscard]] queue_family_indices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);
        [[nodiscard]] VkRenderPass create_default_render_pass(VkDevice device, VkFormat imageFormat, const VkAllocationCallbacks* allocationCallbacks);
        VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
        [[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT create_default_debug_messenger_create_info();
        [[nodiscard]] uint32_t sizeof_vk_format(VkFormat); // in bytes
        [[nodiscard]] uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void analyze_spirv(const uint32_t* code, uint32_t codeSize, VkShaderStageFlagBits stageFlags, RenderVulkanUtils::public_spirv_variable_declaration publicDecls[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t& variableCount);


        static const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        static const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
        #if (defined __WIN32)
            static const char* instanceExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, "VK_EXT_debug_utils" };
        #else
        #   error "NOT SUPPORTED"
        #endif // defined __WIN32
    };
};
render_manager::render_manager(const window_info& windowInfo) : 
        windowInfo(windowInfo),
        vulkanInstance(0),
        debugMessenger(0),
        physicalDevice{},
        logicalDevice{},
        surface(0),
        defaultRenderPass(0),
        swapchain(0),
        swapchainImages{},
        commandPool(0),
        commandBuffers{},
        syncObject{},
        currentShader{},
        bindedBuffer{},
        imageIndex(0),
        validationEnable(true) {

    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    vulkanInstance = create_vulkan_instance();

    if (validationEnable)
        debugMessenger = create_debug_messenger();

    physicalDevice = get_physical_device();

    surface = create_surface();

    physicalDevice.queueIndeces = create_falimy_indices();

    logicalDevice = create_logical_device();

    swapchainSupportInfo = create_swapchain_info();

    defaultRenderPass = create_default_render_pass(logicalDevice, swapchainSupportInfo.imageFormat.format, allocationCallbacks);

    swapchain = create_swapchain();

    initialize_swapchain_images(swapchainImages);

    commandPool = create_command_pool();
    
    allocate_command_buffers(commandBuffers);

    intitialize_sync_object(syncObject);
}
render_manager::render_manager(render_manager&& other) : 
        vulkanInstance(other.vulkanInstance),
        debugMessenger(other.debugMessenger),
        physicalDevice(other.physicalDevice),
        logicalDevice(other.logicalDevice),
        surface(other.surface),
        swapchainSupportInfo(other.swapchainSupportInfo),
        defaultRenderPass(other.defaultRenderPass),
        swapchain(other.swapchain),
        swapchainImages(other.swapchainImages),
        commandPool(other.commandPool),
        syncObject(other.syncObject),
        imageIndex(other.imageIndex) {

    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i)
        commandBuffers[i] = other.commandBuffers[i];
    other.set_members_zero();
}

render_manager::~render_manager() {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    vkDeviceWaitIdle(logicalDevice);
    if (syncObject.semaphore != 0)
        vkDestroySemaphore(logicalDevice, syncObject.semaphore, allocationCallbacks);
    if (syncObject.fence != 0)
        vkDestroyFence(logicalDevice, syncObject.fence, allocationCallbacks);
    
    destroy_swapchain_images(swapchainImages);
    if (commandPool != 0)
        vkDestroyCommandPool(logicalDevice, commandPool, allocationCallbacks);
    if (swapchain != 0)
        vkDestroySwapchainKHR(logicalDevice, swapchain, allocationCallbacks);
    if (defaultRenderPass != 0)
        vkDestroyRenderPass(logicalDevice, defaultRenderPass, allocationCallbacks);
    if (surface != 0)
        vkDestroySurfaceKHR(vulkanInstance, surface, allocationCallbacks);
    if (logicalDevice != 0)
        vkDestroyDevice(logicalDevice, allocationCallbacks);
    if (debugMessenger != 0) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance,"vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(vulkanInstance, debugMessenger, allocationCallbacks);
    }
    if (vulkanInstance != 0)
        vkDestroyInstance(vulkanInstance, allocationCallbacks);

    set_members_zero();
}
render_manager& render_manager::clear_command_list() {
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i)
        vkResetCommandBuffer(commandBuffers[i], (VkFlags)0);
    return *this;
}
render_manager& render_manager::start_record() {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        const VkCommandBufferInheritanceInfo inheritanceInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = nullptr,
            .renderPass = currentShader.renderPass,
            .subpass = {},
            .framebuffer = swapchainImages.framebuffers[i],
            .occlusionQueryEnable = VK_FALSE,
            .queryFlags = (VkQueryControlFlags)0,
            .pipelineStatistics = {},
        };
        const VkCommandBufferBeginInfo beginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = (VkFlags)0,
            .pInheritanceInfo = &inheritanceInfo,
        };

        RENDER_VK_CHECK(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));
    }

    return *this;
}
render_manager& render_manager::record_update_viewport() {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        const VkViewport view {.x = 0.0f, .y = 0.0f, .width = (float)swapchainSupportInfo.extent.width, .height = (float)swapchainSupportInfo.extent.height, .minDepth = 0.0f, .maxDepth = 1.0f, };
        vkCmdSetViewport(commandBuffers[i], 0, 1, &view);
    }

    return *this;
}
render_manager& render_manager::record_update_scissor() {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        const VkRect2D scissor {{0, 0}, swapchainSupportInfo.extent};
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);
    }

    return *this;
}
render_manager& render_manager::record_start_render() {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        const VkClearValue clearVal{};//.color = VkClearColorValue{{0}},//.depthStencil = {//    .depth = 0.0f,//    .stencil = 0u,//},
        const VkRenderPassBeginInfo beginRenderPassInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = currentShader.renderPass,
            .framebuffer = swapchainImages.framebuffers[i],
            .renderArea = {{0, 0}, swapchainSupportInfo.extent},
            .clearValueCount = 1,
            .pClearValues = &clearVal,
        };
        vkCmdBeginRenderPass(commandBuffers[i], &beginRenderPassInfo, {});

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, currentShader.layout, 0, 1, &currentShader.descriptorSet, 0, nullptr );
    }

    return *this;
}
render_manager& render_manager::record_draw_verteces(uint32_t vertexCount, uint32_t offset, uint32_t instanceCount) {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        const VkDeviceSize offsets[] {
            offset,
        };
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, currentShader.pipeline);
        vkCmdBindVertexBuffers(commandBuffers[i],0,1, &bindedBuffer.buffer, offsets);

        vkCmdDraw(commandBuffers[i], vertexCount, instanceCount, 0, 0);
    }

    return *this;
}
render_manager& render_manager::record_draw_indexed(uint32_t vertexCount, uint32_t offset, uint32_t instanceCount) {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        const VkDeviceSize offsets[] {
            offset,
        };
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, currentShader.pipeline);
        vkCmdBindVertexBuffers(commandBuffers[i],0,1, &bindedBuffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], bindedBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDraw(commandBuffers[i], vertexCount, instanceCount, 0, 0);
    }

    return *this;
}
render_manager& render_manager::record_end_render() {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i)
        vkCmdEndRenderPass(commandBuffers[i]);

    return *this;
}
render_manager& render_manager::end_record() {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i)
        RENDER_VK_CHECK(vkEndCommandBuffer(commandBuffers[i]));

    return *this;
}
render_manager& render_manager::resize() {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    destroy_swapchain_images(swapchainImages);
    if (swapchain != 0)
        vkDestroySwapchainKHR(logicalDevice, swapchain, allocationCallbacks);

    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        swapchainImages.framebuffers[i] = 0;
        swapchainImages.views[i] = 0;
    }
    swapchain = 0;
    swapchainSupportInfo = create_swapchain_info();
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;
    swapchain = create_swapchain();
    initialize_swapchain_images(swapchainImages);

    return *this;
}
render_manager& render_manager::set_shader(const RenderVulkanUtils::active_shader_state& newCurrentShader) {
    currentShader = newCurrentShader;
    return *this;
}
render_manager& render_manager::bind_buffer(const RenderVulkanUtils::binded_buffer_state& newBindedBuffer) {
    bindedBuffer = newBindedBuffer;
    return *this;
}
render_manager& render_manager::wait_executing() {
    RENDER_VK_CHECK(vkQueueWaitIdle(logicalDevice.graphicsQueue));
    RENDER_VK_CHECK(vkQueueWaitIdle(logicalDevice.presentQueue));
    return *this;
}
render_manager& render_manager::execute() {
    if ((swapchainSupportInfo.extent.width == 0) || (swapchainSupportInfo.extent.height == 0))
        return *this;

    const VkSemaphore& semaphore = syncObject.semaphore;
    const VkFence& fence = syncObject.fence;
    vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX);
    vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, semaphore, 0, &imageIndex);

    vkResetFences(logicalDevice, 1, &fence);
    const VkPipelineStageFlags waitStages[] { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSubmitInfo submit {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &semaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = RENDER_ARRAY_SIZE(waitStages),
        .pCommandBuffers = &commandBuffers[imageIndex],
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };
    vkQueueSubmit(logicalDevice.graphicsQueue, 1, &submit, fence);
    const VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr, // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentInfoKHR.html
    };
    vkQueuePresentKHR(logicalDevice.presentQueue, &presentInfo);

    return *this;
}
void render_manager::set_members_zero() {
    syncObject.semaphore = 0;
    syncObject.fence = 0;
    commandPool = 0;
    for (uint32_t i = 0; i < swapchainImages.imageCount; ++i) {
        swapchainImages.framebuffers[i] = 0;
        swapchainImages.views[i] = 0;
    }
    defaultRenderPass = 0;
    swapchain = 0;
    surface = 0;
    logicalDevice.device = 0;
    debugMessenger = 0;
    vulkanInstance = 0;
}
shader::shader(render_manager* owner_, const shader_create_info& createInfo) : owner(owner_) {
    RENDER_ASSERT(owner != nullptr, "owner pointer does not valid");

    RenderVulkanUtils::public_spirv_variable_declaration publicDecls[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT];
    uint32_t publicDeclCount = 0;
    for (uint32_t stageI = 0; stageI < createInfo.stageCount; ++stageI) {
        RenderVulkanUtils::analyze_spirv(createInfo.stages[stageI].code, createInfo.stages[stageI].codeSize, createInfo.stages[stageI].stage, publicDecls, publicDeclCount);
    }
    descriptorPool = create_descriptor_pool(createInfo, publicDecls, publicDeclCount);
    uniformBuffers = create_uniform_buffers(createInfo, publicDecls, publicDeclCount);
    descriptorSetLayout = create_descriptor_set_layout(createInfo, publicDecls, publicDeclCount);
    descriptorSet = create_descriptor_set(createInfo, publicDecls, publicDeclCount);
    renderPass = create_render_pass();
    pipelineLayout = create_pipeline_layout();
    pipeline = create_pipeline(createInfo);
}
shader::~shader() {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    if (owner == nullptr)
        return;

    vkDeviceWaitIdle(owner->logicalDevice);

    if (pipeline != 0)
        vkDestroyPipeline(owner->logicalDevice, pipeline, allocationCallbacks);
    if (pipelineLayout != 0)
        vkDestroyPipelineLayout(owner->logicalDevice, pipelineLayout, allocationCallbacks);
    if (renderPass != 0)
        vkDestroyRenderPass(owner->logicalDevice, renderPass, allocationCallbacks);
    
    for (auto i : uniformBuffers.buffers)
        if (i.buffer)
            vkDestroyBuffer(owner->logicalDevice, i.buffer, allocationCallbacks);
    if (uniformBuffers.mappedMemory)
        vkUnmapMemory(owner->logicalDevice, uniformBuffers.memory);
    if (uniformBuffers.memory)
        vkFreeMemory(owner->logicalDevice, uniformBuffers.memory, allocationCallbacks);
        
    if (descriptorSetLayout)
        vkDestroyDescriptorSetLayout(owner->logicalDevice, descriptorSetLayout, allocationCallbacks);
    if (descriptorPool)
        vkDestroyDescriptorPool(owner->logicalDevice, descriptorPool, allocationCallbacks);
    
    set_members_zero();
}
[[nodiscard]] RenderVulkanUtils::active_shader_state shader::get_state() const {
    return RenderVulkanUtils::active_shader_state {
        .pipeline = pipeline,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .descriptorSet = descriptorSet,
    };
}
[[nodiscard]] void* shader::get_uniform_buffer_memory_on_binding(uint32_t binding) {
    RENDER_ASSERT(binding < RENDER_UNIFORM_BUFFER_MAX_COUNT, "Binding has to be less than RENDER_UNIFORM_BUFFER_MAX_COUNT");
    VkDeviceSize offset = 0;
    for (uint32_t i = 0; i < binding; ++i) 
        offset += uniformBuffers.buffers[i].size;
    return (void*)((char*)uniformBuffers.mappedMemory + offset); 
}
void shader::set_members_zero() {
    renderPass = 0;
    pipelineLayout = 0;
    pipeline = 0;

    for (auto& i : uniformBuffers.buffers)
        i.buffer = 0;
    uniformBuffers.mappedMemory = nullptr;
    uniformBuffers.memory = 0;

    descriptorSetLayout = 0;
    descriptorPool = 0;
}
shader_builder::shader_builder(render_manager* owner_) : owner(owner_), createInfo{} {
    RENDER_ASSERT(owner != nullptr, "Owner pointer does not valid");
}
shader_builder& shader_builder::add_stage(const shader_stage& newStage) {
    //RENDER_ASSERT(newStage.code != nullptr, "code pointer is nullptr");
    //RENDER_ASSERT(newStage.codeSize > 4, "code size too small");
    RENDER_ASSERT(createInfo.stageCount < RENDER_STAGE_MAX_COUNT, "Out of createInfo.stages range");
    createInfo.stages[createInfo.stageCount] = newStage;
    ++createInfo.stageCount;
    return *this;
}
shader_builder& shader_builder::add_vertex_input_attribute(const VkVertexInputAttributeDescription& newAttribute) {
    RENDER_ASSERT(createInfo.vertexInputAttributeCount < RENDER_INPUT_ATTRIBUTE_MAX_COUNT, "Out of createInfo.vertexInputAtributes range");
    createInfo.vertexInputAtributes[createInfo.vertexInputAttributeCount] = newAttribute;
    ++createInfo.vertexInputAttributeCount;
    return *this;
}
shader_builder& shader_builder::add_dynamic_state(VkDynamicState newState) {
    RENDER_ASSERT(createInfo.dynamicStateCount < RENDER_DYNAMIC_STATE_MAX_COUNT, "Out of createInfo.dynamicStateCount range");
    createInfo.dynamicStates[createInfo.dynamicStateCount] = newState;
    ++createInfo.dynamicStateCount;
    return *this;
}
shader_builder& shader_builder::pop_dynamic_state() {
    RENDER_ASSERT(createInfo.dynamicStateCount > 0, "Out of createInfo.dynamicStateCount range");
    --createInfo.dynamicStateCount;
    return *this;
}
shader_builder& shader_builder::set_primitive_topology(VkPrimitiveTopology newPrimitiveTopology) {
    createInfo.primitiveTopology = newPrimitiveTopology;
    return *this;
}
shader_builder& shader_builder::set_line_width(float newLineWidth) {
    createInfo.lineWidth = newLineWidth;
    return *this;
}
shader_builder& shader_builder::set_polygon_mode(VkPolygonMode newPolygonMode) {
    createInfo.polygonMode = newPolygonMode;
    return *this;
}
shader_builder& shader_builder::set_rasterization_sample_count(VkSampleCountFlagBits newSampleCount) {
    createInfo.rasterizationSampleCount = newSampleCount;
    return *this;
}
shader_builder& shader_builder::set_cull_mode(VkCullModeFlagBits newCullMode) {
    createInfo.cullMode = newCullMode;
    return *this;
}
[[nodiscard]] shader shader_builder::build() const {
    return shader(owner, createInfo);
}
buffer_host_mapped_memory::buffer_host_mapped_memory(render_manager* owner_, const buffer_create_info& createInfo) : owner(owner_), size(createInfo.size), buffer{}, usage(createInfo.usage) {
    RENDER_ASSERT(owner != nullptr, "Owner pointer does not valid");

    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    const VkBufferCreateInfo bufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &owner->physicalDevice.queueIndeces.graphicsFamily,
    };
    RENDER_VK_CHECK(vkCreateBuffer(owner->logicalDevice, &bufferCreateInfo, allocationCallbacks, &buffer.buffer));

    const VkMemoryAllocateInfo memoryAllocationInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = size,
        .memoryTypeIndex = RenderVulkanUtils::find_memory_type(owner->physicalDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    // Я не знаю, как будет использоваться буффер, так что придётся указать все возможные варианты,
    // Так-же, "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT" может не существовать
    // Но "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT" может существовать и нормально работать, когда его используют как вертексный буффер
    RENDER_VK_CHECK(vkAllocateMemory(owner->logicalDevice, &memoryAllocationInfo, allocationCallbacks, &buffer.memory));

    RENDER_VK_CHECK(vkBindBufferMemory(owner->logicalDevice, buffer.buffer, buffer.memory, 0));
}
buffer_host_mapped_memory::~buffer_host_mapped_memory() {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    if (owner == nullptr)
        return;

    vkDeviceWaitIdle(owner->logicalDevice);

    if (buffer.buffer != 0)
        vkDestroyBuffer(owner->logicalDevice, buffer.buffer, allocationCallbacks);
    if (buffer.memory != 0)
        vkFreeMemory(owner->logicalDevice, buffer.memory, allocationCallbacks);

    set_members_zero();
}
void buffer_host_mapped_memory::map_memory(void** datap) {
    vkMapMemory(owner->logicalDevice, buffer.memory, 0, size, 0, datap);
}
void buffer_host_mapped_memory::unmap_memory() {
    vkUnmapMemory(owner->logicalDevice, buffer.memory);
}
[[nodiscard]] RenderVulkanUtils::binded_buffer_state buffer_host_mapped_memory::get_state() const {
    return RenderVulkanUtils::binded_buffer_state {
        .buffer = buffer.buffer,
    };
}
void buffer_host_mapped_memory::set_members_zero() {
    buffer = {};
}

/*GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG*/
/*GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG*/
/*GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG*/
[[nodiscard]] VkInstance render_manager::create_vulkan_instance() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    const VkDebugUtilsMessengerCreateInfoEXT dbgMessengerCreateInfo = RenderVulkanUtils::create_default_debug_messenger_create_info();

    const VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "HAME",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "GN",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };
    const VkInstanceCreateInfo instanceInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = validationEnable ? &dbgMessengerCreateInfo : 0,
        .flags = (VkFlags)0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (uint32_t)RENDER_ARRAY_SIZE(RenderVulkanUtils::validationLayers) - (validationEnable ? 0u : 1u),
        .ppEnabledLayerNames = RenderVulkanUtils::validationLayers,
        .enabledExtensionCount = (uint32_t)RENDER_ARRAY_SIZE(RenderVulkanUtils::instanceExtensions) - (validationEnable ? 0u : 1u),
        .ppEnabledExtensionNames = RenderVulkanUtils::instanceExtensions,
    };

    VkInstance newInstance;
    RENDER_VK_CHECK(vkCreateInstance(&instanceInfo, allocationCallbacks, &newInstance));

    return newInstance;
}
[[nodiscard]] VkDebugUtilsMessengerEXT render_manager::create_debug_messenger() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    VkDebugUtilsMessengerEXT newDebugMessenger = 0;
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance,"vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        const VkDebugUtilsMessengerCreateInfoEXT dbgMessengerCreateInfo = RenderVulkanUtils::create_default_debug_messenger_create_info();
        func(vulkanInstance, &dbgMessengerCreateInfo, allocationCallbacks, &newDebugMessenger);
    }
    return newDebugMessenger;
}
[[nodiscard]] RenderVulkanUtils::physical_device_info render_manager::get_physical_device() const {
    VkPhysicalDevice devices[RENDER_DEFAULT_MAX_COUNT] = {};
    uint32_t deviceCount = 0;
    RENDER_VK_CHECK(vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, 0));
    RENDER_ASSERT(deviceCount > 0, "Passed zero devices");
    if (deviceCount > RENDER_DEFAULT_MAX_COUNT)
        deviceCount = RENDER_DEFAULT_MAX_COUNT;
    RENDER_VK_CHECK(vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices));

    return RenderVulkanUtils::choose_best_physical_device(devices, deviceCount);
}
#if (defined __WIN32)
[[nodiscard]] VkSurfaceKHR render_manager::create_surface() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    RENDER_ASSERT(windowInfo.hwnd != 0, "Invalid hwnd");
    RENDER_ASSERT(windowInfo.hInstance != 0, "Invalid hInstance");

    const VkWin32SurfaceCreateInfoKHR surfaceInfo {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .hinstance = windowInfo.hInstance,
        .hwnd = windowInfo.hwnd,
    };
    VkSurfaceKHR newSurface;
    RENDER_VK_CHECK(vkCreateWin32SurfaceKHR(vulkanInstance, &surfaceInfo, allocationCallbacks, &newSurface));

    return newSurface;
}
#else
[[nodiscard]] VkSurfaceKHR render_manager::create_surface() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    RENDER_ASSERT(windowInfo.dpy != 0, "Invalid display");
    RENDER_ASSERT(windowInfo.window != 0, "Invalid window");

    const VkXlibSurfaceCreateInfoKHR surfaceInfo {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .dpy = windowInfo.dpy,
        .window = windowInfo.window,
    };
    VkSurfaceKHR newSurface;
    RENDER_VK_CHECK(vkCreateXlibSurfaceKHR(vulkanInstance, &surfaceInfo, allocationCallbacks, &newSurface));

    return newSurface;
}
#endif // defined __WIN32
[[nodiscard]] RenderVulkanUtils::queue_family_indices render_manager::create_falimy_indices() const {
    RenderVulkanUtils::queue_family_indices queueIndeces = RenderVulkanUtils::find_queue_families(physicalDevice, surface);
    RENDER_ASSERT(queueIndeces.is_complete(), "Physical device families indices aren't complete");
    return queueIndeces;
}
[[nodiscard]] RenderVulkanUtils::logical_device_info render_manager::create_logical_device() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    VkDeviceQueueCreateInfo queueCreateInfos[2];

    float queuePriority = 1.0f;
    for (uint32_t i = 0; i < physicalDevice.queueIndeces.falimiesCount; ++i) {
        VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[i];
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = physicalDevice.queueIndeces.indeces[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
    }
    const VkDeviceCreateInfo deviceInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &physicalDevice.features,
        .flags = (VkFlags)0,
        .queueCreateInfoCount = physicalDevice.queueIndeces.falimiesCount,
        .pQueueCreateInfos = queueCreateInfos,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = RENDER_ARRAY_SIZE(RenderVulkanUtils::deviceExtensions),
        .ppEnabledExtensionNames =  RenderVulkanUtils::deviceExtensions,
        .pEnabledFeatures = nullptr,
    };

    RenderVulkanUtils::logical_device_info result{};
    RENDER_VK_CHECK(vkCreateDevice(physicalDevice, &deviceInfo, allocationCallbacks, &result.device));
    vkGetDeviceQueue(result.device, physicalDevice.queueIndeces.graphicsFamily, 0, &result.graphicsQueue);
    vkGetDeviceQueue(result.device, physicalDevice.queueIndeces.presentFamily, 0, &result.presentQueue);

    return result;
}
[[nodiscard]] RenderVulkanUtils::swapchain_support_info render_manager::create_swapchain_info() const {
    RenderVulkanUtils::swapchain_support_info newSwapchainSupportInfo;
    RENDER_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &newSwapchainSupportInfo.capabilities));

    uint32_t formatCount = 0;
    VkSurfaceFormatKHR formats[RENDER_DEFAULT_MAX_COUNT] = {};
    RENDER_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
    RENDER_ASSERT(formatCount > 0, "No supported formats found");
    if (formatCount > RENDER_DEFAULT_MAX_COUNT)
        formatCount = RENDER_DEFAULT_MAX_COUNT;
    RENDER_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats));

    uint32_t presentModeCount = 0;
    VkPresentModeKHR presentModes[RENDER_DEFAULT_MAX_COUNT] = {};
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    RENDER_ASSERT(presentModeCount > 0, "No supported present modes");
    if (presentModeCount > RENDER_DEFAULT_MAX_COUNT)
        presentModeCount = RENDER_DEFAULT_MAX_COUNT;
    RENDER_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes));

    newSwapchainSupportInfo.imageFormat = formats[0];
    for (uint32_t i = 1; i < formatCount; ++i) {
        if ((formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) && (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)) {
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

    if (newSwapchainSupportInfo.imageCount > RENDER_SWAPCHAIN_IMAGE_MAX_COUNT)
        newSwapchainSupportInfo.imageCount = RENDER_SWAPCHAIN_IMAGE_MAX_COUNT;

    return newSwapchainSupportInfo;
}
[[nodiscard]] VkSwapchainKHR render_manager::create_swapchain() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    const VkSwapchainCreateInfoKHR swapchainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .surface = surface,
        .minImageCount = swapchainSupportInfo.imageCount,
        .imageFormat = swapchainSupportInfo.imageFormat.format,
        .imageColorSpace = swapchainSupportInfo.imageFormat.colorSpace,
        .imageExtent = swapchainSupportInfo.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = (physicalDevice.queueIndeces.falimiesCount == 2u) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = (physicalDevice.queueIndeces.falimiesCount == 2u) ? 2u : 0u,
        .pQueueFamilyIndices = physicalDevice.queueIndeces.indeces,
        .preTransform = swapchainSupportInfo.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = swapchainSupportInfo.presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = 0,
    };
    VkSwapchainKHR newSwapchain;
    RENDER_VK_CHECK(vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, allocationCallbacks, &newSwapchain));

    return newSwapchain;
}
void render_manager::initialize_swapchain_images(RenderVulkanUtils::swapchain_images& swapchainImagesToInitialize) const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    RENDER_VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImagesToInitialize.imageCount, nullptr));
    RENDER_ASSERT(swapchainImagesToInitialize.imageCount > 0, "No images from swapchain");
    if (swapchainImagesToInitialize.imageCount > RENDER_SWAPCHAIN_IMAGE_MAX_COUNT)
        swapchainImagesToInitialize.imageCount = RENDER_SWAPCHAIN_IMAGE_MAX_COUNT;
    RENDER_VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImagesToInitialize.imageCount, swapchainImagesToInitialize.images));

    for (uint32_t i = 0; i < swapchainImagesToInitialize.imageCount; ++i) {
        const VkImageViewCreateInfo imageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = (VkFlags)0,
            .image = swapchainImagesToInitialize.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchainSupportInfo.imageFormat.format,
            .components = VkComponentMapping {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        RENDER_VK_CHECK(vkCreateImageView(logicalDevice, &imageViewCreateInfo, allocationCallbacks, &swapchainImagesToInitialize.views[i]));

        const VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = (VkFlags)0,
            .renderPass = defaultRenderPass,
            .attachmentCount = 1,
            .pAttachments = &swapchainImagesToInitialize.views[i],
            .width = swapchainSupportInfo.extent.width,
            .height = swapchainSupportInfo.extent.height,
            .layers = 1,
        };
        RENDER_VK_CHECK(vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, allocationCallbacks, &swapchainImagesToInitialize.framebuffers[i]));
    }
}
[[nodiscard]] VkCommandPool render_manager::create_command_pool() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    const VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = physicalDevice.queueIndeces.graphicsFamily
    };
    VkCommandPool newCommandPool;
    RENDER_VK_CHECK(vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, allocationCallbacks, &newCommandPool));

    return newCommandPool;
}
void render_manager::allocate_command_buffers(VkCommandBuffer commandBuffersToAllocate[RENDER_SWAPCHAIN_IMAGE_MAX_COUNT]) const {
    const VkCommandBufferAllocateInfo commandBuuferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = swapchainImages.imageCount,
    };

    RENDER_VK_CHECK(vkAllocateCommandBuffers(logicalDevice, &commandBuuferAllocateInfo, commandBuffersToAllocate));
}
void render_manager::destroy_swapchain_images(RenderVulkanUtils::swapchain_images& swapchainImagesToDestroy) const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    for (uint32_t i = 0; i < swapchainImagesToDestroy.imageCount; ++i) {
        if (swapchainImagesToDestroy.framebuffers[i] != 0)
            vkDestroyFramebuffer(logicalDevice, swapchainImagesToDestroy.framebuffers[i], allocationCallbacks);
        if (swapchainImagesToDestroy.views[i] != 0)
            vkDestroyImageView(logicalDevice, swapchainImagesToDestroy.views[i], allocationCallbacks);
    }
}
void render_manager::intitialize_sync_object(RenderVulkanUtils::sync_object& syncObjectsToInitialize) const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    const VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    RENDER_VK_CHECK(vkCreateFence(logicalDevice, &fenceInfo, allocationCallbacks, &syncObjectsToInitialize.fence));

    const VkSemaphoreCreateInfo semapforeInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
    };
    RENDER_VK_CHECK(vkCreateSemaphore(logicalDevice, &semapforeInfo, allocationCallbacks, &syncObjectsToInitialize.semaphore));
}
uint16_t getOpcode(uint32_t word) {
    return word & 0xFFFF;
}
uint16_t getWordCount(uint32_t word) {
    return (word >> 16) & 0xFFFF;
}
[[nodiscard]] VkDescriptorPool shader::create_descriptor_pool(const shader_create_info& createInfo, const RenderVulkanUtils::public_spirv_variable_declaration publicDecls[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t publicDeclCount) const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    VkDescriptorPoolSize poolSizes[RENDER_DESCRIPTOR_MAX_COUNT]{};
    uint32_t poolSizeCount = 0;
    for (uint32_t i = 0; i < publicDeclCount; ++i) {
        const auto& pubDecl = publicDecls[i];
        if (pubDecl.type == 2/*Uniform*/) {
            poolSizes[poolSizeCount].descriptorCount = 1;
            poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ++poolSizeCount;
        } else if (pubDecl.type == 11/*Image*/) {
            poolSizes[poolSizeCount].descriptorCount = 1;
            poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            ++poolSizeCount;
        } else if (pubDecl.type == 12/*StorageBuffer*/) {
            poolSizes[poolSizeCount].descriptorCount = 1;
            poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            ++poolSizeCount;
        }
        RENDER_ASSERT(poolSizeCount < RENDER_DESCRIPTOR_MAX_COUNT, "poolSizeCount has to be less than RENDER_DESCRIPTOR_MAX_COUNT");
    }
    const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .maxSets = 1,
        .poolSizeCount = poolSizeCount,
        .pPoolSizes = poolSizes,
    };
    VkDescriptorPool result;
    RENDER_VK_CHECK(vkCreateDescriptorPool(owner->logicalDevice, &descriptorPoolCreateInfo, allocationCallbacks, &result));
    return result;
}
[[nodiscard]] RenderVulkanUtils::uniform_buffers_info shader::create_uniform_buffers(const shader_create_info& createInfo, const RenderVulkanUtils::public_spirv_variable_declaration publicDecls[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t publicDeclCount) const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    RenderVulkanUtils::uniform_buffers_info result;
    for (uint32_t i = 0; i < publicDeclCount; ++i) {
        const auto& pubDecl = publicDecls[i];
        if (pubDecl.type != 2/*Uniform*/)
            continue;
        RENDER_ASSERT(pubDecl.binding < RENDER_UNIFORM_BUFFER_MAX_COUNT, "Uniform buffer binding has to be less than RENDER_UNIFORM_BUFFER_MAX_COUNT");
        RENDER_ASSERT(result.buffers[pubDecl.binding].size == 0, "This binding is defined twice or more times");
        result.buffers[pubDecl.binding].size = pubDecl.size;  
    }
    
    VkDeviceSize totalBufferSize = 0;
    for (uint32_t i = 0; i < RENDER_UNIFORM_BUFFER_MAX_COUNT; ++i)
        totalBufferSize += result.buffers[i].size;
    totalBufferSize = totalBufferSize < 128 ? 128 : totalBufferSize; // 128 - common minimal allocated memory size

    const VkMemoryAllocateInfo allocationInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = totalBufferSize,
        .memoryTypeIndex = RenderVulkanUtils::find_memory_type(owner->physicalDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    RENDER_VK_CHECK(vkAllocateMemory(owner->logicalDevice, &allocationInfo, allocationCallbacks, &result.memory));

    VkDeviceSize offset = 0;
    for (uint32_t i = 0; i < RENDER_UNIFORM_BUFFER_MAX_COUNT; ++i)  {
        if (result.buffers[i].size == 0)
            continue;
        
        const VkBufferCreateInfo bufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = (VkFlags)0, //VK_BUFFER_CREATE_SPARSE_ALIASED_BIT | VK_BUFFER_CREATE_SPARSE_BINDING_BIT,
            .size = result.buffers[i].size,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &owner->physicalDevice.queueIndeces.graphicsFamily,
        };

        RENDER_VK_CHECK(vkCreateBuffer(owner->logicalDevice, &bufferCreateInfo, allocationCallbacks, &result.buffers[i].buffer));
        RENDER_VK_CHECK(vkBindBufferMemory(owner->logicalDevice, result.buffers[i].buffer, result.memory, offset));
        offset += result.buffers[i].size;
    }
    RENDER_VK_CHECK(vkMapMemory(owner->logicalDevice, result.memory, 0, totalBufferSize, (VkFlags)0, &result.mappedMemory));
    return result;
}
[[nodiscard]] VkDescriptorSetLayout shader::create_descriptor_set_layout(const shader_create_info&, const RenderVulkanUtils::public_spirv_variable_declaration publicDecls[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t publicDeclCount) {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    VkDescriptorSetLayoutBinding bindings[RENDER_DESCRIPTOR_MAX_COUNT];
    uint32_t bindingCount = 0;
    for (uint32_t i = 0; i < publicDeclCount; ++i) {
        VkDescriptorSetLayoutBinding& binding = bindings[bindingCount];
        binding.binding = publicDecls[i].binding;
        binding.stageFlags = publicDecls[i].stageFlags;
        binding.descriptorCount = 1;
        binding.pImmutableSamplers = nullptr;

        if (publicDecls[i].type == 2/*Uniform*/) {
            binding.descriptorType =  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        } else if (publicDecls[i].type == 11/*Image*/) {
            binding.descriptorType =  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        } else if (publicDecls[i].type == 12/*StorageBuffer*/) {
            binding.descriptorType =  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        } else {
            continue;
        }
        ++bindingCount;
    }

    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .bindingCount = bindingCount,
        .pBindings = bindings,
    };
    VkDescriptorSetLayout result;
    RENDER_VK_CHECK(vkCreateDescriptorSetLayout(owner->logicalDevice, &descriptorSetLayoutCreateInfo, allocationCallbacks, &result));
    return result;
}
[[nodiscard]] VkDescriptorSet shader::create_descriptor_set(const shader_create_info&, const RenderVulkanUtils::public_spirv_variable_declaration publicDecls[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t publicDeclCount) {
    const VkDescriptorSetAllocateInfo allocationInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    VkDescriptorSet result;
    RENDER_VK_CHECK(vkAllocateDescriptorSets(owner->logicalDevice, &allocationInfo, &result));

    for (uint32_t i = 0; i < publicDeclCount; ++i) {
        const VkDescriptorBufferInfo bufferInfo {
            .buffer = uniformBuffers.buffers[publicDecls[i].binding].buffer,
            .offset = 0,
            .range = uniformBuffers.buffers[publicDecls[i].binding].size,
        };
        const VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = result,
            .dstBinding = publicDecls[i].binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType =   (publicDecls[i].type == 2/*Uniform*/) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
                                (publicDecls[i].type == 11/*Image*/) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE :
                                (publicDecls[i].type == 12/*StorageBuffer*/) ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
                                VK_DESCRIPTOR_TYPE_MAX_ENUM,
            .pImageInfo = (publicDecls[i].type == 11/*Image*/) ? nullptr : nullptr, // TODO
            .pBufferInfo = ((publicDecls[i].type == 2/*Uniform*/) || (publicDecls[i].type == 12/*StorageBuffer*/)) ? &bufferInfo : nullptr,
            .pTexelBufferView = nullptr,
        };

        vkUpdateDescriptorSets(owner->logicalDevice, 1, &write, 0, nullptr);
    }
    return result;
}
[[nodiscard]] VkRenderPass shader::create_render_pass() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    const VkAttachmentDescription colorAttachment {
        .flags = (VkFlags)0,
        .format = owner->swapchainSupportInfo.imageFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    const VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass {
        .flags = (VkFlags)0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    const VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = (VkFlags)0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = (VkFlags)0,
    };
    const VkAttachmentDescription attachments[] = { colorAttachment };
    const VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .attachmentCount = static_cast<uint32_t>(RENDER_ARRAY_SIZE(attachments)),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VkRenderPass newRenderPass;
    RENDER_VK_CHECK(vkCreateRenderPass(owner->logicalDevice, &renderPassCreateInfo, allocationCallbacks, &newRenderPass));
    return newRenderPass;
}
[[nodiscard]] VkPipelineLayout shader::create_pipeline_layout() const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;

    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .setLayoutCount = descriptorSetLayout ? 1u : 0u,
        .pSetLayouts = descriptorSetLayout ? &descriptorSetLayout : nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    VkPipelineLayout newPipelineLayout;
    RENDER_VK_CHECK(vkCreatePipelineLayout(owner->logicalDevice, &pipelineLayoutCreateInfo, allocationCallbacks, &newPipelineLayout));
    return newPipelineLayout;
}
[[nodiscard]] VkPipeline shader::create_pipeline(const shader_create_info& createInfo) const {
    const VkAllocationCallbacks* allocationCallbacks = nullptr;
    RENDER_ASSERT(createInfo.stageCount < RENDER_STAGE_MAX_COUNT, "Too many stages");
    RENDER_ASSERT(createInfo.stageCount > 0, "No shader stages for shader program");

    RENDER_ASSERT(createInfo.dynamicStateCount < RENDER_DYNAMIC_STATE_MAX_COUNT, "Too many dynamic states");

    RENDER_ASSERT(createInfo.vertexInputAttributeCount < RENDER_INPUT_ATTRIBUTE_MAX_COUNT, "Too many vertex input attributes");

    VkPipelineShaderStageCreateInfo shaderStages[RENDER_DEFAULT_MAX_COUNT];

    for (uint32_t i = 0; i < createInfo.stageCount; ++i) {
        const VkShaderModuleCreateInfo shaderModuleCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = (VkFlags)0,
            .codeSize = createInfo.stages[i].codeSize,
            .pCode = createInfo.stages[i].code,
        };

        RENDER_VK_CHECK(vkCreateShaderModule(owner->logicalDevice, &shaderModuleCreateInfo, allocationCallbacks, &shaderStages[i].module));
        shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i].pNext = nullptr;
        shaderStages[i].flags = (VkFlags)0;
        shaderStages[i].stage = createInfo.stages[i].stage;
        shaderStages[i].pName = "main";
        shaderStages[i].pSpecializationInfo = {};
    }
    uint32_t inputSize = 0;
    for (uint32_t i = 0; i < createInfo.vertexInputAttributeCount; ++i)
        inputSize += RenderVulkanUtils::sizeof_vk_format(createInfo.vertexInputAtributes[i].format);

    const VkVertexInputBindingDescription inputBinding {
        .binding = 0,
        .stride = inputSize,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &inputBinding,
        .vertexAttributeDescriptionCount = createInfo.vertexInputAttributeCount,
        .pVertexAttributeDescriptions = createInfo.vertexInputAtributes,
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .topology = createInfo.primitiveTopology,
        .primitiveRestartEnable = VK_FALSE,
    };
    const VkRect2D scissors {
        {0, 0},
        owner->swapchainSupportInfo.extent,
    };
    const VkViewport view {
        .x = 0.0f, .y = 0.0f,
        .width = (float)owner->swapchainSupportInfo.extent.width, .height = (float)owner->swapchainSupportInfo.extent.height,
        .minDepth = 0.0f, .maxDepth = 1.0f,
    };
    const VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .viewportCount = 1,
        .pViewports = &view,
        .scissorCount = 1,
        .pScissors = &scissors,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = createInfo.polygonMode,
        .cullMode = createInfo.cullMode,//VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0,
        .depthBiasClamp = 0,
        .depthBiasSlopeFactor = 0,
        .lineWidth = createInfo.lineWidth,
    };

    const VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .rasterizationSamples = createInfo.rasterizationSampleCount,
        .sampleShadingEnable = createInfo.rasterizationSampleCount != VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 0,
        .pSampleMask = 0,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back  = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {},
    };

    const VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .dynamicStateCount = createInfo.dynamicStateCount,
        .pDynamicStates = createInfo.dynamicStates,
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .stageCount = createInfo.stageCount,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = false ? &depthStencil : nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    VkPipeline newPipeline;
    RENDER_VK_CHECK(vkCreateGraphicsPipelines(owner->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, allocationCallbacks, &newPipeline));
    for (uint32_t i = 0; i < createInfo.stageCount; ++i)
        vkDestroyShaderModule(owner->logicalDevice, shaderStages[i].module, allocationCallbacks);

    return newPipeline;
}
[[nodiscard]] RenderVulkanUtils::physical_device_info RenderVulkanUtils::choose_best_physical_device(VkPhysicalDevice* devices, uint32_t deviceCount) {
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

        VkPhysicalDeviceFeatures2 features2 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = nullptr,
            .features = {},
        };
        const VkPhysicalDeviceFeatures& features = features2.features;

        VkPhysicalDeviceProperties2 properties2 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = nullptr,
            .properties = {},
        };
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
    RENDER_ASSERT(bestDeviceScore > 0, "Physical device not found");
    return bestDevice;
}
[[nodiscard]] RenderVulkanUtils::queue_family_indices RenderVulkanUtils::find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    RenderVulkanUtils::queue_family_indices indices;
    VkQueueFamilyProperties queueFamilies[RENDER_DEFAULT_MAX_COUNT] = {};
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    if (queueFamilyCount > RENDER_DEFAULT_MAX_COUNT)
        queueFamilyCount = RENDER_DEFAULT_MAX_COUNT;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        VkQueueFamilyProperties queueFamily = queueFamilies[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 presentSupport = false;
        RENDER_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport));
        if (presentSupport)
            indices.presentFamily = i;

        if (indices.is_complete()) 
            break;
        ++i;
    }

    indices.falimiesCount = (indices.graphicsFamily == indices.presentFamily) ? 1 : 2;
    return indices;
}
[[nodiscard]] VkRenderPass RenderVulkanUtils::create_default_render_pass(VkDevice device, VkFormat imageFormat, const VkAllocationCallbacks* allocationCallbacks) {
    const VkAttachmentDescription colorAttachment {
        .flags = (VkFlags)0,
        .format = imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    const VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass {
        .flags = (VkFlags)0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    const VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = (VkDependencyFlags)0,
    };
    VkAttachmentDescription attachments[] = { colorAttachment };
    const VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFlags)0,
        .attachmentCount = static_cast<uint32_t>(RENDER_ARRAY_SIZE(attachments)),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VkRenderPass newRenderPass;
    RENDER_VK_CHECK(vkCreateRenderPass(device, &renderPassCreateInfo, allocationCallbacks, &newRenderPass));
    return newRenderPass;
}
VKAPI_ATTR VkBool32 VKAPI_CALL RenderVulkanUtils::debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    (void)messageSeverity;
    (void)messageType;
    (void)pUserData;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cout << "[ERROR] " << pCallbackData->pMessage << '\n';
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cout << "[WARNING] " << pCallbackData->pMessage << '\n';
    } else {
        std::cout << "[MSG] " << pCallbackData->pMessage << '\n';
    }
    return VK_FALSE;
}
[[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT RenderVulkanUtils::create_default_debug_messenger_create_info() {
    return VkDebugUtilsMessengerCreateInfoEXT {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = RenderVulkanUtils::debug_messenger_callback,
        .pUserData = nullptr,
    };
}

[[nodiscard]] uint32_t RenderVulkanUtils::sizeof_vk_format(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
            return 1u;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2u;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3u;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return 4u;
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6u;
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8u;
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12u;
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16u;
        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24u;
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32u;
        default:
            return ~0u;
    }
    return ~0u;
}
[[nodiscard]] uint32_t RenderVulkanUtils::find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    RENDER_ASSERT(false, "Memory index type not found");
}
void RenderVulkanUtils::analyze_spirv(const uint32_t* code, uint32_t codeSize, VkShaderStageFlagBits stageFlags, RenderVulkanUtils::public_spirv_variable_declaration publicDecls[RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT], uint32_t& variableCount)  {
    struct basic_declaration {
        uint32_t binding = 0u;
        uint32_t descriptorSet = 0u;
        uint32_t pointerTarget = 0u;
        uint32_t size = 0u;
        uint32_t storageClass = ~0u;
        uint16_t opType = 0u;
        bool isPublic = false;
    };

    std::unordered_map<uint32_t, basic_declaration> declarations;
    uint32_t currentOffset = 5;

    while (currentOffset < codeSize) {
        const uint32_t word = code[currentOffset];
        const uint16_t opcode = getOpcode(word);
        const uint16_t length = getWordCount(word);
        //std::cout << "L: " << length << "\n";
        //std::cout << "OP: " << opcode << "\n";
        if (length == 0 || opcode == 0)
            break;

        if (opcode == 19/*OpTypeVoid*/) {
            const uint32_t structId = code[currentOffset + 1];
            (void)declarations[structId];
            declarations[structId].opType = opcode;

        } else if (opcode == 20/*OpTypeBool*/) {
            const uint32_t structId = code[currentOffset + 1];
            declarations[structId].size = 16;
            declarations[structId].opType = opcode;

        } else if (opcode == 21/*OpTypeInt*/) {
            const uint32_t structId = code[currentOffset + 1];
            const uint32_t size = code[currentOffset + 2]; // in bits
            // const uint32_t signed = code[currentOffset + 3];
            declarations[structId].size = size / 8;
            declarations[structId].opType = opcode;

        } else if (opcode == 22/*OpTypeFloat*/) {
            const uint32_t structId = code[currentOffset + 1];
            const uint32_t size = code[currentOffset + 2]; // in bits
            declarations[structId].size = size / 8;
            declarations[structId].opType = opcode;

        } else if (opcode == 23/*OpTypeVector*/) {
            const uint32_t structId = code[currentOffset + 1];
            const uint32_t size = declarations.at(code[currentOffset + 2]).size * code[currentOffset + 3]; // in bytes
            declarations[structId].size = size;
            declarations[structId].opType = opcode;

        } else if (opcode == 24/*OpTypeMatrix*/) {
            const uint32_t structId = code[currentOffset + 1];
            const uint32_t size = declarations.at(code[currentOffset + 2]).size * code[currentOffset + 3]; // in bytes, must be vector
            declarations[structId].size = size;
            declarations[structId].opType = opcode;

        } else if (opcode == 28/*OpTypeArray*/) {
            const uint32_t structId = code[currentOffset + 1];
            const uint32_t size = declarations.at(code[currentOffset + 2]).size * code[currentOffset + 3]; // in bytes
            declarations[structId].size = size;
            declarations[structId].opType = opcode;

        } else if (opcode == 30/*OpTypeStruct*/) {
            const uint32_t structId = code[currentOffset + 1];
            auto& structDec = declarations[structId];// = { .binding = 0, .descriptorSet = 0, .size = 0, .isUniform = false };
            structDec.opType = opcode;
            for (uint32_t i = 2; i < length; ++i)
                structDec.size += declarations.at(code[currentOffset + i]).size;
                
        } else if (opcode == 32/*OpTypePointer*/) {
            const uint32_t structId = code[currentOffset + 1];
            const uint32_t strageClass = code[currentOffset + 2];
            const uint32_t target = code[currentOffset + 3];
            declarations[structId].size = 8;
            declarations[structId].opType = opcode;
            declarations[structId].pointerTarget = target;

        } else if (opcode == 59/*OpVariable*/) {
            const uint32_t target = code[currentOffset + 1];
            const uint32_t structId = code[currentOffset + 2];
            const uint32_t storageClass = code[currentOffset + 3];
            
            declarations[structId].opType = opcode;
            declarations[structId].pointerTarget = target;
            declarations[structId].storageClass = storageClass;
            if (storageClass == 2/*Uniform*/ || storageClass == 11/*Image*/ || storageClass == 12/*StorageBuffer*/) {
                declarations[structId].isPublic = true;
            }

            if (declarations[target].pointerTarget != 0) {
                declarations[structId].size = declarations[declarations[target].pointerTarget].size;
            } else  {
                declarations[structId].size = declarations[target].size;
            }

        } else if (opcode == 71/*OpDecorate*/) {
            const uint32_t target = code[currentOffset + 1];
            const uint32_t decoration = code[currentOffset + 2];
            const uint32_t operand = code[currentOffset + 3];

            if (decoration == 34/*DescriptorSet*/) {
                auto& decl = declarations[target];
                decl.descriptorSet = operand;
            } else if (decoration == 33/*Binding*/) {
                auto& decl = declarations[target];
                decl.binding = operand;
            }
        }
 
        currentOffset += length;
    }

    for (const auto& i : declarations) {
        if (i.second.isPublic) {
            RENDER_ASSERT(variableCount < RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT, "variableCount has to be less than RENDER_SPIRV_PUBLIC_VARIABLE_MAX_COUNT");
            publicDecls[variableCount] = RenderVulkanUtils::public_spirv_variable_declaration{.binding = i.second.binding, .descriptorSet = i.second.descriptorSet, .size = i.second.size, .type = i.second.storageClass, stageFlags};
            ++variableCount;
        }
    }
}