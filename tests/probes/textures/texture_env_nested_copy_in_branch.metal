// EXPECT: valid
// A copy declared under a condition takes its members' assignments there, nested members included.
#include <metal_stdlib>
using namespace metal;
struct Env { thread texture2d<float>* texture; thread sampler* sampler; };
struct Outer { Env a; };
static float4 look(thread Outer& o) { return o.a.texture->sample(*o.a.sampler, float2(0.5)); }
fragment float4 f(texture2d<float> t [[texture(0)]], sampler s0 [[sampler(0)]], sampler s1 [[sampler(1)]],
    float4 p [[position]]) {
    Outer o;
    o.a.texture = &t;
    if (p.x > 1.0) {
        Outer c = o;
        c.a.sampler = &s1;
        return look(c);
    }
    o.a.sampler = &s0;
    return look(o);
}
