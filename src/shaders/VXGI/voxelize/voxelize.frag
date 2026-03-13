#version 460

// AMD-compatible voxelization: pack color into uint, use standard imageAtomicMax.
// Replaces NVIDIA-specific GL_NV_gpu_shader5 / GL_NV_shader_atomic_fp16_vector.

in passThroughData {
    vec4 worldPos;
    vec4 worldNormal;
    vec4 worldTexCoord;
} data;

struct LightProperties
{
    vec4 position;
    vec3 color;
    vec3 spotDirection;   
    float spotAngleCosine;
};

uniform int numLights;
const int MAXLIGHTS = 10;
uniform LightProperties light[MAXLIGHTS];

layout(binding = 0, r32ui) restrict uniform uimage3D ImgResult;

uniform sampler2D albedoMap;

uniform vec4 gridMin;
uniform vec4 gridMax;

ivec3 worldToVoxelSpace(vec3 worldPos);
uint packColorToUint(vec3 color);


void main()
{
    vec4 fPosition  = data.worldPos;
    vec4 fTexCoord  = data.worldTexCoord;
    vec3 albedo     = pow(texture(albedoMap, vec2(fTexCoord.s,fTexCoord.t)).rgb, vec3(2.2));
    vec3 normal     = data.worldNormal.xyz;

    float spotAttenuation = 1.0f;
    float angle, dist, attenuation, nDotL;

    vec3 lightVec, radiance, lightContribution;
    vec3 Lo = vec3(0.0f);
    for(int i = 0; i < numLights; i++)
    {
         if (light[i].position.w!=0)
            lightVec = normalize(light[i].position.xyz - fPosition.xyz); 
        else
            lightVec = normalize(-light[i].position.xyz);

        bool isSpot = light[i].spotAngleCosine < 0.95;
        angle = 1.0f;
        if(isSpot)
        {
            angle = dot(normalize(-lightVec), normalize(light[i].spotDirection));
            spotAttenuation = 1.0 - (1.0 - angle) * 1.0/(1.0 - light[i].spotAngleCosine);
        }

        spotAttenuation = clamp(spotAttenuation, 0.0f, 1.0f);

        nDotL = max(dot(normal, lightVec), 0.0f);
        dist = length(light[i].position.xyz - fPosition.xyz);
        attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        radiance = light[i].color * attenuation;

        lightContribution = albedo * radiance * nDotL * spotAttenuation;
        Lo += lightContribution;
    }

    vec3 color = Lo;

    // convert to voxel space here!
    ivec3 voxelPos = worldToVoxelSpace(fPosition.xyz);
    uint packedColor = packColorToUint(color);
    imageAtomicMax(ImgResult, voxelPos, packedColor);
}

ivec3 worldToVoxelSpace(vec3 worldPos)
{
    vec3 uvw = (worldPos - gridMin.xyz) / (gridMax.xyz - gridMin.xyz);
    ivec3 voxelPos = ivec3(uvw * imageSize(ImgResult));
    return voxelPos;
}

// Pack color into a uint for atomic comparison.
// Luminance is placed in the MSB so that imageAtomicMax naturally
// selects the brightest contributing fragment.
// Layout: [luminance:8 | blue:8 | green:8 | red:8]
uint packColorToUint(vec3 color)
{
    color = clamp(color, 0.0, 1.0);
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    uint r = uint(color.r * 255.0);
    uint g = uint(color.g * 255.0);
    uint b = uint(color.b * 255.0);
    uint l = uint(luminance * 255.0);
    return (l << 24u) | (b << 16u) | (g << 8u) | r;
}
