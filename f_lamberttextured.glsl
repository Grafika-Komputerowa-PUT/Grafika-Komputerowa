#version 330

uniform sampler2D tex;

// --- Narzucona struktura z wymagań GitHuba ---
struct Light {
    vec3 position; // Dla światła kierunkowego tu będzie kierunek (jako wektor)
    vec3 color;
    float intensity;
};

uniform Light uLights[4];
uniform int uNumLights;

in vec2 i_tc;
in vec3 i_normal;
in vec3 i_fragPos;

out vec4 pixelColor;

void main(void) {
    vec3 N = normalize(i_normal);
    vec3 V = normalize(-i_fragPos);
    
    float ka = 0.2;
    float kd = 1.0;
    float ks = 0.4;
    float shininess = 16.0;

    // Startujemy z bazowym ambientem (od słońca - uLights[0])
    vec3 ambient = ka * uLights[0].color;
    vec3 totalDiffuseSpecular = vec3(0.0);

    // Pętla po wszystkich aktywnych światłach (uNumLights = 2)
    for(int i = 0; i < uNumLights; i++) {
        vec3 L;
        float attenuation = 1.0;

        if (i == 0) {
            // Światło 0: Słońce (kierunkowe)
            L = normalize(uLights[i].position);
        } else {
            // Światło 1: Lawa w kraterze (punktowe)
            vec3 L_dir = uLights[i].position - i_fragPos;
            float dist = length(L_dir);
            L = normalize(L_dir);
            
            // Tłumienie z taska LIGHT-03
            attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        }

        // Diffuse
        vec3 diffuse = kd * uLights[i].color * max(dot(N, L), 0.0) * uLights[i].intensity * attenuation;

        // Specular (Wzór z wykładu)
        vec3 R = normalize(2.0 * dot(N, L) * N - L);
        vec3 specular = ks * uLights[i].color * pow(max(dot(R, V), 0.0), shininess) * uLights[i].intensity * attenuation;

        totalDiffuseSpecular += (diffuse + specular);
    }

    vec3 finalLight = ambient + totalDiffuseSpecular;
    vec4 texColor = texture(tex, i_tc);
    pixelColor = vec4(texColor.rgb * finalLight, texColor.a);
}