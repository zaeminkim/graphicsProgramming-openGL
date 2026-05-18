#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <sb7.h>
#include <vmath.h>

#include <assimp/scene.h>

#include "Mesh.h"
#include "Texture.h"

class Model
{
public:
    std::vector<Mesh> meshes;
    std::string directory;

    GLuint diffuseMap;
    GLuint specularMap;

    float shininess;

    vmath::vec3 defaultAmbient;
    vmath::vec3 defaultDiffuse;
    vmath::vec3 defaultSpecular;

private:
    bool useDiffuseMap;
    bool useSpecularMap;

public:
    Model();
    ~Model();

    void init();

    bool loadModel(const std::string& path);

    void setupMesh(
        int numVertices,
        GLfloat* positions,
        GLfloat* texCoords = nullptr,
        GLfloat* normals = nullptr
    );

    void setupIndices(int numIndices, GLuint* indices);

    bool loadDiffuseMap(const char* filepath);
    bool loadSpecularMap(const char* filepath);

    void draw(GLuint shaderID);

private:
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};

#endif