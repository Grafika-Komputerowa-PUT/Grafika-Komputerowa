#include "terrain.h"
#include <string.h>

namespace Models {

    Terrain terrain;

    Terrain::Terrain() {
        float s = 10.0f; // polowa rozmiaru terenu
        float t = 6.0f;  // krotnosc kafli tekstury

        float verts[] = {
            -s, 0, -s, 1,
             s, 0, -s, 1,
             s, 0,  s, 1,
            -s, 0, -s, 1,
             s, 0,  s, 1,
            -s, 0,  s, 1,
        };
        float norms[] = {
            0,1,0,0,  0,1,0,0,  0,1,0,0,
            0,1,0,0,  0,1,0,0,  0,1,0,0,
        };
        float uvs[] = {
            0,0,  t,0,  t,t,
            0,0,  t,t,  0,t,
        };

        memcpy(vertices,  verts, sizeof(verts));
        memcpy(normals,   norms, sizeof(norms));
        memcpy(texCoords, uvs,   sizeof(uvs));
    }

    void Terrain::draw() {
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, normals);
        glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, texCoords);

        glDrawArrays(GL_TRIANGLES, 0, VERT_COUNT);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
}
