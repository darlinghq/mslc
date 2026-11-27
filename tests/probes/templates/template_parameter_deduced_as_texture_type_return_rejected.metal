// EXPECT: error returns a pointer, an array, a texture or sampler, or a value in an address space
// A type parameter deduced as a texture type cannot be the return type, as texture2d<float> cannot.
#include <metal_stdlib>
using namespace metal;
template <typename Tex> static Tex g(Tex t) { return t; }
fragment float4 f(texture2d<float> a [[texture(0)]]) { g(a); return float4(0); }
