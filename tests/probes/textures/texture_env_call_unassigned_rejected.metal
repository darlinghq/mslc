// EXPECT: error is not assigned before the call
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
static float4 look(thread Env& env) { return env.texture->sample(*env.sampler, float2(0.5)); }
fragment float4 f(texture2d<float> t [[texture(0)]]) {
    Env env;
    env.texture = &t;
    return look(env);
}
