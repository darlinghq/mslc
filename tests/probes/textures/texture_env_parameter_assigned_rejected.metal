// EXPECT: error which mslc does not assign
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
static float4 look(thread Env& env, thread texture2d<float>& u) { env.texture = &u; return env.texture->sample(*env.sampler, float2(0.5)); }
fragment float4 f(texture2d<float> t [[texture(0)]], texture2d<float> u [[texture(1)]], sampler s [[sampler(0)]]) {
    Env env;
    env.texture = &t;
    env.sampler = &s;
    return look(env, u);
}
