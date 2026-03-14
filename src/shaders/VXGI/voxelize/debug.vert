#version 460 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 worldPos;

void main()
{
    worldPos = model * vPosition;
    gl_Position = projection * view * worldPos;
}
