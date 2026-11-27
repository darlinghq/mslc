// EXPECT: valid
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
fragment float4 f(texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]]) {
    Env env;
    env.texture = &t;
    env.sampler = &s;
    Env copy = env;
    return copy.texture->sample(*copy.sampler, float2(0.5));
}
