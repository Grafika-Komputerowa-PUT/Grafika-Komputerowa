#version 330

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

layout (location=0) in vec3 aPosition;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTexCoord;

out vec2 i_tc;
out vec3 i_normal;
out vec3 i_fragPos;

void main(void) {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);

    vec4 vPos = uView * uModel * vec4(aPosition, 1.0);
    i_fragPos = vPos.xyz;

    mat3 G = inverse(transpose(mat3(uView * uModel)));
    i_normal = normalize(G * aNormal);

    i_tc = aTexCoord;
}