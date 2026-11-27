// EXPECT: valid
// Each call of look names its own texture and sampler, so look is emitted once per pair.
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
struct Envs { Env a; Env b; };
static float4 look(thread Env& env, float2 c) { return env.texture->sample(*env.sampler, c); }
static float4 body(thread Envs& envs, float2 c) { return look(envs.a, c) + look(envs.b, c * 2.0); }
fragment float4 f(texture2d<float> t0 [[texture(0)]], sampler s0 [[sampler(0)]],
    texture2d<float> t1 [[texture(1)]], sampler s1 [[sampler(1)]], float4 p [[position]]) {
    Envs envs;
    {
        envs.a.texture = &t0;
        envs.a.sampler = &s0;
        envs.b.texture = &t1;
        envs.b.sampler = &s1;
        return body(envs, p.xy);
    }
}
