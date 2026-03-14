#version 330

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform mat4 projection;
uniform mat4 modelview;
uniform mat4 normalmatrix;
uniform mat4 texturematrix;


out vec4 fPosition;
out vec3 fNormal;
out vec4 fTexCoord;
out vec3 fTangent;
out vec3 fBiTangent;

void main()
{

    //gl_Position stuff
    fPosition = modelview * vec4(vPosition.xyzw); // view space
    gl_Position = projection * fPosition;

    //normal transformation stuff
    vec4 tNormal =  normalmatrix * vNormal; // now this should also be in the view space!
    fNormal = normalize(tNormal.xyz);

    //tangent stuff
    vec4 tTangent = modelview * vTangent; // this is in the view space! (so cosines should give me view to tangent space transformations)
    fTangent = normalize(tTangent.xyz);
    //calculating the bitangent here instead of the cpu!
    fBiTangent = cross(fNormal,fTangent);
    fBiTangent = normalize(fBiTangent);


    fTexCoord = texturematrix * vec4(1*vTexCoord.s,1*vTexCoord.t,0,1);

}
