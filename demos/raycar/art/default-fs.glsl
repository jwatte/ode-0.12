#version 120

uniform sampler2D texAmbient;
uniform sampler2D texDiffuse;
uniform sampler2D texSpecular;
uniform sampler2D texEmissive;

varying vec3 vNormal;
varying vec4 vEyePos;

void main(void)
{
    //  do the Blinn-Phong thing
    vec3 vnn = normalize(vNormal);
    float NdotL = clamp(dot(vnn, gl_LightSource[0].position.xyz), 0, 1);
    vec3 H = normalize(gl_LightSource[0].position.xyz - normalize(vEyePos.xyz));
    float NdotH = clamp(dot(vnn, H), 0, 1);
    float sPow = pow(NdotH, max(1, gl_FrontMaterial.shininess));
    vec2 tuv = gl_TexCoord[0].xy;
    vec4 color = 
        (gl_FrontLightProduct[0].ambient + gl_LightModel.ambient) * texture2D(texAmbient, tuv) +
        gl_FrontLightProduct[0].diffuse * texture2D(texDiffuse, tuv) * NdotL +
        gl_FrontLightProduct[0].specular * texture2D(texSpecular, tuv) * sPow +
        gl_FrontMaterial.emission * texture2D(texEmissive, tuv);
    gl_FragColor = color;
    // gl_FragColor = vec4(vNormal, 1) * 0.5 + vec4(0.5, 0.5, 0.5, 0.5);
}

