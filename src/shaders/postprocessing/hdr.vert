#version 330

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

out vec2 fTexCoords;

void main()
{
    fTexCoords = vTexCoord.st;
    gl_Position = vPosition;
}
