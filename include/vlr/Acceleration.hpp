#pragma once
#include "./Config.hpp"
#include "./vlr/GeometrySet.hpp"
#include "./vlr/InstanceSet.hpp"

namespace vlr {


    struct AccelerationCreateInfo {
        vkt::uni_ptr<GeometrySet> geometrySet = {};
        vkt::uni_ptr<InstanceSet> instanceSet = {};
        std::vector<VkDeviceSize> initials = {};
    };


    class Acceleration : public std::enable_shared_from_this<Acceleration> { protected: 
        vkt::uni_ptr<InstanceSet> instanceSet = {};
        vkt::uni_ptr<GeometrySet> geometrySet = {};
        vkt::uni_ptr<Driver> driver = {};

        // FOR CREATE (Acceleration Structure)
        vkt::uni_arg<AccelerationCreateInfo>                               info = {};
        vkh::VkAccelerationStructureCreateInfoKHR                          create = {};
        std::vector<vkh::VkAccelerationStructureCreateGeometryTypeInfoKHR> dataCreate = {};

        // FOR BUILD! BUT ONLY SINGLE! (Contain Multiple-Instanced)
        vkh::VkAccelerationStructureBuildGeometryInfoKHR              bdHeadInfo = {};
        vkh::VkAccelerationStructureBuildOffsetInfoKHR                offsetTemp = {};
        std::vector<vkh::VkAccelerationStructureBuildOffsetInfoKHR>   offsetInfo = {};
        std::vector<vkh::VkAccelerationStructureBuildOffsetInfoKHR*>  offsetPtr  = {};

        // But used only one, due transform feedback shaders used... 
        std::vector<vkh::VkAccelerationStructureGeometryKHR>   buildGInfo = {};
        std::vector<vkh::VkAccelerationStructureGeometryKHR*>  buildGPtr  = {};
        vkh::VkAccelerationStructureGeometryKHR                buildGTemp = {}; // INSTANCE TEMPLATE, CAN'T BE ARRAY!


    public: 
        Acceleration() { this->constructor(); };
        Acceleration(vkt::uni_ptr<Driver> driver) { this->constructor(driver); };

        virtual void constructor() {};
        virtual void constructor(vkt::uni_ptr<Driver> driver, vkt::uni_arg<AccelerationCreateInfo> info = {});
        virtual void updateAccelerationStructure(vkt::uni_arg<AccelerationCreateInfo> info, const bool& build = false);
        virtual void buildAccelerationStructureCmd(const VkCommandBuffer& cmd = VK_NULL_HANDLE);
    };

};
