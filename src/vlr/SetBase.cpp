#include "./vlr/Implementation.hpp"
#include "./vlr/PipelineLayout.hpp"

namespace vlr {

    void SetBase::constructor(vkt::uni_ptr<Driver> driver, vkt::uni_arg<DataSetCreateInfo> info, const uint32_t& stride) {
        this->driver = driver;
        auto hostUsage = vkh::VkBufferUsageFlags{.eTransferSrc = 1, .eTransferDst = 1, .eUniformBuffer = 1, .eStorageBuffer = 1, .eRayTracing = 1 };
        auto gpuUsage = vkh::VkBufferUsageFlags{.eTransferSrc = 1, .eTransferDst = 1, .eUniformBuffer = 1, .eStorageBuffer = 1, .eIndexBuffer = 1, .eVertexBuffer = 1, .eRayTracing = 1, .eSharedDeviceAddress = 1 };
        auto upstreamUsage = vkh::VkBufferUsageFlags{ .eTransferSrc = 1, .eTransferDst = 1, .eUniformBuffer = 1, .eStorageBuffer = 1, .eIndirectBuffer = 1, .eRayTracing = 1, .eTransformFeedbackBuffer = 1, .eTransformFeedbackCounterBuffer = 1, .eSharedDeviceAddress = 1 };

        // 
        vkt::MemoryAllocationInfo almac = {};
        almac.memUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        almac.glMemory = almac.glID = 0u;
        almac.queueFamilyIndices = {};
        almac.memoryProperties = driver->getMemoryProperties().memoryProperties;
        almac.instanceDispatch = driver->getInstanceDispatch();
        almac.deviceDispatch = driver->getDeviceDispatch();
        almac.instance = driver->getInstance();
        almac.device = driver->getDevice();

        // 
        auto allocator = this->driver->getAllocator();
        if (info->enableCPU) {
            this->getCpuBuffer() = vkt::VectorBase(std::make_shared<vkt::VmaBufferAllocation>(allocator, vkh::VkBufferCreateInfo{ .size = stride * info->count, .usage = hostUsage }, vkt::VmaMemoryInfo{ .memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU }), 0ull, stride * info->count, stride);
        };
        if (info->enableGL) {
            this->getGpuBuffer() = vkt::VectorBase(std::make_shared<vkt::BufferAllocation>(vkh::VkBufferCreateInfo{ .size = stride * info->count, .usage = gpuUsage }, almac), 0ull, stride * info->count, stride);
        } else {
            this->getGpuBuffer() = vkt::VectorBase(std::make_shared<vkt::VmaBufferAllocation>(allocator, vkh::VkBufferCreateInfo{ .size = stride * info->count, .usage = gpuUsage }, vkt::VmaMemoryInfo{ .memUsage = VMA_MEMORY_USAGE_GPU_ONLY }), 0ull, stride * info->count, stride);
        };
        this->uniform = info->uniform, this->enableCPU = info->enableCPU, this->enableGL = info->enableGL;
    };

    void SetBase::createDescriptorSet(vkt::uni_ptr<PipelineLayout> pipelineLayout) {
        vkh::VsDescriptorSetCreateInfoHelper descriptorSetInfo = {};
        descriptorSetInfo = vkh::VsDescriptorSetCreateInfoHelper(uniform ? pipelineLayout->getUniformSetLayout() : pipelineLayout->getSetLayout(), pipelineLayout->getDescriptorPool());
        auto& handle = descriptorSetInfo.pushDescription(vkh::VkDescriptorUpdateTemplateEntry{
            .dstBinding = 0u,
            .dstArrayElement = 0u,
            .descriptorCount = 1u,
            .descriptorType = uniform ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        });
        handle.offset<VkDescriptorBufferInfo>(0) = this->getGpuBuffer();
        vkh::handleVk(vkh::AllocateDescriptorSetWithUpdate(driver->getDeviceDispatch(), descriptorSetInfo, this->set, this->updated));
    };

    void SetBase::setCommand(vkt::uni_arg<VkCommandBuffer> commandBuffer, bool barrier) {
        auto device = driver->getDeviceDispatch();
        if (this->getCpuBuffer().has()) {
            vkh::VkBufferCopy region = { .srcOffset = this->getCpuBuffer().offset(), .dstOffset = this->getGpuBuffer().offset(), .size = this->getGpuBuffer().range() };
            device->CmdCopyBuffer(commandBuffer, this->getCpuBuffer(), this->getGpuBuffer(), 1, region);
        };
        if (barrier) { vkt::commandBarrier(device, commandBuffer); }; // TODO: Advanced Barrier
    };

};
