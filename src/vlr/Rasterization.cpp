#include "./vlr/Rasterization.hpp"
#include "./vlr/PipelineLayout.hpp"
#include "./vlr/Framebuffer.hpp"
#include "./vlr/GeometrySet.hpp"
#include "./vlr/Geometry.hpp"
#include "./vlr/Constants.hpp"
#include "./vlr/InstanceSet.hpp"
#include "./vlr/Interpolation.hpp"

namespace vlr {

    // 
    void Rasterization::constructor(vkt::uni_ptr<Driver> driver, vkt::uni_arg<PipelineCreateInfo> info) {
        this->driver = driver; auto device = this->driver->getDeviceDispatch();
        if (info.has()) {
            this->layout = info->layout, this->framebuffer = info->framebuffer, this->instanceSet = info->instanceSet, this->geometrySets = info->geometrySets, this->constants = info->constants; 
        };
        
        // 
        this->stages = { // for faster code, pre-initialize
            vkt::makePipelineStageInfo(device, vkt::readBinary(std::string("./shaders/rasterize.vert.spv")), VK_SHADER_STAGE_VERTEX_BIT),
            vkt::makePipelineStageInfo(device, vkt::readBinary(std::string("./shaders/rasterize.geom.spv")), VK_SHADER_STAGE_GEOMETRY_BIT),
            vkt::makePipelineStageInfo(device, vkt::readBinary(std::string("./shaders/rasterize.frag.spv")), VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        // 
        const auto& viewport = reinterpret_cast<vkh::VkViewport&>(framebuffer->viewport);
        const auto& renderArea = reinterpret_cast<vkh::VkRect2D&>(framebuffer->scissor);

        // Enable Conservative Rasterization For Fix Some Antialiasing Issues
        this->conserv.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
        this->pipelineInfo = vkh::VsGraphicsPipelineCreateInfoConstruction();
        this->pipelineInfo.stages = this->stages;
        this->pipelineInfo.depthStencilState = vkh::VkPipelineDepthStencilStateCreateInfo{ .depthTestEnable = true, .depthWriteEnable = true };
        this->pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        this->pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
        this->pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);          // NEW!
        this->pipelineInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT); // NEW!
        this->pipelineInfo.graphicsPipelineCreateInfo.renderPass = framebuffer->rasterFBO.renderPass;
        this->pipelineInfo.graphicsPipelineCreateInfo.layout = layout->pipelineLayout;
        this->pipelineInfo.rasterizationState.pNext = &this->conserv;
        this->pipelineInfo.viewportState.pViewports = viewport;
        this->pipelineInfo.viewportState.pScissors = renderArea;

        // 
        for (uint32_t i = 0u; i < 8u; i++) {
            this->pipelineInfo.colorBlendAttachmentStates.push_back(framebuffer->rasterFBO.blendStates[i]); // transparency will generated by ray-tracing
        };

        // 
        vkh::handleVk(device->CreateGraphicsPipelines(driver->getPipelineCache(), 1u, this->pipelineInfo, nullptr, &this->pipeline));
    };

    // 
    void Rasterization::setDescriptorSets(vkt::uni_ptr<PipelineLayout> ilayout) { // 
        if (layout.has()) { this->layout = ilayout; };
        
        if (this->constants.has()) {
            if (!this->constants->set) {
                this->constants->createDescriptorSet(layout);
            };
            this->layout->bound[0u] = this->constants->set;
        };

        if (this->instanceSet.has() && this->geometrySets.size() > 0) { // 
            this->interpolations->resetBufferViews();
            this->geometriesDescs->resetBufferViews();

            // 
            for (uint32_t i=0;i<this->instanceSet->getGpuBuffer().size();i++) {
                auto instanceDesc = this->instanceSet->get(i);
                auto geometrySet = this->geometrySets[instanceDesc.customId];
                vkt::uni_ptr<Interpolation> interpolation = geometrySet->interpolations;

                // 
                this->interpolations->pushBufferView(interpolation->getGpuBuffer());
                this->geometriesDescs->pushBufferView(geometrySet->getGpuBuffer());
            };

            // 
            this->interpolations->createDescriptorSet(layout);
            this->geometriesDescs->createDescriptorSet(layout);
        
            // TODO: Decise by PipelineLayout class
            this->layout->bound[8u] = this->geometriesDescs->set;
            this->layout->bound[9u] = this->interpolations->set;
        };

        this->layout->setConstants(this->constants);
    };

    // 
    void Rasterization::drawCommand(vkt::uni_arg<VkCommandBuffer> rasterCommand, const glm::uvec4& meta) {
        const auto& viewport = reinterpret_cast<vkh::VkViewport&>(framebuffer->viewport);
        const auto& renderArea = reinterpret_cast<vkh::VkRect2D&>(framebuffer->scissor);
        auto device = this->driver->getDeviceDispatch();

        // 
        device->CmdBindPipeline(rasterCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
        device->CmdBindDescriptorSets(rasterCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->pipelineLayout, 0u, layout->bound.size(), layout->bound.data(), 0u, nullptr);
        device->CmdSetViewport(rasterCommand, 0u, 1u, viewport);
        device->CmdSetScissor(rasterCommand, 0u, 1u, renderArea);
        device->CmdSetPrimitiveTopologyEXT(rasterCommand, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        device->CmdPushConstants(rasterCommand, layout->pipelineLayout, layout->stages, 0u, sizeof(meta), &meta);
        device->CmdBeginRenderPass(rasterCommand, vkh::VkRenderPassBeginInfo{ .renderPass = framebuffer->rasterFBO.renderPass, .framebuffer = framebuffer->rasterFBO.framebuffer, .renderArea = renderArea, .clearValueCount = static_cast<uint32_t>(framebuffer->rasterFBO.clearValues.size()), .pClearValues = framebuffer->rasterFBO.clearValues.data() }, VK_SUBPASS_CONTENTS_INLINE);

        // 
        auto instanceDesc = this->instanceSet->get(meta.x);
        auto geometrySet = this->geometrySets[instanceDesc.customId];
        auto geometry = geometrySet->geometries[meta.y];

        // 
        if (geometry->desc->indexType != VK_INDEX_TYPE_NONE_KHR && geometry->desc->indexBufferView != ~0u && geometry->desc->indexBufferView != -1) {
            const auto& buffer = geometry->vertexSet->getBuffer(geometry->desc->indexBufferView);
            device->CmdBindIndexBuffer(rasterCommand, buffer, buffer.offset(), geometry->desc->indexType);
            device->CmdDrawIndexed(rasterCommand, geometry->desc->primitiveCount * 3u, 1u, geometry->desc->firstVertex, 0u, 0u);
        } else {
            device->CmdDraw(rasterCommand, geometry->desc->primitiveCount * 3u, 1u, geometry->desc->firstVertex, 0u); // TODO: Instanced Support
        };

        // 
        device->CmdEndRenderPass(rasterCommand);
    };

    // 
    void Rasterization::setCommand(vkt::uni_arg<VkCommandBuffer> rasterCommand, const glm::uvec4& meta) {
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
            this->driver->getDeviceDispatch()->CmdClearColorImage(rasterCommand, image, image.getImageLayout(), vkh::VkClearColorValue{ .float32 = { 0.f,0.f,0.f,0.f } }, 1u, image.getImageSubresourceRange());
        };
        this->driver->getDeviceDispatch()->CmdClearDepthStencilImage(rasterCommand, this->framebuffer->depthStencilImage, this->framebuffer->depthStencilImage.getImageLayout(), vkh::VkClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 }, 1u, this->framebuffer->depthStencilImage.getImageSubresourceRange());

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
        for (uint32_t i=0;i<this->instanceSet->getGpuBuffer().size();i++) {
            auto instanceDesc = this->instanceSet->get(i);
            auto geometrySet = this->geometrySets[instanceDesc.customId];
            for (uint32_t j=0;j<geometrySet->geometries.size();j++) {
                auto geometry = geometrySet->geometries[j];
                this->drawCommand(rasterCommand, glm::uvec4(i, j, 0u, 0u));
            };
        };
        vkt::commandBarrier(this->driver->getDeviceDispatch(), rasterCommand);
    };

};
