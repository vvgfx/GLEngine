#version 330
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vNormal;
layout(location = 2) in vec4 vTexCoord;
layout(location = 3) in vec4 vTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 texturematrix;
uniform mat4 normalmatrix;

out vec4 fTexCoord;
out vec4 fPosition; 
out vec3 fTangent;  
out vec3 fBiTangent;
out vec3 fNormal;   

void main()
{
    fPosition = model * vPosition;
    
    gl_Position = projection * view * fPosition;
    fTexCoord = texturematrix * vec4(vTexCoord.s, vTexCoord.t, 0, 1);

    vec4 tNormal = normalmatrix * vNormal;
    fNormal = normalize(tNormal.xyz);
    
    vec4 tTangent = model * vTangent; 
    fTangent = normalize(tTangent.xyz);
    
    fBiTangent = cross(fNormal, fTangent);
    fBiTangent = normalize(fBiTangent);
}
