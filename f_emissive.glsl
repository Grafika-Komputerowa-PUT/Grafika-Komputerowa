#version 330

uniform sampler2D tex;
uniform vec3  tint      = vec3(1.0, 0.55, 0.18);
uniform float intensity = 1.0;

in vec2 i_tc;

out vec4 pixelColor;

void main(void) {
    vec4 c = texture(tex, i_tc);
    // Tekstura na czarnym tle - additive blending zniweluje czarne
    pixelColor = vec4(c.rgb * tint * intensity, c.a);
}
