// EXPECT: error which is a set of names resolved while compiling and not a value
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
fragment float4 f(texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]]) {
    Env env;
    env.texture = &t;
    env.sampler = &s;
    Env other;
    other = env;
    return other.texture->sample(*other.sampler, float2(0.5));
}
