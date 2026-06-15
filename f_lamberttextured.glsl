#version 330

uniform sampler2D tex;

out vec4 pixelColor;

in vec2 i_tc;
in vec3 i_light;

void main(void) {
    vec4 c = texture(tex, i_tc);
    pixelColor = vec4(c.rgb * i_light, c.a);
}
