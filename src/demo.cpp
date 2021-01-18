#include "common.h"
#include "demo.h"
#include "matrix.h"
#include "mesh.h"
#include "vk.h"
#include "utils.h"

#include "glfw/glfw3.h"

#include <cinttypes>
#include <chrono>

namespace {
struct Uniform_Buffer {
    Matrix4x4   model_view_proj;
    Matrix4x4   model_view;
};
}

void Vk_Demo::initialize(GLFWwindow* window, bool enable_validation_layers) {
    vk_initialize(window, enable_validation_layers);

    // Device properties.
    {
        VkPhysicalDeviceProperties2 physical_device_properties { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        vkGetPhysicalDeviceProperties2(vk.physical_device, &physical_device_properties);

        printf("Device: %s\n", physical_device_properties.properties.deviceName);
        printf("Vulkan API version: %d.%d.%d\n",
            VK_VERSION_MAJOR(physical_device_properties.properties.apiVersion),
            VK_VERSION_MINOR(physical_device_properties.properties.apiVersion),
            VK_VERSION_PATCH(physical_device_properties.properties.apiVersion)
        );
    }

    // Geometry buffers.
    {
        Mesh mesh = load_obj_mesh(get_resource_path("model/mesh.obj"), 1.25f);

        model_vertex_count = static_cast<uint32_t>(mesh.vertices.size());
        model_index_count = static_cast<uint32_t>(mesh.indices.size());
        {
            const VkDeviceSize size = mesh.vertices.size() * sizeof(mesh.vertices[0]);
            vertex_buffer = vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "vertex_buffer");
            vk_ensure_staging_buffer_allocation(size);
            memcpy(vk.staging_buffer_ptr, mesh.vertices.data(), size);

            vk_execute(vk.command_pools[0], vk.queue, [&size, this](VkCommandBuffer command_buffer) {
                VkBufferCopy region;
                region.srcOffset = 0;
                region.dstOffset = 0;
                region.size = size;
                vkCmdCopyBuffer(command_buffer, vk.staging_buffer, vertex_buffer.handle, 1, &region);
            });
        }
        {
            const VkDeviceSize size = mesh.indices.size() * sizeof(mesh.indices[0]);
            index_buffer = vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "index_buffer");
            vk_ensure_staging_buffer_allocation(size);
            memcpy(vk.staging_buffer_ptr, mesh.indices.data(), size);

            vk_execute(vk.command_pools[0], vk.queue, [&size, this](VkCommandBuffer command_buffer) {
                VkBufferCopy region;
                region.srcOffset = 0;
                region.dstOffset = 0;
                region.size = size;
                vkCmdCopyBuffer(command_buffer, vk.staging_buffer, index_buffer.handle, 1, &region);
            });
        }
    }

    // Texture.
    {
        texture = vk_load_texture("model/diffuse.jpg");

        VkSamplerCreateInfo create_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        create_info.magFilter           = VK_FILTER_LINEAR;
        create_info.minFilter           = VK_FILTER_LINEAR;
        create_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.mipLodBias          = 0.0f;
        create_info.anisotropyEnable    = VK_FALSE;
        create_info.maxAnisotropy       = 1;
        create_info.minLod              = 0.0f;
        create_info.maxLod              = 12.0f;

        VK_CHECK(vkCreateSampler(vk.device, &create_info, nullptr, &sampler));
        vk_set_debug_name(sampler, "diffuse_texture_sampler");
    }

    uniform_buffer = vk_create_host_visible_buffer(static_cast<VkDeviceSize>(sizeof(Uniform_Buffer)),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &mapped_uniform_buffer, "uniform_buffer");

    descriptor_set_layout = Descriptor_Set_Layout()
        .uniform_buffer (0, VK_SHADER_STAGE_VERTEX_BIT)
        .sampled_image  (1, VK_SHADER_STAGE_FRAGMENT_BIT)
        .sampler        (2, VK_SHADER_STAGE_FRAGMENT_BIT)
        .create         ("set_layout");

    // Pipeline layout.
    {
        VkPushConstantRange push_constant_range; // show_texture_lods value
        push_constant_range.stageFlags  = VK_SHADER_STAGE_FRAGMENT_BIT;
        push_constant_range.offset      = 0;
        push_constant_range.size        = 4;

        VkPipelineLayoutCreateInfo create_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        create_info.setLayoutCount          = 1;
        create_info.pSetLayouts             = &descriptor_set_layout;
        create_info.pushConstantRangeCount  = 1;
        create_info.pPushConstantRanges     = &push_constant_range;

        VK_CHECK(vkCreatePipelineLayout(vk.device, &create_info, nullptr, &pipeline_layout));
        vk_set_debug_name(pipeline_layout, "pipeline_layout");
    }

    // Render pass.
    {
        VkAttachmentDescription attachments[2] = {};
        attachments[0].format           = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        attachments[1].format           = vk.depth_info.format;
        attachments[1].samples          = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].finalLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_attachment_ref;
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_attachment_ref;
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &color_attachment_ref;
        subpass.pDepthStencilAttachment = &depth_attachment_ref;

        VkRenderPassCreateInfo create_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        create_info.attachmentCount = (uint32_t)std::size(attachments);
        create_info.pAttachments = attachments;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;

        VK_CHECK(vkCreateRenderPass(vk.device, &create_info, nullptr, &render_pass));
        vk_set_debug_name(render_pass, "color_depth_render_pass");
    }

    // Pipeline.
    {
        VkShaderModule vertex_shader = vk_load_spirv("spirv/mesh.vert.spv");
        VkShaderModule fragment_shader = vk_load_spirv("spirv/mesh.frag.spv");

        Vk_Graphics_Pipeline_State state = get_default_graphics_pipeline_state();

        // VkVertexInputBindingDescription
        state.vertex_bindings[0].binding = 0;
        state.vertex_bindings[0].stride = sizeof(Vertex);
        state.vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        state.vertex_binding_count = 1;

        // VkVertexInputAttributeDescription
        state.vertex_attributes[0].location = 0; // vertex
        state.vertex_attributes[0].binding = 0;
        state.vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        state.vertex_attributes[0].offset = 0;

        state.vertex_attributes[1].location = 1; // normal
        state.vertex_attributes[1].binding = 0;
        state.vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        state.vertex_attributes[1].offset = 12;

        state.vertex_attributes[2].location = 2; // uv
        state.vertex_attributes[2].binding = 0;
        state.vertex_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
        state.vertex_attributes[2].offset = 24;
        state.vertex_attribute_count = 3;

        pipeline = vk_create_graphics_pipeline(state, pipeline_layout, render_pass, vertex_shader, fragment_shader);

        vkDestroyShaderModule(vk.device, vertex_shader, nullptr);
        vkDestroyShaderModule(vk.device, fragment_shader, nullptr);
    }

    // Descriptor sets.
    {
        VkDescriptorSetAllocateInfo desc { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        desc.descriptorPool     = vk.descriptor_pool;
        desc.descriptorSetCount = 1;
        desc.pSetLayouts        = &descriptor_set_layout;
        VK_CHECK(vkAllocateDescriptorSets(vk.device, &desc, &descriptor_set));

        Descriptor_Writes(descriptor_set)
            .uniform_buffer (0, uniform_buffer.handle, 0, sizeof(Uniform_Buffer))
            .sampled_image  (1, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .sampler        (2, sampler);
    }

    copy_to_swapchain.create();
    restore_resolution_dependent_resources();
}

void Vk_Demo::shutdown() {
    VK_CHECK(vkDeviceWaitIdle(vk.device));

    vertex_buffer.destroy();
    index_buffer.destroy();
    texture.destroy();
    copy_to_swapchain.destroy();
    vkDestroySampler(vk.device, sampler, nullptr);
    release_resolution_dependent_resources();
    uniform_buffer.destroy();
    vkDestroyDescriptorSetLayout(vk.device, descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(vk.device, pipeline_layout, nullptr);
    vkDestroyPipeline(vk.device, pipeline, nullptr);
    vkDestroyRenderPass(vk.device, render_pass, nullptr);

    vk_shutdown();
}

void Vk_Demo::release_resolution_dependent_resources() {
    vkDestroyFramebuffer(vk.device, framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;

    output_image.destroy();
}

void Vk_Demo::restore_resolution_dependent_resources() {
    // output image
    {
        output_image = vk_create_image(vk.surface_size.width, vk.surface_size.height, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "output_image");
    }
    
    VkImageView attachments[] = {output_image.view, vk.depth_info.image_view};
    VkFramebufferCreateInfo create_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    create_info.renderPass      = render_pass;
    create_info.attachmentCount = (uint32_t)std::size(attachments);
    create_info.pAttachments    = attachments;
    create_info.width           = vk.surface_size.width;
    create_info.height          = vk.surface_size.height;
    create_info.layers          = 1;

    VK_CHECK(vkCreateFramebuffer(vk.device, &create_info, nullptr, &framebuffer));
    vk_set_debug_name(framebuffer, "color_depth_framebuffer");

    copy_to_swapchain.update_resolution_dependent_descriptors(output_image.view);
    last_frame_time = Clock::now();
}

void Vk_Demo::run_frame() {
    Time current_time = Clock::now();
    if (animate) {
        double time_delta = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_frame_time).count() / 1e6;
        sim_time += time_delta;
    }
    last_frame_time = current_time;

    model_transform = rotate_y(Matrix3x4::identity, (float)sim_time * radians(20.0f));
    view_transform = look_at_transform(camera_pos, Vector3(0), Vector3(0, 1, 0));

    float aspect_ratio = (float)vk.surface_size.width / (float)vk.surface_size.height;
    Matrix4x4 proj = perspective_transform_opengl_z01(radians(45.0f), aspect_ratio, 0.1f, 50.0f);
    Matrix4x4 model_view = Matrix4x4::identity * view_transform * model_transform;
    Matrix4x4 model_view_proj = proj * view_transform * model_transform;
    static_cast<Uniform_Buffer*>(mapped_uniform_buffer)->model_view_proj = model_view_proj;
    static_cast<Uniform_Buffer*>(mapped_uniform_buffer)->model_view = model_view;

    Matrix3x4 camera_to_world_transform;
    camera_to_world_transform.set_column(0, Vector3(view_transform.get_row(0)));
    camera_to_world_transform.set_column(1, Vector3(view_transform.get_row(1)));
    camera_to_world_transform.set_column(2, Vector3(view_transform.get_row(2)));
    camera_to_world_transform.set_column(3, camera_pos);

    draw_frame();
}

void Vk_Demo::draw_frame() {
    vk_begin_frame();
    draw_rasterized_image();
    copy_output_image_to_swapchain();
    vk_end_frame();
}

void Vk_Demo::draw_rasterized_image() {
    GPU_MARKER_SCOPE(vk.command_buffer, "draw_rasterized_image");

    VkViewport viewport{};
    viewport.width = static_cast<float>(vk.surface_size.width);
    viewport.height = static_cast<float>(vk.surface_size.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = vk.surface_size;

    vkCmdSetViewport(vk.command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(vk.command_buffer, 0, 1, &scissor);

    VkClearValue clear_values[2];
    clear_values[0].color = {srgb_encode(0.32f), srgb_encode(0.32f), srgb_encode(0.4f), 0.0f};
    clear_values[1].depthStencil.depth = 1.0;
    clear_values[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo render_pass_begin_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    render_pass_begin_info.renderPass        = render_pass;
    render_pass_begin_info.framebuffer       = framebuffer;
    render_pass_begin_info.renderArea.extent = vk.surface_size;
    render_pass_begin_info.clearValueCount   = (uint32_t)std::size(clear_values);
    render_pass_begin_info.pClearValues      = clear_values;

    vkCmdBeginRenderPass(vk.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    const VkDeviceSize zero_offset = 0;
    vkCmdBindVertexBuffers(vk.command_buffer, 0, 1, &vertex_buffer.handle, &zero_offset);
    vkCmdBindIndexBuffer(vk.command_buffer, index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
    vkCmdBindPipeline(vk.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDrawIndexed(vk.command_buffer, model_index_count, 1, 0, 0, 0);
    vkCmdEndRenderPass(vk.command_buffer);
}

void Vk_Demo::copy_output_image_to_swapchain() {
    const uint32_t group_size_x = 32; // according to shader
    const uint32_t group_size_y = 32;

    uint32_t group_count_x = (vk.surface_size.width + group_size_x - 1) / group_size_x;
    uint32_t group_count_y = (vk.surface_size.height + group_size_y - 1) / group_size_y;

    vk_cmd_image_barrier(vk.command_buffer, vk.swapchain_info.images[vk.swapchain_image_index],
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,                                  VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,          VK_IMAGE_LAYOUT_GENERAL);

    uint32_t push_constants[] = { vk.surface_size.width, vk.surface_size.height };

    vkCmdPushConstants(vk.command_buffer, copy_to_swapchain.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(push_constants), push_constants);

    vkCmdBindDescriptorSets(vk.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_to_swapchain.pipeline_layout,
        0, 1, &copy_to_swapchain.sets[vk.swapchain_image_index], 0, nullptr);

    vkCmdBindPipeline(vk.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_to_swapchain.pipeline);
    vkCmdDispatch(vk.command_buffer, group_count_x, group_count_y, 1);

    vk_cmd_image_barrier(vk.command_buffer, vk.swapchain_info.images[vk.swapchain_image_index],
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_SHADER_WRITE_BIT,             0,
        VK_IMAGE_LAYOUT_GENERAL,                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}
