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

// === STAN KAMERY ===
const float CAM_PITCH_DEFAULT  = 0.8f;
const float CAM_YAW_DEFAULT    = 0.0f;
const float CAM_RADIUS_DEFAULT = 14.0f;

float cameraAngleX = CAM_PITCH_DEFAULT;
float cameraAngleY = CAM_YAW_DEFAULT;
float cameraRadius = CAM_RADIUS_DEFAULT;

bool mousePressed = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

// === STAN SYMULACJI ===
double realPrevTime = 0.0;
double simTime      = 0.0;
bool   paused       = false;

// === TEKSTURY ===
GLuint texGround   = 0;
GLuint texVolcano  = 0;
GLuint texRock     = 0;
GLuint texEmissive = 0;

// === MODELE ===
AssimpModel volcanoModel;
AssimpModel rockModel;

const float VOLCANO_SCALE = 5.0f;
const glm::vec3 CRATER_POS = glm::vec3(0.0f, VOLCANO_SCALE * 0.95f, 0.0f);

// === SYSTEMY ERUPCJI ===
ParticleSystem lavaParticles;
ParticleSystem smokeParticles;
StoneSystem    stoneSystem;

float eruptionCycleLen = 4.5f;
float prevBurst    = 0.0f;
float prevPhase    = 0.0f;
float emitLavaAcc  = 0.0f;
float emitSmokeAcc = 0.0f;
float eruptionMagnitude = 1.0f;   // losowane per cykl: 0.6..1.4

// === PRESETY KAMERY ===
struct CamPreset { float pitch, yaw, radius; };
static const CamPreset camPresets[] = {
    { 0.85f, 0.0f,  14.0f },   // 1: overview - dalej, lekko z gory
    { 1.20f, 0.6f,   7.0f },   // 2: blisko, niski kat, prawie zbocza
    { 0.30f, 0.0f,  10.0f }    // 3: prawie z gory na krater
};
static const int camPresetCount = sizeof(camPresets) / sizeof(camPresets[0]);

// === KAMIENIE SPOCZYNKOWE ===
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

// === GEOMETRIE WBUDOWANE ===
// Pelno-ekranowy quad w NDC dla nieba
static const float skyQuad[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
    -1.0f,  1.0f
};

// Poziomy quad dla lawy w kraterze
static const float lavaQuadPos[] = {
    -1.0f, 0.0f, -1.0f, 1.0f,
     1.0f, 0.0f, -1.0f, 1.0f,
     1.0f, 0.0f,  1.0f, 1.0f,
    -1.0f, 0.0f, -1.0f, 1.0f,
     1.0f, 0.0f,  1.0f, 1.0f,
    -1.0f, 0.0f,  1.0f, 1.0f
};
static const float lavaQuadUV[] = {
    0,0,  1,0,  1,1,
    0,0,  1,1,  0,1
};

// ===================================================================

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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_R) {
        cameraAngleX = CAM_PITCH_DEFAULT;
        cameraAngleY = CAM_YAW_DEFAULT;
        cameraRadius = CAM_RADIUS_DEFAULT;
    } else if (key == GLFW_KEY_SPACE) {
        paused = !paused;
    } else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_1 + camPresetCount - 1) {
        const CamPreset& cp = camPresets[key - GLFW_KEY_1];
        cameraAngleX = cp.pitch;
        cameraAngleY = cp.yaw;
        cameraRadius = cp.radius;
    }
}

void pollCameraKeys(GLFWwindow* window, float dt) {
    float rotSpeed = 1.6f * dt;
    float zoomSpeed = 6.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraAngleY -= rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraAngleY += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraAngleX -= rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraAngleX += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)      cameraRadius -= zoomSpeed;
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) cameraRadius += zoomSpeed;
    if (cameraAngleX < 0.05f)       cameraAngleX = 0.05f;
    if (cameraAngleX > PI - 0.05f)  cameraAngleX = PI - 0.05f;
    if (cameraRadius < 3.0f)  cameraRadius = 3.0f;
    if (cameraRadius > 40.0f) cameraRadius = 40.0f;
}

void initOpenGLProgram(GLFWwindow* window) {
    initShaders();
    glEnable(GL_DEPTH_TEST);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback  (window, cursor_position_callback);
    glfwSetScrollCallback     (window, scroll_callback);
    glfwSetKeyCallback        (window, key_callback);

    texGround   = loadTexture("bricks_diffuse.png");
    texVolcano  = loadTexture("volcano/textures/Volcano_AOAmbient_Occlusion.png");
    texRock     = loadTexture("renders/rock-low-polygon/textures/DefaultMaterial_Base_color.png");
    texEmissive = loadTexture("volcano/textures/emissive_01.png");

    volcanoModel.load("volcano/source/Volcano_Lowpoly.fbx");
    rockModel.load   ("renders/rock-low-polygon/source/Rock/Rock.fbx");

    lavaParticles.emitterPos = CRATER_POS;
    lavaParticles.gravity    = glm::vec3(0.0f, -7.5f, 0.0f);
    lavaParticles.additive   = true;
    lavaParticles.texture    = texEmissive;  // glowing sprite

    smokeParticles.gravity   = glm::vec3(0.0f, 0.8f, 0.0f);
    smokeParticles.additive  = false;
    smokeParticles.texture   = 0;            // soft circle

    stoneSystem.emitterPos = CRATER_POS;
    stoneSystem.gravity    = glm::vec3(0.0f, -9.81f, 0.0f);
}

void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
}

static float computeBurst(float phase) {
    float x = (phase - 0.25f) * 4.0f;
    return expf(-x * x);
}

static void drawSky() {
    spSky->use();
    glDisable(GL_DEPTH_TEST);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, skyQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);
}

static void drawCraterLava(const glm::mat4& P, const glm::mat4& V, float burst, float t) {
    spEmissive->use();
    glUniformMatrix4fv(spEmissive->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spEmissive->u("V"), 1, false, glm::value_ptr(V));

    // Powierzchnia lawy lekko ponad gornym wlotem krateru, skala pulsuje z erupcja
    float baseSize = 0.55f;
    float size = baseSize + 0.25f * burst + 0.05f * sinf(t * 6.1f);
    glm::mat4 M(1.0f);
    M = glm::translate(M, CRATER_POS + glm::vec3(0.0f, 0.05f, 0.0f));
    M = glm::scale    (M, glm::vec3(size, 1.0f, size));
    glUniformMatrix4fv(spEmissive->u("M"), 1, false, glm::value_ptr(M));

    glm::vec3 tint = glm::vec3(1.0f, 0.55f + 0.25f * burst, 0.18f);
    float intensity = 1.2f + 2.5f * burst + 0.15f * sinf(t * 9.7f);
    glUniform3fv(spEmissive->u("tint"),      1, glm::value_ptr(tint));
    glUniform1f (spEmissive->u("intensity"), intensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texEmissive);
    glUniform1i(spEmissive->u("tex"), 0);

    // Additive - czarne tlo zniknie, swieci tylko jadro
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, lavaQuadPos);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, lavaQuadUV);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void drawScene(GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    double now = glfwGetTime();
    float realDt = (float)(now - realPrevTime);
    realPrevTime = now;
    if (realDt > 0.1f) realDt = 0.1f;

    float simDt = paused ? 0.0f : realDt;
    simTime += simDt;
    float t = (float)simTime;

    pollCameraKeys(window, realDt);

    // === FAZA ERUPCJI ===
    float phase = fmodf(t, eruptionCycleLen) / eruptionCycleLen;

    // Nowy cykl - losowa intensywnosc erupcji
    if (phase < prevPhase) {
        eruptionMagnitude = 0.6f + 0.8f * ((float)rand() / (float)RAND_MAX);
    }
    prevPhase = phase;

    float burst = computeBurst(phase) * eruptionMagnitude;

    if (prevBurst < 0.7f && burst >= 0.7f) {
        int stoneCount = (int)(6 + 6 * eruptionMagnitude);
        float stoneSpeedMax = 9.0f + 3.0f * eruptionMagnitude;
        stoneSystem.erupt(stoneCount, 6.0f, stoneSpeedMax, 0.5f, 0.20f, 0.40f, 4.5f);
    }
    prevBurst = burst;

    // Akumulator emisji - zalezny od burst (ktory juz jest pomnozony przez magnitude)
    float lavaRate  = 40.0f + 350.0f * burst;
    float smokeRate = 30.0f + 90.0f  * burst;
    emitLavaAcc  += lavaRate  * simDt;
    emitSmokeAcc += smokeRate * simDt;
    int emitLava  = (int)emitLavaAcc;  emitLavaAcc  -= emitLava;
    int emitSmoke = (int)emitSmokeAcc; emitSmokeAcc -= emitSmoke;

    if (emitLava > 0) {
        lavaParticles.emitterPos = CRATER_POS;
        lavaParticles.emit(
            emitLava,
            3.5f, 7.5f,
            0.55f,
            glm::vec4(1.0f, 0.85f, 0.25f, 1.0f),
            glm::vec4(0.7f, 0.10f, 0.02f, 0.0f),
            0.35f, 0.10f,   // wieksze - tekstura zniwsi sie z odlegloscia, dym musi rozmnozyc
            1.6f
        );
    }
    if (emitSmoke > 0) {
        // Lekkie kolysanie miejsca emisji dymu - sin/cos w czasie
        smokeParticles.emitterPos = CRATER_POS + glm::vec3(
            0.35f * sinf(t * 0.4f),
            0.30f,
            0.35f * cosf(t * 0.3f)
        );
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

    lavaParticles.update(simDt);
    smokeParticles.update(simDt);
    stoneSystem.update(simDt);

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

    // === NIEBO ===
    drawSky();

    // === SWIATLA ===
    glm::vec4 sunDir   = glm::normalize(V * glm::vec4(0.6f, 1.0f, 0.4f, 0.0f));
    glm::vec4 sunColor = glm::vec4(1.0f, 0.95f, 0.85f, 1.0f);

    glm::vec3 lavaView = glm::vec3(V * glm::vec4(CRATER_POS, 1.0f));

    float flicker = 0.10f * sinf(t * 7.3f) + 0.05f * sinf(t * 17.1f);
    glm::vec4 lavaColor = glm::vec4(1.0f, 0.45f, 0.10f, 1.0f);
    float lavaIntensity = 0.9f + flicker + 3.5f * burst;

    auto setLightUniforms = [&](ShaderProgram* sp) {
        glUniform4fv(sp->u("sunDir"),       1, glm::value_ptr(sunDir));
        glUniform4fv(sp->u("sunColor"),     1, glm::value_ptr(sunColor));
        glUniform3fv(sp->u("lavaPos"),      1, glm::value_ptr(lavaView));
        glUniform4fv(sp->u("lavaColor"),    1, glm::value_ptr(lavaColor));
        glUniform1f (sp->u("lavaIntensity"), lavaIntensity);
    };

    // === SCENA ===
    spLambertTextured->use();
    glUniformMatrix4fv(spLambertTextured->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spLambertTextured->u("V"), 1, false, glm::value_ptr(V));
    setLightUniforms(spLambertTextured);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(spLambertTextured->u("tex"), 0);

    glBindTexture(GL_TEXTURE_2D, texGround);
    glm::mat4 M(1.0f);
    glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));
    Models::terrain.draw();

    glBindTexture(GL_TEXTURE_2D, texVolcano);
    M = glm::mat4(1.0f);
    M = glm::scale(M, glm::vec3(VOLCANO_SCALE));
    glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));
    volcanoModel.draw();

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

    stoneSystem.drawAll(spLambertTextured, rockModel);

    // === LAWA W KRATERZE (emissive, additive) ===
    drawCraterLava(P, V, burst, t);

    // === CZASTKI ===
    spParticle->use();
    glUniformMatrix4fv(spParticle->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spParticle->u("V"), 1, false, glm::value_ptr(V));
    glUniform1f(spParticle->u("pointScale"), (float)h * 1.2f);

    smokeParticles.draw();
    lavaParticles.draw();

    glfwSwapBuffers(window);
}

int main(void) {
    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "Nie można zainicjować GLFW.\n");
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(800, 600, "Wulkan 3D  [Space=pauza  R=reset  1/2/3=ujecia  WSAD/strzalki=kamera  +/-=zoom  Esc=wyjscie]", NULL, NULL);

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
    realPrevTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        drawScene(window);
        glfwPollEvents();
    }

    freeOpenGLProgram(window);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
