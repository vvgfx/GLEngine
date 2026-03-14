#version 330 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

out vec3 localPos;

uniform mat4 projection;
uniform mat4 modelview;

void main()
{
    localPos = vPosition.xyz;  // this is in local space.
    gl_Position =  projection * modelview * vec4(localPos, 1.0);
}
