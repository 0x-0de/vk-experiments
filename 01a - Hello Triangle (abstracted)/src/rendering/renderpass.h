#ifndef _RENDERPASS_H_
#define _RENDERPASS_H_

#include "swapchain.h"

class RenderPass
{
    public:
        RenderPass(VkDevice* device, Swapchain* swapchain);
        virtual ~RenderPass();

        void add_attachment(VkAttachmentDescription desc);
        void add_subpass(VkSubpassDescription desc, VkAttachmentReference* ref);
        void add_subpass_dependency(VkSubpassDependency dep);

        void compile_render_pass();

        VkRenderPassBeginInfo create_default_render_pass_begin_info(size_t image_index);

        VkRenderPass get_render_pass() const;

        bool is_usable() const;
    protected:
        VkDevice* device;
        Swapchain* swapchain;

        VkRenderPass render_pass;

        std::vector<VkAttachmentDescription> attachment_descriptions;
        std::vector<VkAttachmentReference*> attachment_references;

        std::vector<VkSubpassDescription> subpass_descriptions;
        std::vector<VkSubpassDependency> subpass_dependencies;

        void clear_data();

        bool usable;
};

class DefaultColorRenderPass : public RenderPass
{
    public:
        DefaultColorRenderPass(VkDevice* device, Swapchain* swapchain);
        ~DefaultColorRenderPass();
    private:

};

#endif