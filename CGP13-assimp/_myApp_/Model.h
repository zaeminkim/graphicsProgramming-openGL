#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <map>
#include <sb7.h>
#include <vmath.h>
#include <glm/glm.hpp>

#include <assimp/scene.h>

#include "Mesh.h"
#include "Texture.h"

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};

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

    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

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

    std::map<std::string, BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }

private:
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    void SetVertexBoneDataToDefault(Vertex& vertex);
    void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
    void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
};

#endif
