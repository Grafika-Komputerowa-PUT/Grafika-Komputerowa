#ifndef STONES_H
#define STONES_H

#include <glm/glm.hpp>
#include <vector>
#include "assimp_model.h"
#include "shaderprogram.h"

class StoneSystem {
public:
    struct Stone {
        glm::vec3 pos;
        glm::vec3 vel;
        glm::vec3 spinAxis;
        float     angle;
        float     angularVel;
        float     scale;
        float     life;     // <=0 = martwy (nie rysowac)
        float     maxLife;
    };

    glm::vec3 emitterPos = glm::vec3(0.0f);
    glm::vec3 gravity    = glm::vec3(0.0f, -9.81f, 0.0f);

    // Wyrzuc N kamieni w gore z lekkim rozrzutem (przypisuje sloty do umarlych)
    void erupt(int count, float speedMin, float speedMax, float coneAngleRad,
               float scaleMin, float scaleMax, float maxLife);

    void update(float dt);

    // Rysuje wszystkie zywe kamienie z aktualna macierza M ustawiana per kamien.
    // Wymaga aktywnego programu cieniujacego (spLambertTextured), zwiazanej tekstury
    // i ustawionych juz uniformow swiatla / P / V.
    void drawAll(ShaderProgram* sp, AssimpModel& model);

    int alive() const;

private:
    std::vector<Stone> stones;
};

#endif
