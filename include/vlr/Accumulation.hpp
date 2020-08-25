#pragma once
#include "./Config.hpp"
#include "./Driver.hpp"

namespace vlr {

    class Accumulation : public std::enable_shared_from_this<Accumulation> { protected: 
    public: 
        Accumulation() { this->constructor(); };
        Accumulation(vkt::uni_ptr<Driver> driver) { this->constructor(driver); };
        ~Accumulation() {};

        virtual void constructor() {};
        virtual void constructor(vkt::uni_ptr<Driver> driver) {
            
        };
    };

};

namespace vlj {
    class Accumulation : public Wrap<vlr::Accumulation> {
        Accumulation() : Wrap<vlr::Accumulation>() {};
        Accumulation(vkt::uni_ptr<vlr::Driver> driver) : Wrap<vlr::Accumulation>(std::make_shared<vlr::Accumulation>(driver)) {};

        //CALLIFY(constructor);
    };
};
