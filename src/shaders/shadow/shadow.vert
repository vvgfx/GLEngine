#version 330

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

out vec3 gPosition;

uniform mat4 projection;
uniform mat4 modelview;

void main()
{
    gPosition = vec3(modelview * vPosition); // remember-view coordinates already
    // gPosition = vec3(vPosition);
    // gl_Position = projection * modelview * vPosition;
}
