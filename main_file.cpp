/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "assimp_model.h"
#include "particles.h"
#include "stones.h"

// Stan kamery orbitalnej (wspolrzedne sferyczne)
float cameraAngleX = 0.8f;
float cameraAngleY = 0.0f;
float cameraRadius = 14.0f;

// Stan myszy
bool mousePressed = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

// Tekstury
GLuint texGround  = 0;
GLuint texVolcano = 0;
GLuint texRock    = 0;

// Modele wczytywane przez Assimp
AssimpModel volcanoModel;
AssimpModel rockModel;

// Skala wulkanu (model jest znormalizowany do max wymiaru = 1)
const float VOLCANO_SCALE = 5.0f;

// Pozycja krateru (z ktorej startuja czastki/kamienie i bije swiatlo lawy)
const glm::vec3 CRATER_POS = glm::vec3(0.0f, VOLCANO_SCALE * 0.95f, 0.0f);

// Systemy erupcji
ParticleSystem lavaParticles;
ParticleSystem smokeParticles;
StoneSystem    stoneSystem;

// Stan erupcji - faza cyklu (0..1) i akumulatory emisji
float eruptionCycleLen = 4.5f;
float prevBurst = 0.0f;
float emitLavaAcc  = 0.0f;
float emitSmokeAcc = 0.0f;
double lastFrameTime = 0.0;

// Kamienie spoczywajace wokol wulkanu (x, z, skala, rotacja_y)
struct RockPlacement { float x, z, scale, rotY; };
static const RockPlacement rockPlacements[] = {
    {  4.4f,  0.2f, 0.90f, 0.4f },
    { -4.0f,  1.6f, 0.75f, 1.2f },
    {  2.6f, -3.8f, 0.70f, 2.7f },
    { -2.4f, -4.1f, 1.00f, 0.9f },
    {  3.9f,  3.4f, 0.60f, 1.8f },
    { -4.5f, -1.2f, 0.85f, 3.4f },
    {  1.5f,  4.3f, 0.55f, 5.1f }
};
static const int rockCount = sizeof(rockPlacements) / sizeof(rockPlacements[0]);

GLuint loadTexture(const char* filename) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    unsigned char* data;
    unsigned w, h;
    unsigned error = lodepng_decode32_file(&data, &w, &h, filename);
    if (error) {
        fprintf(stderr, "Blad ladowania tekstury: %s\n", filename);
        return 0;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    free(data);
    return tex;
}

void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else {
            mousePressed = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mousePressed) return;
    float dx = (float)(xpos - lastMouseX) * 0.01f;
    float dy = (float)(ypos - lastMouseY) * 0.01f;
    cameraAngleY += dx;
    cameraAngleX += dy;
    if (cameraAngleX < 0.05f)       cameraAngleX = 0.05f;
    if (cameraAngleX > PI - 0.05f)  cameraAngleX = PI - 0.05f;
    lastMouseX = xpos;
    lastMouseY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraRadius -= (float)yoffset * 0.5f;
    if (cameraRadius < 3.0f)  cameraRadius = 3.0f;
    if (cameraRadius > 40.0f) cameraRadius = 40.0f;
}

void initOpenGLProgram(GLFWwindow* window) {
    initShaders();
    glClearColor(0.1f, 0.1f, 0.2f, 1);
    glEnable(GL_DEPTH_TEST);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    texGround  = loadTexture("bricks_diffuse.png");
    texVolcano = loadTexture("volcano/textures/Volcano_AOAmbient_Occlusion.png");
    texRock    = loadTexture("renders/rock-low-polygon/textures/DefaultMaterial_Base_color.png");

    volcanoModel.load("volcano/source/Volcano_Lowpoly.fbx");
    rockModel.load   ("renders/rock-low-polygon/source/Rock/Rock.fbx");

    // Systemy erupcji
    lavaParticles.emitterPos = CRATER_POS;
    lavaParticles.gravity    = glm::vec3(0.0f, -7.5f, 0.0f);
    lavaParticles.additive   = true;

    smokeParticles.emitterPos = CRATER_POS + glm::vec3(0.0f, 0.3f, 0.0f);
    smokeParticles.gravity    = glm::vec3(0.0f, 0.8f, 0.0f);  // dym sie unosi
    smokeParticles.additive   = false;

    stoneSystem.emitterPos = CRATER_POS;
    stoneSystem.gravity    = glm::vec3(0.0f, -9.81f, 0.0f);
}

void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
}

// Gaussowski "burst" eksplozji w cyklu - peak okolo phase=0.25
static float computeBurst(float phase) {
    float x = (phase - 0.25f) * 4.0f;
    return expf(-x * x);
}

void drawScene(GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    double now = glfwGetTime();
    float t  = (float)now;
    float dt = (float)(now - lastFrameTime);
    if (dt > 0.1f) dt = 0.1f; // zabezpieczenie pierwszej klatki / lagow
    lastFrameTime = now;

    // === FAZA ERUPCJI ===
    float phase = fmodf(t, eruptionCycleLen) / eruptionCycleLen;
    float burst = computeBurst(phase);

    // Wyrzut kamieni - przy przekroczeniu progu erupcji w gore (peak ok. phase=0.25)
    if (prevBurst < 0.7f && burst >= 0.7f) {
        stoneSystem.erupt(8, 7.0f, 11.0f, 0.5f, 0.20f, 0.40f, 4.5f);
    }
    prevBurst = burst;

    // Emisja czastek z akumulatorem (frakcyjne ilosci klatka po klatce)
    float lavaRate  = 40.0f + 350.0f * burst;   // czastki/sek
    float smokeRate = 30.0f + 90.0f  * burst;
    emitLavaAcc  += lavaRate  * dt;
    emitSmokeAcc += smokeRate * dt;
    int emitLava  = (int)emitLavaAcc;  emitLavaAcc  -= emitLava;
    int emitSmoke = (int)emitSmokeAcc; emitSmokeAcc -= emitSmoke;

    if (emitLava > 0) {
        lavaParticles.emit(
            emitLava,
            3.5f, 7.5f,          // predkosc
            0.55f,               // kat stozka
            glm::vec4(1.0f, 0.85f, 0.25f, 1.0f),  // start: jasnozolty
            glm::vec4(0.7f, 0.10f, 0.02f, 0.0f),  // koniec: ciemna czerwien zanika
            0.16f, 0.04f,
            1.6f
        );
    }
    if (emitSmoke > 0) {
        smokeParticles.emit(
            emitSmoke,
            1.0f, 2.2f,
            0.35f,
            glm::vec4(0.45f, 0.42f, 0.40f, 0.55f),
            glm::vec4(0.20f, 0.20f, 0.20f, 0.0f),
            0.40f, 1.20f,
            3.5f
        );
    }

    lavaParticles.update(dt);
    smokeParticles.update(dt);
    stoneSystem.update(dt);

    // === KAMERA ===
    float camX = cameraRadius * sinf(cameraAngleX) * sinf(cameraAngleY);
    float camY = cameraRadius * cosf(cameraAngleX);
    float camZ = cameraRadius * sinf(cameraAngleX) * cosf(cameraAngleY);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);

    glm::mat4 P = glm::perspective(glm::radians(50.0f), (float)w / h, 0.1f, 500.0f);
    glm::mat4 V = glm::lookAt(
        glm::vec3(camX, camY, camZ),
        glm::vec3(0.0f, 1.5f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // === SWIATLA (w przestrzeni widoku) ===
    glm::vec4 sunDir   = glm::normalize(V * glm::vec4(0.6f, 1.0f, 0.4f, 0.0f));
    glm::vec4 sunColor = glm::vec4(1.0f, 0.95f, 0.85f, 1.0f);

    glm::vec3 lavaView = glm::vec3(V * glm::vec4(CRATER_POS, 1.0f));

    // Intensywnosc lawy: tlo + drobny szum + duzy peak przy erupcji
    float flicker = 0.10f * sinf(t * 7.3f) + 0.05f * sinf(t * 17.1f);
    glm::vec4 lavaColor   = glm::vec4(1.0f, 0.45f, 0.10f, 1.0f);
    float lavaIntensity   = 0.9f + flicker + 3.5f * burst;

    auto setLightUniforms = [&](ShaderProgram* sp) {
        glUniform4fv(sp->u("sunDir"),       1, glm::value_ptr(sunDir));
        glUniform4fv(sp->u("sunColor"),     1, glm::value_ptr(sunColor));
        glUniform3fv(sp->u("lavaPos"),      1, glm::value_ptr(lavaView));
        glUniform4fv(sp->u("lavaColor"),    1, glm::value_ptr(lavaColor));
        glUniform1f (sp->u("lavaIntensity"), lavaIntensity);
    };

    // === SCENA - obiekty oteksturowane ===
    spLambertTextured->use();
    glUniformMatrix4fv(spLambertTextured->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spLambertTextured->u("V"), 1, false, glm::value_ptr(V));
    setLightUniforms(spLambertTextured);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(spLambertTextured->u("tex"), 0);

    // Teren
    glBindTexture(GL_TEXTURE_2D, texGround);
    glm::mat4 M = glm::mat4(1.0f);
    glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));
    Models::terrain.draw();

    // Wulkan
    glBindTexture(GL_TEXTURE_2D, texVolcano);
    M = glm::mat4(1.0f);
    M = glm::scale(M, glm::vec3(VOLCANO_SCALE));
    glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));
    volcanoModel.draw();

    // Spoczynkowe kamienie
    glBindTexture(GL_TEXTURE_2D, texRock);
    for (int i = 0; i < rockCount; i++) {
        const RockPlacement& r = rockPlacements[i];
        M = glm::mat4(1.0f);
        M = glm::translate(M, glm::vec3(r.x, 0.0f, r.z));
        M = glm::rotate   (M, r.rotY, glm::vec3(0.0f, 1.0f, 0.0f));
        M = glm::scale    (M, glm::vec3(r.scale));
        glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));
        rockModel.draw();
    }

    // Lecace kamienie - ta sama tekstura, ten sam shader
    stoneSystem.drawAll(spLambertTextured, rockModel);

    // === CZASTKI ===
    spParticle->use();
    glUniformMatrix4fv(spParticle->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spParticle->u("V"), 1, false, glm::value_ptr(V));
    glUniform1f(spParticle->u("pointScale"), (float)h * 1.2f);

    // Dym pierwszy (zwykly alpha-blending), potem lawa (additive na wierzchu)
    smokeParticles.draw();
    lavaParticles.draw();

    glfwSwapBuffers(window);
}

int main(void)
{
    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "Nie można zainicjować GLFW.\n");
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(800, 600, "Wulkan 3D", NULL, NULL);

    if (!window) {
        fprintf(stderr, "Nie można utworzyć okna.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Nie można zainicjować GLEW.\n");
        exit(EXIT_FAILURE);
    }

    initOpenGLProgram(window);

    glfwSetTime(0);
    lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        drawScene(window);
        glfwPollEvents();
    }

    freeOpenGLProgram(window);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
