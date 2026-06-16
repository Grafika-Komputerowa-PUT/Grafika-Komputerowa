#ifndef ASSIMP_MODEL_H
#define ASSIMP_MODEL_H

#include <GL/glew.h>
#include <vector>
#include <string>

class AssimpModel {
private:
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    int vertCount;

public:
    AssimpModel();
    // normalize=true: centruje model w (0,0,0) i skaluje tak by max wymiar = 1
    bool load(const std::string& path, bool normalize = true);
    void draw();
    int getVertexCount() const { return vertCount; }
};

#endif
