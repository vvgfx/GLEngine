#version 330 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

out vec2 TexCoords;

void main()
{
    TexCoords = vTexCoord.st;
	gl_Position = vPosition;
}
