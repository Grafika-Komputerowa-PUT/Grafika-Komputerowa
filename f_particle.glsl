#version 330

uniform sampler2D tex;
uniform bool useTexture = false;

in vec4 v_color;
out vec4 pixelColor;

void main(void) {
    if (useTexture) {
        // Tekstura na czarnym tle - czerni nie widac przy additive
        vec4 t = texture(tex, gl_PointCoord);
        pixelColor = vec4(t.rgb * v_color.rgb, v_color.a);
    } else {
        // Miekkie kolko - dla dymu / generycznych czastek
        vec2 d = gl_PointCoord - vec2(0.5);
        float r = length(d) * 2.0;
        if (r > 1.0) discard;
        float a = 1.0 - r;
        a = a * a;
        pixelColor = vec4(v_color.rgb, v_color.a * a);
    }
}
