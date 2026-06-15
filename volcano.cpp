#include "volcano.h"
#include <cmath>

static const float _PI = 3.14159265358979f;

namespace Models {

    Volcano volcano;

    void Volcano::addVertex(float x, float y, float z,
                            float nx, float ny, float nz,
                            float u, float v) {
        vertices.push_back(x);  vertices.push_back(y);
        vertices.push_back(z);  vertices.push_back(1.0f);
        normals.push_back(nx);  normals.push_back(ny);
        normals.push_back(nz);  normals.push_back(0.0f);
        texCoords.push_back(u); texCoords.push_back(v);
    }

    // Normalna plaskiej sciany (cross product dwoch krawedzi)
    static void faceNormal(
        float x0, float y0, float z0,
        float x1, float y1, float z1,
        float x2, float y2, float z2,
        float& nx, float& ny, float& nz)
    {
        float ax = x1 - x0, ay = y1 - y0, az = z1 - z0;
        float bx = x2 - x0, by = y2 - y0, bz = z2 - z0;
        nx = ay * bz - az * by;
        ny = az * bx - ax * bz;
        nz = ax * by - ay * bx;
        float len = sqrtf(nx*nx + ny*ny + nz*nz);
        if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }
    }

    void Volcano::build(float baseR, float cR, float H, int segs) {
        for (int i = 0; i < segs; i++) {
            float a0 = 2.0f * _PI * i / segs;
            float a1 = 2.0f * _PI * (i + 1) / segs;
            float u0 = (float)i / segs;
            float u1 = (float)(i + 1) / segs;

            float bx0 = baseR * cosf(a0), bz0 = baseR * sinf(a0);
            float bx1 = baseR * cosf(a1), bz1 = baseR * sinf(a1);
            float tx0 = cR * cosf(a0),    tz0 = cR * sinf(a0);
            float tx1 = cR * cosf(a1),    tz1 = cR * sinf(a1);

            // Normalna wspolna dla calej sciany (flat shading)
            float nx, ny, nz;
            faceNormal(bx0,0,bz0, bx1,0,bz1, tx1,H,tz1, nx,ny,nz);

            // Trojkat 1
            addVertex(bx0, 0, bz0, nx, ny, nz, u0, 0.0f);
            addVertex(bx1, 0, bz1, nx, ny, nz, u1, 0.0f);
            addVertex(tx1,  H, tz1, nx, ny, nz, u1, 1.0f);

            // Trojkat 2
            addVertex(bx0, 0, bz0, nx, ny, nz, u0, 0.0f);
            addVertex(tx1,  H, tz1, nx, ny, nz, u1, 1.0f);
            addVertex(tx0,  H, tz0, nx, ny, nz, u0, 1.0f);
        }

        // Dolna pokrywa
        for (int i = 0; i < segs; i++) {
            float a0 = 2.0f * _PI * i / segs;
            float a1 = 2.0f * _PI * (i + 1) / segs;
            float x0 = baseR * cosf(a0), z0 = baseR * sinf(a0);
            float x1 = baseR * cosf(a1), z1 = baseR * sinf(a1);

            addVertex(0,  0, 0,  0, -1, 0, 0.5f, 0.5f);
            addVertex(x1, 0, z1, 0, -1, 0, 0.5f + 0.5f * cosf(a1), 0.5f + 0.5f * sinf(a1));
            addVertex(x0, 0, z0, 0, -1, 0, 0.5f + 0.5f * cosf(a0), 0.5f + 0.5f * sinf(a0));
        }

        vertCount = (int)vertices.size() / 4;
    }

    Volcano::Volcano() {
        craterY = 3.5f;
        craterR = 0.6f;
        build(3.0f, craterR, craterY, 8);
    }

    void Volcano::draw() {
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
