
#version 120

varying vec3 vNormal;
varying vec4 vEyePos;

void main(void)
{
    gl_TexCoord[0] = gl_MultiTexCoord0;
    vNormal = normalize(mat3(gl_ModelViewMatrixInverseTranspose) * gl_Normal);
    vEyePos = gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
