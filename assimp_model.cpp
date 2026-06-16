#include "assimp_model.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdio.h>
#include <float.h>

AssimpModel::AssimpModel() : vertCount(0) {}

bool AssimpModel::load(const std::string& path, bool normalize) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_PreTransformVertices |
        aiProcess_JoinIdenticalVertices);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        fprintf(stderr, "Assimp blad: %s\n", importer.GetErrorString());
        return false;
    }

    vertices.clear();
    normals.clear();
    texCoords.clear();

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];

        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;

            for (unsigned int i = 0; i < 3; i++) {
                unsigned int idx = face.mIndices[i];

                aiVector3D v = mesh->mVertices[idx];
                vertices.push_back(v.x);
                vertices.push_back(v.y);
                vertices.push_back(v.z);
                vertices.push_back(1.0f);

                if (mesh->HasNormals()) {
                    aiVector3D n = mesh->mNormals[idx];
                    normals.push_back(n.x);
                    normals.push_back(n.y);
                    normals.push_back(n.z);
                    normals.push_back(0.0f);
                } else {
                    normals.push_back(0); normals.push_back(1);
                    normals.push_back(0); normals.push_back(0);
                }

                if (mesh->HasTextureCoords(0)) {
                    aiVector3D t = mesh->mTextureCoords[0][idx];
                    texCoords.push_back(t.x);
                    texCoords.push_back(t.y);
                } else {
                    texCoords.push_back(0); texCoords.push_back(0);
                }
            }
        }
    }

    vertCount = (int)vertices.size() / 4;

    // Wylicz bounding box
    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
    for (int i = 0; i < vertCount; i++) {
        float x = vertices[i*4 + 0];
        float y = vertices[i*4 + 1];
        float z = vertices[i*4 + 2];
        if (x < minX) minX = x; if (x > maxX) maxX = x;
        if (y < minY) minY = y; if (y > maxY) maxY = y;
        if (z < minZ) minZ = z; if (z > maxZ) maxZ = z;
    }

    printf("Zaladowano model %s\n", path.c_str());
    printf("  Wierzcholki: %d\n", vertCount);
    printf("  BBox min: (%.2f, %.2f, %.2f)\n", minX, minY, minZ);
    printf("  BBox max: (%.2f, %.2f, %.2f)\n", maxX, maxY, maxZ);
    printf("  Rozmiar: (%.2f, %.2f, %.2f)\n", maxX-minX, maxY-minY, maxZ-minZ);

    if (normalize && vertCount > 0) {
        float cx = (minX + maxX) * 0.5f;
        float cz = (minZ + maxZ) * 0.5f;
        float sx = maxX - minX;
        float sy = maxY - minY;
        float sz = maxZ - minZ;
        float maxDim = sx; if (sy > maxDim) maxDim = sy; if (sz > maxDim) maxDim = sz;
        float scale = (maxDim > 0.0f) ? (1.0f / maxDim) : 1.0f;

        // Centruj XZ, podstawa na Y=0, skaluj do max wymiar = 1
        for (int i = 0; i < vertCount; i++) {
            vertices[i*4 + 0] = (vertices[i*4 + 0] - cx) * scale;
            vertices[i*4 + 1] = (vertices[i*4 + 1] - minY) * scale;
            vertices[i*4 + 2] = (vertices[i*4 + 2] - cz) * scale;
        }
        printf("  Znormalizowano (skala %.4f)\n", scale);
    }

    return true;
}

void AssimpModel::draw() {
    if (vertCount == 0) return;

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
