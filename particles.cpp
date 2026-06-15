#include "particles.h"
#include <cstdlib>
#include <cmath>

static float frand() {
    return (float)rand() / (float)RAND_MAX;
}
static float frandRange(float a, float b) {
    return a + (b - a) * frand();
}

ParticleSystem::ParticleSystem() {}

ParticleSystem::~ParticleSystem() {
    if (initialized) {
        glDeleteBuffers(1, &vboPos);
        glDeleteBuffers(1, &vboCol);
        glDeleteBuffers(1, &vboSize);
        glDeleteVertexArrays(1, &vao);
    }
}

void ParticleSystem::initGL() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vboPos);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, (void*)0);

    glGenBuffers(1, &vboCol);
    glBindBuffer(GL_ARRAY_BUFFER, vboCol);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, (void*)0);

    glGenBuffers(1, &vboSize);
    glBindBuffer(GL_ARRAY_BUFFER, vboSize);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, false, 0, (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    initialized = true;
}

void ParticleSystem::ensureCapacity(int n) {
    if (n <= capacity) return;
    capacity = n + 64;
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glBufferData(GL_ARRAY_BUFFER, capacity * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vboCol);
    glBufferData(GL_ARRAY_BUFFER, capacity * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vboSize);
    glBufferData(GL_ARRAY_BUFFER, capacity * sizeof(float),     nullptr, GL_DYNAMIC_DRAW);
}

void ParticleSystem::emit(int count,
                          float speedMin, float speedMax,
                          float coneAngleRad,
                          glm::vec4 colorStart, glm::vec4 colorEnd,
                          float sizeStart, float sizeEnd,
                          float maxAge) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = emitterPos;

        // Predkosc w stozku skierowanym do gory (+Y)
        float theta = coneAngleRad * frand();          // odchylenie od osi Y
        float phi   = 2.0f * 3.14159265f * frand();    // kat azymutalny
        float speed = frandRange(speedMin, speedMax);

        float sinT = sinf(theta), cosT = cosf(theta);
        p.vel = glm::vec3(sinT * cosf(phi), cosT, sinT * sinf(phi)) * speed;

        p.colorStart = colorStart;
        p.colorEnd   = colorEnd;
        p.sizeStart  = sizeStart;
        p.sizeEnd    = sizeEnd;
        p.age        = 0.0f;
        p.maxAge     = maxAge * frandRange(0.7f, 1.1f);

        particles.push_back(p);
    }
}

void ParticleSystem::update(float dt) {
    for (size_t i = 0; i < particles.size(); ) {
        Particle& p = particles[i];
        p.age += dt;
        if (p.age >= p.maxAge) {
            particles[i] = particles.back();
            particles.pop_back();
        } else {
            p.vel += gravity * dt;
            p.pos += p.vel * dt;
            // proste odbicie od ziemi z tlumieniem
            if (p.pos.y < 0.0f && p.vel.y < 0.0f) {
                p.pos.y = 0.0f;
                p.vel.y = -p.vel.y * 0.25f;
                p.vel.x *= 0.6f;
                p.vel.z *= 0.6f;
            }
            ++i;
        }
    }
}

void ParticleSystem::draw() {
    if (!initialized) initGL();
    int n = (int)particles.size();
    if (n == 0) return;

    // Interpolacja kolorow/rozmiarow wedlug wieku
    stagePos.resize(n);
    stageCol.resize(n);
    stageSize.resize(n);
    for (int i = 0; i < n; i++) {
        const Particle& p = particles[i];
        float t = p.age / p.maxAge;
        if (t > 1.0f) t = 1.0f;
        stagePos[i]  = p.pos;
        stageCol[i]  = p.colorStart * (1.0f - t) + p.colorEnd * t;
        stageSize[i] = p.sizeStart  * (1.0f - t) + p.sizeEnd  * t;
    }

    ensureCapacity(n);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glBufferSubData(GL_ARRAY_BUFFER, 0, n * sizeof(glm::vec3), stagePos.data());
    glBindBuffer(GL_ARRAY_BUFFER, vboCol);
    glBufferSubData(GL_ARRAY_BUFFER, 0, n * sizeof(glm::vec4), stageCol.data());
    glBindBuffer(GL_ARRAY_BUFFER, vboSize);
    glBufferSubData(GL_ARRAY_BUFFER, 0, n * sizeof(float),     stageSize.data());

    // Stan render: bez writu glebi (zeby czastki nie kasowaly sie wzajemnie), blending wedlug trybu
    GLboolean prevDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glEnable(GL_PROGRAM_POINT_SIZE);
    if (additive) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_POINTS, 0, n);

    glDisable(GL_BLEND);
    glDepthMask(prevDepthMask);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
