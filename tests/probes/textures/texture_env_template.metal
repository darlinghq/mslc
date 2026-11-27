// EXPECT: valid
// The shape of the texture environment ANGLE's Metal backend gives every WebGL shader: a class template on the
// texture type, a function template deducing its element type, and the address taken through a template.
#include <metal_stdlib>
template <typename T>
struct TextureEnv
{
    thread T *texture;
    thread metal::sampler *sampler;
};
template <typename T>
__attribute__((always_inline)) auto textureLookup(thread TextureEnv<metal::texture2d<T>> &env, metal::float2 const coord)
{
    return env.texture->sample(*env.sampler, coord);
}
template <typename T>
__attribute__((always_inline)) thread T *addressOf(thread T &ref)
{
    return &ref;
}
struct Envs
{
    TextureEnv<metal::texture2d<float>> map;
};
struct In { metal::float2 uv [[user(locn0)]]; };
static metal::float4 body(thread Envs &envs, thread In &in)
{
    return textureLookup(envs.map, in.uv);
}
fragment metal::float4 fs(In in [[stage_in]], metal::texture2d<float> tex [[texture(0)]], metal::sampler smp [[sampler(0)]])
{
    Envs envs;
    envs.map.texture = addressOf(tex);
    envs.map.sampler = addressOf(smp);
    return body(envs, in);
}
