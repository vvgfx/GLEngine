#version 330

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform vec4 vColor;
uniform mat4 modelview;
uniform mat4 projection;
out vec4 outColor;

void main()
{
    gl_Position = projection * modelview * vPosition;
    outColor = vColor;
}
