#include "rocks.h"
#include <cmath>

namespace Models {

    Rock rock;

    static void normalize3(float& x, float& y, float& z) {
        float l = sqrtf(x*x + y*y + z*z);
        if (l > 0.0f) { x /= l; y /= l; z /= l; }
    }

    void Rock::addTriangle(float ax, float ay, float az,
                           float bx, float by, float bz,
                           float cx, float cy, float cz) {
        // Normalna sciany (cross product) - flat shading
        float ux = bx - ax, uy = by - ay, uz = bz - az;
        float vx = cx - ax, vy = cy - ay, vz = cz - az;
        float nx = uy*vz - uz*vy;
        float ny = uz*vx - ux*vz;
        float nz = ux*vy - uy*vx;
        normalize3(nx, ny, nz);

        float pos[3][3] = { {ax,ay,az}, {bx,by,bz}, {cx,cy,cz} };
        float uvs[3][2] = { {0.0f,0.0f}, {1.0f,0.0f}, {0.5f,1.0f} };

        for (int i = 0; i < 3; i++) {
            vertices.push_back(pos[i][0]);
            vertices.push_back(pos[i][1]);
            vertices.push_back(pos[i][2]);
            vertices.push_back(1.0f);
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
            normals.push_back(0.0f);
            texCoords.push_back(uvs[i][0]);
            texCoords.push_back(uvs[i][1]);
        }
    }

    void Rock::build() {
        // 6 wierzcholkow nieregularnego osmiosciau - perturbacja dla "skalistego" wygladu
        float v[6][3] = {
            {  1.10f,  0.05f,  0.10f}, // 0: +X
            { -1.00f, -0.05f, -0.10f}, // 1: -X
            {  0.05f,  0.90f, -0.05f}, // 2: +Y
            {  0.00f, -0.80f,  0.10f}, // 3: -Y
            {  0.00f,  0.05f,  1.00f}, // 4: +Z
            {  0.10f, -0.05f, -1.10f}  // 5: -Z
        };
        // 8 scian - kazda CCW patrzac z zewnatrz (normalna wyjdzie na zewnatrz)
        int faces[8][3] = {
            {0, 2, 4}, {4, 2, 1}, {1, 2, 5}, {5, 2, 0},  // gorne 4
            {4, 3, 0}, {1, 3, 4}, {5, 3, 1}, {0, 3, 5}   // dolne 4
        };
        for (int i = 0; i < 8; i++) {
            int a = faces[i][0], b = faces[i][1], c = faces[i][2];
            addTriangle(v[a][0], v[a][1], v[a][2],
                        v[b][0], v[b][1], v[b][2],
                        v[c][0], v[c][1], v[c][2]);
        }
        vertCount = (int)vertices.size() / 4;
    }

    Rock::Rock() : vertCount(0) {
        build();
    }

    void Rock::draw() {
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, vertices.data());
        glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, normals.data());
        glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, texCoords.data());

        glDrawArrays(GL_TRIANGLES, 0, vertCount);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
}
