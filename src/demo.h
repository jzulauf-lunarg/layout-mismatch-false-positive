#pragma once

#include "copy_to_swapchain.h"
#include "matrix.h"
#include "utils.h"
#include "vk.h"

#include <vector>

struct GLFWwindow;

class Vk_Demo {
public:
    void initialize(GLFWwindow* glfw_window, bool enable_validation_layers);
    void shutdown();

    void release_resolution_dependent_resources();
    void restore_resolution_dependent_resources();
    void run_frame();

private:
    void draw_frame();
    void draw_rasterized_image();
    void copy_output_image_to_swapchain();

private:
    Vk_Image                    output_image;
    Copy_To_Swapchain           copy_to_swapchain;

    VkDescriptorSetLayout       descriptor_set_layout;
    VkPipelineLayout            pipeline_layout;
    VkPipeline                  pipeline;
    VkDescriptorSet             descriptor_set;
    VkRenderPass                render_pass;
    VkFramebuffer               framebuffer;
    Vk_Buffer                   uniform_buffer;
    void*                       mapped_uniform_buffer;

    Vk_Buffer                   vertex_buffer;
    Vk_Buffer                   index_buffer;
    uint32_t                    model_vertex_count;
    uint32_t                    model_index_count;
    Vk_Image                    texture;
    VkSampler                   sampler;

    Vector3                     camera_pos = Vector3(0, 0.5, 3.0);
    Matrix3x4                   model_transform = Matrix3x4::identity;
    Matrix3x4                   view_transform;
};
