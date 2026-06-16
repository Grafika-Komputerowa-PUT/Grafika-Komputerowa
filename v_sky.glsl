#version 330

layout (location=0) in vec2 in_pos;

out vec2 v_ndc;

void main(void) {
    // Pelno-ekranowy quad w NDC, na samym tylnym planie (z = 1)
    gl_Position = vec4(in_pos, 1.0, 1.0);
    v_ndc = in_pos;
}
