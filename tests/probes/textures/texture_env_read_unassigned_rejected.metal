// EXPECT: error is read before it is assigned
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
fragment float4 f(texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]]) {
    Env env;
    env.texture = &t;
    return env.texture->sample(*env.sampler, float2(0.5));
}
