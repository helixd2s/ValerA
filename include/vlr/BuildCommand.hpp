#pragma once
#include "./Config.hpp"

namespace vlr {
    
    class BuildCommand : public std::enable_shared_from_this<BuildCommand> { protected: 
    public: 
        BuildCommand() { this->constructor(); };
        BuildCommand(vkt::uni_ptr<Driver> driver) { this->constructor(driver); };

        void constructor() {};
        void constructor(vkt::uni_ptr<Driver> driver) {
            
        };
    };

};
