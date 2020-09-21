#include "./vlr/Implementation.hpp"
#include "./vlr/Rasterization.hpp"
#include "./vlr/PipelineLayout.hpp"
#include "./vlr/Framebuffer.hpp"
#include "./vlr/GeometrySet.hpp"
#include "./vlr/Geometry.hpp"
#include "./vlr/Constants.hpp"
#include "./vlr/InstanceSet.hpp"

namespace vlr {

    // 
    void Rasterization::constructor(vkt::uni_ptr<Driver> driver, vkt::uni_arg<PipelineCreateInfo> info) {
        this->driver = driver; auto device = this->driver->getDeviceDispatch();
        if (info.has()) {
            this->layout = info->layout, this->framebuffer = info->framebuffer, this->instanceSet = info->instanceSet, this->constants = info->constants; 
        };

        // 
        vkh::VsGraphicsPipelineCreateInfoConstruction pipelineInfo = {};

        // 
        this->stages = { // for faster code, pre-initialize
            vkt::makePipelineStageInfo(device, vkt::readBinary(info->translucent.vertexShader), VK_SHADER_STAGE_VERTEX_BIT),
            vkt::makePipelineStageInfo(device, vkt::readBinary(info->translucent.geometryShader), VK_SHADER_STAGE_GEOMETRY_BIT),
            vkt::makePipelineStageInfo(device, vkt::readBinary(info->translucent.fragmentShader), VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        // 
        const auto& viewport = reinterpret_cast<vkh::VkViewport&>(framebuffer->viewport);
        const auto& renderArea = reinterpret_cast<vkh::VkRect2D&>(framebuffer->scissor);

        // Enable Conservative Rasterization For Fix Some Antialiasing Issues
        this->conserv.pNext = nullptr;
        this->conserv.flags = 0u;
        this->conserv.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
        this->conserv.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;//VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
        this->conserv.extraPrimitiveOverestimationSize = 0.f;
        pipelineInfo = vkh::VsGraphicsPipelineCreateInfoConstruction();
        pipelineInfo.stages = this->stages;
        pipelineInfo.depthStencilState = vkh::VkPipelineDepthStencilStateCreateInfo{ .depthTestEnable = true, .depthWriteEnable = true };
        pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
        //pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);          // NEW!
        //pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT); // NEW!
        pipelineInfo.graphicsPipelineCreateInfo.renderPass = framebuffer->rasterFBO.renderPass;
        pipelineInfo.graphicsPipelineCreateInfo.layout = layout->pipelineLayout;
        pipelineInfo.rasterizationState.pNext = &this->conserv;
        pipelineInfo.viewportState.pViewports = viewport;
        pipelineInfo.viewportState.pScissors = renderArea;
        pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        // 
        for (uint32_t i = 0u; i < 8u; i++) {
            pipelineInfo.colorBlendAttachmentStates.push_back(framebuffer->rasterFBO.blendStates[i]); // transparency will generated by ray-tracing
        };
        vkh::handleVk(device->CreateGraphicsPipelines(driver->getPipelineCache(), 1u, pipelineInfo, nullptr, &this->pipeline));


        // Opaque Shader
        pipelineInfo.stages = this->stages = { // for faster code, pre-initialize
            vkt::makePipelineStageInfo(device, vkt::readBinary(info->opaque.vertexShader), VK_SHADER_STAGE_VERTEX_BIT),
            vkt::makePipelineStageInfo(device, vkt::readBinary(info->opaque.geometryShader), VK_SHADER_STAGE_GEOMETRY_BIT),
            vkt::makePipelineStageInfo(device, vkt::readBinary(info->opaque.fragmentShader), VK_SHADER_STAGE_FRAGMENT_BIT)
        };
        vkh::handleVk(device->CreateGraphicsPipelines(driver->getPipelineCache(), 1u, pipelineInfo, nullptr, &this->opaque));

        // Re-Create
        this->geometriesDescs = std::make_shared<BufferViewSet>(this->driver);
    };

    // 
    void Rasterization::setDescriptorSets(vkt::uni_ptr<PipelineLayout> ilayout) { // 
        vkh::VsDescriptorSetCreateInfoHelper descriptorSetInfo = {};

        if (layout.has()) { this->layout = ilayout; };
        
        if (this->constants.has()) {
            if (!this->constants->set) {
                this->constants->createDescriptorSet(layout);
            };
            this->layout->bound[0u] = this->constants->set;
        };

        if (this->instanceSet.has() && this->instanceSet->accelerations.size() > 0) { //
            this->geometriesDescs->resetBufferViews();

            // 
            for (uint32_t i=0;i<this->instanceSet->getGpuBuffer().size();i++) {
                auto& instanceDesc = this->instanceSet->get(i);
                auto& geometrySet = this->instanceSet->accelerations[instanceDesc.customId]->geometrySet;
                this->geometriesDescs->pushBufferView(geometrySet->getGpuBuffer());
            };

            // TODO: Decise by PipelineLayout class
            this->geometriesDescs->createDescriptorSet(layout);
            this->layout->bound[8u] = this->geometriesDescs->set;
        };

        this->layout->setConstants(this->constants);
    };

    // 
    void Rasterization::drawCommand(vkt::uni_arg<VkCommandBuffer> rasterCommand, vkt::uni_arg<glm::uvec4> meta) {
        const auto& viewport = reinterpret_cast<vkh::VkViewport&>(framebuffer->viewport);
        const auto& renderArea = reinterpret_cast<vkh::VkRect2D&>(framebuffer->scissor);
        auto device = this->driver->getDeviceDispatch();

        // 
        auto& instanceDesc = this->instanceSet->get(meta->x);
        auto& geometrySet = this->instanceSet->accelerations[instanceDesc.customId]->geometrySet;
        auto& geometry = geometrySet->geometries[meta->y];

        // 
        device->CmdPushConstants(rasterCommand, layout->pipelineLayout, layout->stages, 0u, sizeof(glm::uvec4), meta);
        device->CmdDraw(rasterCommand, geometry->desc.primitiveCount, 1u, geometry->desc.firstVertex, 0u); // TODO: Instanced Support
    };

    // 
    struct RasterObject {
        vkt::uni_ptr<vlr::Geometry> geometry = {};
        glm::uvec4 idx = glm::uvec4(0u);
    };

    // 
    void Rasterization::setCommand(vkt::uni_arg<VkCommandBuffer> rasterCommand, vkt::uni_arg<glm::uvec4> meta) {
        const auto& viewport = reinterpret_cast<vkh::VkViewport&>(framebuffer->viewport);
        const auto& renderArea = reinterpret_cast<vkh::VkRect2D&>(framebuffer->scissor);
        auto device = this->driver->getDeviceDispatch();

        // 
        vkt::imageBarrier(rasterCommand, vkt::ImageBarrierInfo{
            .image = this->framebuffer->depthStencilImage.getImage(),
            .targetLayout = VK_IMAGE_LAYOUT_GENERAL,
            .originLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .subresourceRange = vkh::VkImageSubresourceRange{ {}, 0u, 1u, 0u, 1u }.also([=](auto* it) {
                auto aspect = vkh::VkImageAspectFlags{.eDepth = 1u, .eStencil = 1u };
                it->aspectMask = aspect;
                return it;
            })
        });

        // 
        for (auto& image : this->framebuffer->rasterImages) {
            device->CmdClearColorImage(rasterCommand, image, image.getImageLayout(), vkh::VkClearColorValue{ .float32 = { 0.f,0.f,0.f,0.f } }, 1u, image.getImageSubresourceRange());
        };
        device->CmdClearDepthStencilImage(rasterCommand, this->framebuffer->depthStencilImage, this->framebuffer->depthStencilImage.getImageLayout(), vkh::VkClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 }, 1u, this->framebuffer->depthStencilImage.getImageSubresourceRange());

        // 
        vkt::commandBarrier(this->driver->getDeviceDispatch(), rasterCommand);
        vkt::imageBarrier(rasterCommand, vkt::ImageBarrierInfo{
            .image = this->framebuffer->depthStencilImage.getImage(),
            .targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .originLayout = VK_IMAGE_LAYOUT_GENERAL,
            .subresourceRange = vkh::VkImageSubresourceRange{ {}, 0u, 1u, 0u, 1u }.also([=](auto* it) {
                auto aspect = vkh::VkImageAspectFlags{.eDepth = 1u, .eStencil = 1u };
                it->aspectMask = aspect;
                return it;
            })
        });

        // 
        std::vector<RasterObject> opaque = {}, translucent = {};

        // 
        for (uint32_t i = 0u; i < this->instanceSet->getGpuBuffer().size(); i++) {
            auto& instanceDesc = this->instanceSet->get(i);
            auto& geometrySet = this->instanceSet->accelerations[instanceDesc.customId]->geometrySet;
            for (uint32_t j = 0; j < geometrySet->geometries.size(); j++) {
                auto& geometry = geometrySet->geometries[j];
                if ((geometrySet->get(j) = geometry->desc).mesh_flags.translucent) {
                    translucent.push_back({
                        .geometry = geometry,
                        .idx = glm::uvec4(i, j, 0u, 0u)
                    });
                } else {
                    opaque.push_back({
                        .geometry = geometry,
                        .idx = glm::uvec4(i, j, 0u, 0u)
                    });
                };
            };
        };

        // Opaque (with early depth tests)
        device->CmdBindPipeline(rasterCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, this->opaque);
        device->CmdBindDescriptorSets(rasterCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->pipelineLayout, 0u, layout->bound.size(), layout->bound.data(), 0u, nullptr);
        device->CmdSetViewport(rasterCommand, 0u, 1u, viewport);
        device->CmdSetScissor(rasterCommand, 0u, 1u, renderArea);
        device->CmdSetPrimitiveTopologyEXT(rasterCommand, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        device->CmdBeginRenderPass(rasterCommand, vkh::VkRenderPassBeginInfo{ .renderPass = framebuffer->rasterFBO.renderPass, .framebuffer = framebuffer->rasterFBO.framebuffer, .renderArea = renderArea, .clearValueCount = static_cast<uint32_t>(framebuffer->rasterFBO.clearValues.size()), .pClearValues = framebuffer->rasterFBO.clearValues.data() }, VK_SUBPASS_CONTENTS_INLINE);
        for (auto& obj : opaque) { this->drawCommand(rasterCommand, obj.idx); };
        device->CmdEndRenderPass(rasterCommand);

        // Translucent
        device->CmdBindPipeline(rasterCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
        device->CmdBindDescriptorSets(rasterCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->pipelineLayout, 0u, layout->bound.size(), layout->bound.data(), 0u, nullptr);
        device->CmdSetViewport(rasterCommand, 0u, 1u, viewport);
        device->CmdSetScissor(rasterCommand, 0u, 1u, renderArea);
        device->CmdSetPrimitiveTopologyEXT(rasterCommand, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        device->CmdBeginRenderPass(rasterCommand, vkh::VkRenderPassBeginInfo{ .renderPass = framebuffer->rasterFBO.renderPass, .framebuffer = framebuffer->rasterFBO.framebuffer, .renderArea = renderArea, .clearValueCount = static_cast<uint32_t>(framebuffer->rasterFBO.clearValues.size()), .pClearValues = framebuffer->rasterFBO.clearValues.data() }, VK_SUBPASS_CONTENTS_INLINE);
        for (auto& obj : translucent) { this->drawCommand(rasterCommand, obj.idx); };
        device->CmdEndRenderPass(rasterCommand);
        vkt::commandBarrier(this->driver->getDeviceDispatch(), rasterCommand);
    };

};
