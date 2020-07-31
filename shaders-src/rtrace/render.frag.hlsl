//#define DRIVER_INCLUDE_H
#include "../include/driver.hlsli"

//#define FXAA_PC 1
//#define FXAA_GLSL_130 1
//#define FXAA_QUALITY_PRESET 39
//#include "./fxaa3_11.h"

#ifdef GLSL
layout ( location = 0 ) out float4 uFragColor;
layout ( location = 0 ) in float2 vcoord;

struct PSInput
{
    float4 position;
    float2 vcoord;
};

//
struct PSOutput 
{
    float4 uFragColor;
};

#else
// 
struct PSInput
{
    float4 position : SV_POSITION;
    float2 vcoord : COLOR;
};

//
struct PSOutput 
{
    float4 uFragColor : SV_TARGET0;
};
#endif

// 
#ifdef GLSL
void main() 
#else
PSOutput main(in PSInput inp)
#endif
{ // TODO: explicit sampling 
#ifdef GLSL
    PSInput inp;
    inp.position = gl_FragCoord;
    inp.vcoord = vcoord;
#endif
    //const int2 size = int2(textureSize(currImages[BW_RENDERED], 0)), samplep = int2(inp.position.x, inp.position.y);

    // Final Result Rendering
    PSOutput outp;
    outp.uFragColor = 0.f.xxxx;
    
    int2 size = imageSize(currImages[IW_RENDERED]); size.x >> 2;//
    outp.uFragColor = fromLinear(superImageLoad(currImages[IW_RENDERED], int2(inp.position)));

#ifdef GLSL
    uFragColor = outp.uFragColor;
#else
    return outp;
#endif
};
