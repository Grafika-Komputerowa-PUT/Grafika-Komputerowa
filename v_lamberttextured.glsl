#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

// Slonce - swiatlo kierunkowe (kierunek DO swiatla, juz w przestrzeni widoku)
uniform vec4 sunDir   = vec4(0,1,0,0);
uniform vec4 sunColor = vec4(1.0, 0.95, 0.85, 1.0);

// Lawa - swiatlo punktowe (pozycja w przestrzeni widoku)
uniform vec3  lavaPos       = vec3(0,0,0);
uniform vec4  lavaColor     = vec4(1.0, 0.45, 0.1, 1.0);
uniform float lavaIntensity = 1.5;

uniform vec3 ambient = vec3(0.18, 0.18, 0.22);

layout (location=0) in vec4 vertex;
layout (location=1) in vec4 normal;
layout (location=2) in vec2 texCoord;

out vec2 i_tc;
out vec3 i_light;

void main(void) {
    gl_Position = P*V*M*vertex;

    vec4 vPos = V*M*vertex;
    mat4 G = mat4(inverse(transpose(mat3(V*M))));
    vec3 n = normalize((G * normal).xyz);

    float nlSun = clamp(dot(n, sunDir.xyz), 0.0, 1.0);

    vec3  toLava = lavaPos - vPos.xyz;
    float dist   = length(toLava);
    float nlLava = clamp(dot(n, toLava/dist), 0.0, 1.0);
    float atten  = 1.0 / (1.0 + 0.08*dist + 0.02*dist*dist);

    i_light = ambient
            + sunColor.rgb  * nlSun
            + lavaColor.rgb * nlLava * atten * lavaIntensity;
    i_tc = texCoord;
}
