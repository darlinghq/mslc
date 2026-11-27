// EXPECT: error has texture or sampler pointer members beside other members
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
struct Mixed { thread texture2d<float>* texture; float scale; };
fragment float4 f(texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]]) {
    Mixed m;
    m.texture = &t;
    return m.texture->sample(s, float2(0.5)) * 2.0;
}
