#version 330

in vec4 v_color;
out vec4 pixelColor;

void main(void) {
    // Miekkie kolko na point sprite
    vec2 d = gl_PointCoord - vec2(0.5);
    float r = length(d) * 2.0;
    if (r > 1.0) discard;
    float a = 1.0 - r;
    a = a * a;
    pixelColor = vec4(v_color.rgb, v_color.a * a);
}
