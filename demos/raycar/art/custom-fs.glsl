#version 120

uniform sampler2D texCustom;
varying vec4 color;

void main(void)
{
    vec2 tuv = gl_TexCoord[0].xy;
    gl_FragColor = color * texture2D(texCustom, tuv);
}

