// EXPECT: error is assigned under a condition or in a loop
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
fragment float4 f(texture2d<float> t [[texture(0)]], texture2d<float> u [[texture(1)]], sampler s [[sampler(0)]], float4 p [[position]]) {
    Env env;
    env.sampler = &s;
    if (p.x > 1.0) { env.texture = &t; } else { env.texture = &u; }
    return env.texture->sample(*env.sampler, float2(0.5));
}
