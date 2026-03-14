#version 330

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform mat4 modelview;
uniform mat4 projection;
out vec3 texCoord;

void main()
{
    vec4 pos = projection * modelview * vPosition;
    texCoord = vec3(vPosition);
    gl_Position = pos.xyww;
}
