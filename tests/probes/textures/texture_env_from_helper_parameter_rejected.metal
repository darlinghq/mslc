// EXPECT: error names a texture or sampler parameter of the calling helper
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
static float4 look(thread Env& env) { return env.texture->sample(*env.sampler, float2(0.5)); }
static float4 wrap(texture2d<float> t, sampler s) { Env env; env.texture = &t; env.sampler = &s; return look(env); }
fragment float4 f(texture2d<float> t [[texture(0)]], sampler s [[sampler(0)]]) { return wrap(t, s); }
