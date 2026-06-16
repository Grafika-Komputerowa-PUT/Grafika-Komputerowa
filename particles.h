#ifndef PARTICLES_H
#define PARTICLES_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// ===================================================================
// 1. KLASA BAZOWA CZĄSTEK (Wymóg zadania PARTICLES-01)
// ===================================================================
class ParticleSystem {
public:
    struct Particle {
        glm::vec3 pos;
        glm::vec3 vel;
        glm::vec4 colorStart;
        glm::vec4 colorEnd;
        float sizeStart;
        float sizeEnd;
        float age;
        float maxAge;
    };

    // Konfiguracja
    glm::vec3 emitterPos = glm::vec3(0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, -4.0f, 0.0f);
    bool      additive = true;
    GLuint    texture = 0;   // 0 = bez tekstury (soft circle), inaczej point sprite

    ParticleSystem();
    virtual ~ParticleSystem(); // Dodane słowo 'virtual' - wymagane w klasach bazowych

    // Emisja N czastek z parametrami w okreslonym stozku do gory
    void emit(int count,
        float speedMin, float speedMax,
        float coneAngleRad,
        glm::vec4 colorStart, glm::vec4 colorEnd,
        float sizeStart, float sizeEnd,
        float maxAge);

    void update(float dt);
    void draw();  // wymaga aktywnego spParticle z ustawionym P, V

    int alive() const { return (int)particles.size(); }

private:
    std::vector<Particle> particles;

    // VAO/VBO - pos(vec3), color(vec4), size(float) - osobne VBO
    GLuint vao = 0;
    GLuint vboPos = 0, vboCol = 0, vboSize = 0;
    int    capacity = 0;
    bool   initialized = false;

    // CPU staging
    std::vector<glm::vec3> stagePos;
    std::vector<glm::vec4> stageCol;
    std::vector<float>     stageSize;

    void initGL();
    void ensureCapacity(int n);
};

// ===================================================================
// 2. KLASY POCHODNE (Spełnienie warunku dziedziczenia)
// ===================================================================
class LavaParticleSystem : public ParticleSystem {
public:
    LavaParticleSystem() : ParticleSystem() {}
};

class SmokeParticleSystem : public ParticleSystem {
public:
    SmokeParticleSystem() : ParticleSystem() {}
};

#endif