
#version 120

varying vec4 color;

void main(void)
{
    gl_TexCoord[0] = gl_MultiTexCoord0;
    color = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
