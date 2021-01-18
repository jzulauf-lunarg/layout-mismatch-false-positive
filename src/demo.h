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
    bool vsync_enabled() const { return vsync; }

    void run_frame();

private:
    void draw_frame();
    void draw_rasterized_image();
    void draw_imgui();
    void copy_output_image_to_swapchain();
    void do_imgui();

private:
    using Clock = std::chrono::high_resolution_clock;
    using Time  = std::chrono::time_point<Clock>;

    bool                        show_ui                 = true;
    bool                        vsync                   = true;
    bool                        animate                 = false;

    Time                        last_frame_time;
    double                      sim_time;

    VkRenderPass                ui_render_pass;
    VkFramebuffer               ui_framebuffer;
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
    Matrix3x4                   model_transform;
    Matrix3x4                   view_transform;

    GPU_Time_Keeper             time_keeper;
    struct {
        GPU_Time_Interval*      frame;
        GPU_Time_Interval*      draw;
        GPU_Time_Interval*      ui;
        GPU_Time_Interval*      compute_copy;
    } gpu_times;
};
