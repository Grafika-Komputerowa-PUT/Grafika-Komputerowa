#ifndef ROCKS_H
#define ROCKS_H

#include <GL/glew.h>
#include <vector>

namespace Models {

    // Nieregularny osmioscian = "skala". Plaskie cieniowanie (normalna per sciana).
    class Rock {
    private:
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> texCoords;
        int vertCount;

        void addTriangle(float ax, float ay, float az,
                         float bx, float by, float bz,
                         float cx, float cy, float cz);
        void build();
    public:
        Rock();
        void draw();
    };

    extern Rock rock;
}

#endif
