#version 330

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform mat4 projection;
uniform mat4 modelview;
uniform mat4 normalmatrix;


out vec4 fPosition;
out vec3 fNormal;

void main()
{

    //gl_Position stuff
    fPosition = modelview * vec4(vPosition.xyzw); // view space
    gl_Position = projection * fPosition;

    //normal transformation stuff
    vec4 tNormal =  normalmatrix * vNormal; // now this should also be in the view space!
    fNormal = normalize(tNormal.xyz);

}
