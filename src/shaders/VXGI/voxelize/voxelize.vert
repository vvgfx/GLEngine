#version 460 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform mat4 model;
uniform mat4 normalmatrix;
uniform mat4 texturematrix;

uniform vec4 gridMin;
uniform vec4 gridMax;

out passThroughData {
    vec4 worldPos;
    vec4 worldNormal;
    vec4 worldTexCoord;
} outData;

vec4 MapToNdc(vec4 value, vec4 rangeMin, vec4 rangeMax) {
    return ((value - rangeMin) / (rangeMax - rangeMin)) * 2.0 - 1.0;
}

void main()
{
    vec4 fragPos = model * vPosition;
    gl_Position = MapToNdc(fragPos, gridMin, gridMax);

    outData.worldPos = fragPos;
    outData.worldNormal = normalmatrix * vNormal;
    outData.worldNormal = normalize(outData.worldNormal);
    outData.worldTexCoord = texturematrix * vec4(vTexCoord.s, vTexCoord.t, 0, 1);
}
