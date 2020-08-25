#pragma once
#include "./Config.hpp"
#include "./Driver.hpp"
#include "./SetBase.hpp"

namespace vlr {

    using BindingSet = SetBase_T<VkVertexInputBindingDescription>;

};

namespace vlj {
    class BindingSet : public Wrap<vlr::BindingSet> {
        BindingSet() : Wrap<vlr::BindingSet>() {};
        BindingSet(vkt::uni_ptr<vlr::Driver> driver, vkt::uni_arg<vlr::DataSetCreateInfo> info = {}) : Wrap<vlr::BindingSet>(std::make_shared<vlr::BindingSet>(driver, info)) {};

        //CALLIFY(constructor);
        CALLIFY(getCpuBuffer);
        CALLIFY(getGpuBuffer);
        CALLIFY(get);
        CALLIFY(createDescriptorSet);
        CALLIFY(setCommand);
        CALLIFY(getVector);
    };
};
