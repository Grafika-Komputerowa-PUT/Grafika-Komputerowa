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
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "assimp_model.h"

// Stan kamery orbitalnej (współrzędne sferyczne)
float cameraAngleX = 0.8f;  // kąt pionowy (pitch): 0=góra, PI=dół
float cameraAngleY = 0.0f;  // kąt poziomy (yaw)
float cameraRadius = 12.0f; // odległość od centrum sceny

// Stan myszy
bool mousePressed = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

// Tekstury
GLuint texGround   = 0;
GLuint texVolcano  = 0;

// Modele wczytywane przez Assimp
AssimpModel volcanoModel;

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
    if (cameraRadius < 2.0f)  cameraRadius = 2.0f;
    if (cameraRadius > 30.0f) cameraRadius = 30.0f;
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

    volcanoModel.load("volcano/source/Volcano_Lowpoly.fbx");
}

void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
}

void drawScene(GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Pozycja kamery ze współrzędnych sferycznych
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
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Kierunek slonca w przestrzeni widoku (staly w przestrzeni swiata)
    glm::vec4 sunDir = glm::normalize(V * glm::vec4(1.0f, 2.0f, 1.0f, 0.0f));

    // --- Teren ---
    spLambertTextured->use();
    glUniformMatrix4fv(spLambertTextured->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spLambertTextured->u("V"), 1, false, glm::value_ptr(V));
    glUniform4fv(spLambertTextured->u("lightDir"), 1, glm::value_ptr(sunDir));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texGround);
    glUniform1i(spLambertTextured->u("tex"), 0);

    glm::mat4 M = glm::mat4(1.0f);
    glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));
    Models::terrain.draw();

    // --- Wulkan (model FBX z Assimp) - DIAGNOSTYKA: jaskrawy kolor bez tekstury ---
    spLambert->use();
    glUniformMatrix4fv(spLambert->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spLambert->u("V"), 1, false, glm::value_ptr(V));
    glUniform4fv(spLambert->u("lightDir"), 1, glm::value_ptr(sunDir));
    M = glm::mat4(1.0f);
    M = glm::scale(M, glm::vec3(5.0f)); // skala po normalizacji - max wymiar 5 jednostek
    glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M));
    glUniform4f(spLambert->u("color"), 1.0f, 0.0f, 1.0f, 1.0f); // magenta - jaskrawe, latwo zauwazyc
    volcanoModel.draw();

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
    while (!glfwWindowShouldClose(window)) {
        glfwSetTime(0);
        drawScene(window);
        glfwPollEvents();
    }

    freeOpenGLProgram(window);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
