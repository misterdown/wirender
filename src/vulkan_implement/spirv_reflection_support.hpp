#ifndef WIENDER_SPV_RENFLECTION_SUPPORT_HPP_
#define WIENDER_SPV_RENFLECTION_SUPPORT_HPP_ 1

#include <spirv_reflect.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "../wiender_implement_core.hpp"

namespace wiender {
    void spv_reflect_check(SpvReflectResult result, const wcs::tiny_string_view<char>& strv) {
        wiender_assert(result == SPV_REFLECT_RESULT_SUCCESS, strv);
    }
    struct descriptor_set_layout_data {
        uint32_t setNumber;
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<size_t> bufferSizes;
        size_t size;
    };

    void get_descriptor_sets(std::vector<descriptor_set_layout_data>& out, size_t codeSize, const uint32_t* code) { // in bytes
        struct raii_SpvReflectShaderModule { // 'strong' exception g-tie
            private:
            SpvReflectShaderModule value;

            public:
            ~raii_SpvReflectShaderModule() {
                spvReflectDestroyShaderModule(&value);
            }

            public:
            SpvReflectShaderModule* operator->()  {
                return &value;
            }
            const SpvReflectShaderModule* operator->() const {
                return &value;
            }

            public:
            SpvReflectShaderModule& get() noexcept {
                return value;
            }
            const SpvReflectShaderModule& get() const noexcept {
                return value;
            }

        };
        
        raii_SpvReflectShaderModule module{};
        spv_reflect_check(spvReflectCreateShaderModule(codeSize, code, &module.get()), "wiender::get_descriptor_sets failed to create spv reflect shader module");

        uint32_t count;
        spv_reflect_check(spvReflectEnumerateDescriptorSets(&module.get(), &count, NULL), "wiender::get_descriptor_sets failed to enumerate descriptor sets 1");

        std::vector<SpvReflectDescriptorSet*> sets(count);
        spv_reflect_check(spvReflectEnumerateDescriptorSets(&module.get(), &count, sets.data()), "wiender::get_descriptor_sets failed to enumerate descriptor sets 2");

        for (size_t iS = 0; iS < sets.size(); ++iS) {
            const SpvReflectDescriptorSet& reflSet = *(sets[iS]);
            descriptor_set_layout_data* layoutp = nullptr;

            for (auto& set : out) {
                if (set.setNumber == reflSet.set) {
                    layoutp = &set;
                    break;
                }
            }
            if (layoutp == nullptr) {
                out.emplace_back(descriptor_set_layout_data{});
                layoutp = &out.back();
            }
            descriptor_set_layout_data& layout = *layoutp;

            for (uint32_t iB = 0; iB < reflSet.binding_count; ++iB) {
                const SpvReflectDescriptorBinding& reflBinding = *(reflSet.bindings[iB]);

                layout.bufferSizes.emplace_back(0);
                size_t& layoutBindingBufferSize = layout.bufferSizes.back();

                layout.bindings.emplace_back(VkDescriptorSetLayoutBinding{});
                VkDescriptorSetLayoutBinding& layoutBinding = layout.bindings.back();

                layoutBinding.binding = reflBinding.binding;
                layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflBinding.descriptor_type);
                layoutBinding.descriptorCount = 1;

                for (uint32_t iD = 0; iD < reflBinding.array.dims_count; ++iD) {
                    layoutBinding.descriptorCount *= reflBinding.array.dims[iD];
                }
                layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(module->shader_stage);
                layoutBindingBufferSize = reflBinding.block.size;
            }

            layout.setNumber = reflSet.set;
            layout.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout.createInfo.bindingCount = layout.bindings.size();
            layout.createInfo.pBindings = layout.bindings.data();
        }
    }
} // namespace wiender
#endif // WIENDER_SPV_RENFLECTION_SUPPORT_HPP_