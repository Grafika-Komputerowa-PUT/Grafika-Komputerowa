#ifndef VOLCANO_H
#define VOLCANO_H

#include <GL/glew.h>
#include <vector>

namespace Models {

    class Volcano {
    private:
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> texCoords;
        int vertCount;

        void addVertex(float x, float y, float z,
                       float nx, float ny, float nz,
                       float u, float v);
        void build(float baseR, float craterR, float H, int segs);

    public:
        Volcano();
        void draw();
        float craterY;   // wysokosc krateru (do pozycjonowania efektow)
        float craterR;   // promien krateru
    };

    extern Volcano volcano;
}

#endif
