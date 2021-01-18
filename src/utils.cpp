#include "utils.h"

#include <cassert>

//
// Descriptor_Writes
//
Descriptor_Writes& Descriptor_Writes::sampled_image(uint32_t binding, VkImageView image_view, VkImageLayout layout) {
    assert(write_count < max_writes);
    VkDescriptorImageInfo& image = resource_infos[write_count].image;
    image               = VkDescriptorImageInfo{};
    image.imageView     = image_view;
    image.imageLayout   = layout;

    VkWriteDescriptorSet& write = descriptor_writes[write_count++];
    write = VkWriteDescriptorSet { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet             = descriptor_set;
    write.dstBinding         = binding;
    write.descriptorCount    = 1;
    write.descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    write.pImageInfo         = &image;
    return *this;
}

Descriptor_Writes& Descriptor_Writes::storage_image(uint32_t binding, VkImageView image_view) {
    assert(write_count < max_writes);
    VkDescriptorImageInfo& image = resource_infos[write_count].image;
    image               = VkDescriptorImageInfo{};
    image.imageView     = image_view;
    image.imageLayout   = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet& write = descriptor_writes[write_count++];
    write = VkWriteDescriptorSet { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet             = descriptor_set;
    write.dstBinding         = binding;
    write.descriptorCount    = 1;
    write.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.pImageInfo         = &image;
    return *this;
}

Descriptor_Writes& Descriptor_Writes::sampler(uint32_t binding, VkSampler sampler) {
    assert(write_count < max_writes);
    VkDescriptorImageInfo& image = resource_infos[write_count].image;
    image           = VkDescriptorImageInfo{};
    image.sampler   = sampler;

    VkWriteDescriptorSet& write = descriptor_writes[write_count++];
    write = VkWriteDescriptorSet { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet             = descriptor_set;
    write.dstBinding         = binding;
    write.descriptorCount    = 1;
    write.descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo         = &image;
    return *this;
}

Descriptor_Writes& Descriptor_Writes::uniform_buffer(uint32_t binding, VkBuffer buffer_handle, VkDeviceSize offset, VkDeviceSize range) {
    assert(write_count < max_writes);
    VkDescriptorBufferInfo& buffer = resource_infos[write_count].buffer;
    buffer.buffer   = buffer_handle;
    buffer.offset   = offset;
    buffer.range    = range;

    VkWriteDescriptorSet& write = descriptor_writes[write_count++];
    write = VkWriteDescriptorSet { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet             = descriptor_set;
    write.dstBinding         = binding;
    write.descriptorCount    = 1;
    write.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo        = &buffer;
    return *this;
}

Descriptor_Writes& Descriptor_Writes::storage_buffer(uint32_t binding, VkBuffer buffer_handle, VkDeviceSize offset, VkDeviceSize range) {
    assert(write_count < max_writes);
    VkDescriptorBufferInfo& buffer = resource_infos[write_count].buffer;
    buffer.buffer   = buffer_handle;
    buffer.offset   = offset;
    buffer.range    = range;

    VkWriteDescriptorSet& write = descriptor_writes[write_count++];
    write = VkWriteDescriptorSet { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet             = descriptor_set;
    write.dstBinding         = binding;
    write.descriptorCount    = 1;
    write.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo        = &buffer;
    return *this;
}

Descriptor_Writes& Descriptor_Writes::accelerator(uint32_t binding, VkAccelerationStructureNV acceleration_structure) {
    assert(write_count < max_writes);
    Accel_Info& accel_info = resource_infos[write_count].accel_info;
    accel_info.handle = acceleration_structure;

    VkWriteDescriptorSetAccelerationStructureNV& accel = accel_info.accel;
    accel = VkWriteDescriptorSetAccelerationStructureNV { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV };
    accel.accelerationStructureCount = 1;
    accel.pAccelerationStructures = &accel_info.handle;

    VkWriteDescriptorSet& write = descriptor_writes[write_count++];
    write = VkWriteDescriptorSet { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.pNext              = &accel;
    write.dstSet             = descriptor_set;
    write.dstBinding         = binding;
    write.descriptorCount    = 1;
    write.descriptorType     = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    return *this;
}

void Descriptor_Writes::commit() {
    assert(descriptor_set != VK_NULL_HANDLE);
    if (write_count > 0) {
        vkUpdateDescriptorSets(vk.device, write_count, descriptor_writes, 0, nullptr);
        write_count = 0;
    }
}

//
// Descriptor_Set_Layout
//
static VkDescriptorSetLayoutBinding get_set_layout_binding(uint32_t binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags) {
    VkDescriptorSetLayoutBinding entry{};
    entry.binding           = binding;
    entry.descriptorType    = descriptor_type;
    entry.descriptorCount   = 1;
    entry.stageFlags        = stage_flags;
    return entry;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::sampled_image(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::storage_image(uint32_t binding, VkShaderStageFlags stage_flags, VkDescriptorBindingFlags binding_flags) {
    assert(binding_count < max_bindings);
    this->binding_flags[binding_count] = binding_flags;
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::sampler(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_SAMPLER, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::uniform_buffer(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::storage_buffer(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stage_flags);
    return *this;
}

Descriptor_Set_Layout& Descriptor_Set_Layout::accelerator(uint32_t binding, VkShaderStageFlags stage_flags) {
    assert(binding_count < max_bindings);
    bindings[binding_count++] = get_set_layout_binding(binding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, stage_flags);
    return *this;
}

VkDescriptorSetLayout Descriptor_Set_Layout::create(const char* name) {
    bool has_update_after_bind = false;
    for (uint32_t i = 0; i < binding_count; i++)
        if (binding_flags[i] & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT) {
            has_update_after_bind = true;
            break;
        }

    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    binding_flags_info.bindingCount = binding_count;
    binding_flags_info.pBindingFlags = binding_flags;

    VkDescriptorSetLayoutCreateInfo create_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.pNext = has_update_after_bind ? &binding_flags_info : nullptr;
    create_info.flags = has_update_after_bind ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;
    create_info.bindingCount = binding_count;
    create_info.pBindings = bindings;

    VkDescriptorSetLayout set_layout;
    VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &create_info, nullptr, &set_layout));
    vk_set_debug_name(set_layout, name);
    return set_layout;
}

void GPU_Time_Interval::begin() {
    vkCmdWriteTimestamp(vk.command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pool, start_query[vk.frame_index]);
}
void GPU_Time_Interval::end() {
    vkCmdWriteTimestamp(vk.command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pool, start_query[vk.frame_index] + 1);
}

GPU_Time_Interval* GPU_Time_Keeper::allocate_time_interval() {
    assert(time_interval_count < max_time_intervals);
    GPU_Time_Interval* time_interval = &time_intervals[time_interval_count++];

    time_interval->start_query[0] = time_interval->start_query[1] = vk_allocate_timestamp_queries(2);
    time_interval->length_ms = 0.f;
    return time_interval;
}

void GPU_Time_Keeper::initialize_time_intervals() {
    vk_execute(vk.command_pools[0], vk.queue, [this](VkCommandBuffer command_buffer) {
        vkCmdResetQueryPool(command_buffer, vk.timestamp_query_pools[0], 0, 2 * time_interval_count);
        vkCmdResetQueryPool(command_buffer, vk.timestamp_query_pools[1], 0, 2 * time_interval_count);
        for (uint32_t i = 0; i < time_interval_count; i++) {
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[0], time_intervals[i].start_query[0]);
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[0], time_intervals[i].start_query[0] + 1);
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[1], time_intervals[i].start_query[1]);
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vk.timestamp_query_pools[1], time_intervals[i].start_query[1] + 1);
        }
    });
}

void GPU_Time_Keeper::next_frame() {
    uint64_t query_results[2/*query_result + availability*/ * 2/*start + end*/ * max_time_intervals];
    const uint32_t query_count = 2 * time_interval_count;
    VkResult result = vkGetQueryPoolResults(vk.device, vk.timestamp_query_pool, 0, query_count,
        query_count * 2*sizeof(uint64_t), query_results, 2*sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    VK_CHECK_RESULT(result);
    assert(result != VK_NOT_READY);

    const float influence = 0.25f;

    for (uint32_t i = 0; i < time_interval_count; i++) {
        assert(query_results[4*i + 2] >= query_results[4*i]);
        time_intervals[i].length_ms = (1.f-influence) * time_intervals[i].length_ms + influence * float(double(query_results[4*i + 2] - query_results[4*i]) * vk.timestamp_period_ms);
    }

    vkCmdResetQueryPool(vk.command_buffer, vk.timestamp_query_pool, 0, query_count);
}

void begin_gpu_marker_scope(VkCommandBuffer command_buffer, const char* name) {
    VkDebugUtilsLabelEXT label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName = name;
    vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label);
}

void end_gpu_marker_scope(VkCommandBuffer command_buffer) {
    vkCmdEndDebugUtilsLabelEXT(command_buffer);
}

void write_gpu_marker(VkCommandBuffer command_buffer, const char* name) {
    VkDebugUtilsLabelEXT label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName = name;
    vkCmdInsertDebugUtilsLabelEXT(command_buffer, &label);
}

