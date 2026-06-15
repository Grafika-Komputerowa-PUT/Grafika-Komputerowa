#ifndef TERRAIN_H
#define TERRAIN_H

#include <GL/glew.h>

namespace Models {

    class Terrain {
    private:
        static const int VERT_COUNT = 6;
        float vertices[VERT_COUNT * 4];
        float normals[VERT_COUNT * 4];
        float texCoords[VERT_COUNT * 2];
    public:
        Terrain();
        void draw();
    };

    extern Terrain terrain;
}

#endif
