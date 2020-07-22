#pragma once
#include "./Config.hpp"
#include "./Driver.hpp"
#include "./Geometry.hpp"

// 
namespace vlr {

    class GeometrySet : public SetBase_T<GeometryDesc> { protected: friend Acceleration; friend RayTracing; friend Rasterization;
        std::vector<vkt::uni_ptr<Geometry>> geometries = {};
        std::vector<vkt::uni_ptr<Interpolation>> interpolations = {};
        vkt::uni_ptr<VertexSet> vertexSet = {};
        vkt::uni_ptr<Driver> driver = {};

    public: 
        GeometrySet() : SetBase_T<GeometryDesc>() { this->constructorExtend0(); };
        GeometrySet(vkt::uni_ptr<Driver> driver, vkt::uni_arg<DataSetCreateInfo> info) : SetBase_T<GeometryDesc>(driver, info) { this->constructorExtend0(driver); };

        // 
        virtual void constructorExtend0() {};
        virtual void constructorExtend0(vkt::uni_ptr<Driver> driver) {};
        virtual void pushGeometry(vkt::uni_ptr<Geometry> geometry, vkt::uni_ptr<Interpolation> interpolation);
        virtual void resetGeometries() {
            interpolations.resize(0u);
            geometries.resize(0u);
        };

        // 
        virtual const vkt::VectorBase& getCpuBuffer() const override { return SetBase_T<GeometryDesc>::getCpuBuffer(); };
        virtual const vkt::VectorBase& getGpuBuffer() const override { return SetBase_T<GeometryDesc>::getGpuBuffer(); };
        virtual vkt::VectorBase& getCpuBuffer() override { return SetBase_T<GeometryDesc>::getCpuBuffer(); };
        virtual vkt::VectorBase& getGpuBuffer() override { return SetBase_T<GeometryDesc>::getGpuBuffer(); };
        
        // 
        virtual GeometryDesc& get(const uint32_t& I = 0u) override { return SetBase_T<GeometryDesc>::get(I); };
        virtual const GeometryDesc& get(const uint32_t& I = 0u) const override { return SetBase_T<GeometryDesc>::get(I); };

        // 
        GeometryDesc& operator[](const uint32_t& I) { return this->get(I); };
        const GeometryDesc& operator[](const uint32_t& I) const { return this->get(I); };
    };

};
