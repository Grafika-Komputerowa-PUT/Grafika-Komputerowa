#include "stones.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <cmath>

static float frand() {
    return (float)rand() / (float)RAND_MAX;
}
static float frandRange(float a, float b) {
    return a + (b - a) * frand();
}

void StoneSystem::erupt(int count, float speedMin, float speedMax, float coneAngleRad,
                        float scaleMin, float scaleMax, float maxLife) {
    for (int n = 0; n < count; n++) {
        Stone s;
        s.pos = emitterPos;

        float theta = coneAngleRad * frand();
        float phi   = 2.0f * 3.14159265f * frand();
        float sp    = frandRange(speedMin, speedMax);
        float sinT  = sinf(theta), cosT = cosf(theta);
        s.vel       = glm::vec3(sinT * cosf(phi), cosT, sinT * sinf(phi)) * sp;

        s.spinAxis = glm::normalize(glm::vec3(
            frandRange(-1.0f, 1.0f),
            frandRange(-1.0f, 1.0f),
            frandRange(-1.0f, 1.0f)
        ));
        s.angle      = frandRange(0.0f, 6.28f);
        s.angularVel = frandRange(-4.0f, 4.0f);
        s.scale      = frandRange(scaleMin, scaleMax);
        s.maxLife    = maxLife * frandRange(0.8f, 1.2f);
        s.life       = s.maxLife;

        stones.push_back(s);
    }
}

void StoneSystem::update(float dt) {
    for (size_t i = 0; i < stones.size(); ) {
        Stone& s = stones[i];
        s.life -= dt;
        if (s.life <= 0.0f) {
            stones[i] = stones.back();
            stones.pop_back();
            continue;
        }
        s.vel += gravity * dt;
        s.pos += s.vel * dt;
        s.angle += s.angularVel * dt;

        //float groundLevel = -1.0f + (s.scale * 0.4f);
        // Odbicie od ziemi z tlumieniem
        if (s.pos.y < s.scale * 0.4f && s.vel.y < 0.0f) {
            s.pos.y = s.scale * 0.4f;
            s.vel.y = -s.vel.y * 0.35f;
            s.vel.x *= 0.55f;
            s.vel.z *= 0.55f;
            s.angularVel *= 0.6f;
            if (fabsf(s.vel.y) < 0.5f) {
                // wyhamowal - skroc zycie
                s.life = fminf(s.life, 0.5f);
            }
        }
        ++i;
    }
}

void StoneSystem::drawAll(ShaderProgram* sp, AssimpModel& model) {
    for (size_t i = 0; i < stones.size(); i++) {
        const Stone& s = stones[i];
        glm::mat4 M(1.0f);
        M = glm::translate(M, s.pos);
        M = glm::rotate   (M, s.angle, s.spinAxis);
        M = glm::scale    (M, glm::vec3(s.scale));
        glUniformMatrix4fv(sp->u("uModel"), 1, false, glm::value_ptr(M));
        model.draw();
    }
}

int StoneSystem::alive() const {
    return (int)stones.size();
}
