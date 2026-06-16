#version 330

in vec2 v_ndc;

uniform vec3 colorTop      = vec3(0.10, 0.15, 0.35);
uniform vec3 colorHorizon  = vec3(0.90, 0.45, 0.10);
uniform vec3 colorBottom   = vec3(0.20, 0.10, 0.05);

out vec4 pixelColor;

void main(void) {
    float t = (v_ndc.y + 1.0) * 0.5;     // 0 = dol, 1 = gora
    vec3 c;
    if (t < 0.5) {
        c = mix(colorBottom, colorHorizon, t * 2.0);
    } else {
        c = mix(colorHorizon, colorTop, (t - 0.5) * 2.0);
    }
    pixelColor = vec4(c, 1.0);
}
