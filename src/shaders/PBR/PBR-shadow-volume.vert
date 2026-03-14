#version 330

// THIS IS FROM BEFORE THE PIPELINE CHANGE. IT'S NOT USED ANYWHERE AND DOES NOT WORK

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform mat4 projection;
uniform mat4 modelview;
// uniform mat4 normalmatrix; // readd this beacuse it seems like doing it once in the cpu is better than doing everytime on the gpu?
uniform mat4 texturematrix;


out vec4 fPosition;
out vec4 fTexCoord;
out vec3 fNormal;
out vec3 fTangent;
out vec3 fBiTangent;

void main()
{

    //gl_Position stuff
    fPosition = modelview * vec4(vPosition.xyzw);
    gl_Position = projection * fPosition;

    //normal transformation stuff
    mat4 normalMatrix = inverse(transpose(modelview));
    vec4 tNormal =  normalMatrix * vNormal;
    fNormal = normalize(tNormal.xyz);

    //tangent stuff
    vec4 tTangent = modelview * vTangent;
    fTangent = normalize(tTangent.xyz);
    //calculating the bitangent here instead of the cpu!
    fBiTangent = cross(fNormal,fTangent);
    fBiTangent = normalize(fBiTangent);

    fTexCoord = texturematrix * vec4(1*vTexCoord.s,1*vTexCoord.t,0,1);

}
