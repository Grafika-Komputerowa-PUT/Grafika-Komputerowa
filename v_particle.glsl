#version 330

uniform mat4 P;
uniform mat4 V;
uniform float pointScale = 800.0;

layout (location=0) in vec3 in_pos;
layout (location=1) in vec4 in_color;
layout (location=2) in float in_size;

out vec4 v_color;

void main(void) {
    vec4 vp = V * vec4(in_pos, 1.0);
    gl_Position = P * vp;
    // Tlumienie rozmiaru z odlegloscia (perspektywa)
    gl_PointSize = max(1.0, in_size * pointScale / max(-vp.z, 0.1));
    v_color = in_color;
}
