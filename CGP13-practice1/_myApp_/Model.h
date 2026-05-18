#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <unordered_map>

#include <sb7.h>
#include <vmath.h>

#include <assimp/scene.h>

#include "Mesh.h"

constexpr int MAX_BONES = 100;

struct BoneInfo {
    int id = -1;
    vmath::mat4 offset{vmath::mat4::identity()};
};

struct AssimpNodeData {
    vmath::mat4 transformation{vmath::mat4::identity()};
    std::string name;
    int childrenCount = 0;
    std::vector<AssimpNodeData> children;
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;

    float shininess;
    vmath::vec3 defaultAmbient;
    vmath::vec3 defaultDiffuse;
    vmath::vec3 defaultSpecular;

private:
    bool useSpecularMap;

    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter;
    AssimpNodeData m_RootNode;
    vmath::mat4 m_GlobalInverseTransform;

public:
    Model();
    ~Model();

    bool loadModel(const std::string& path);
    void draw(GLuint shaderID);

    std::unordered_map<std::string, BoneInfo>& GetBoneInfoMap();
    int& GetBoneCount();
    const AssimpNodeData& GetRootNode() const;
    const vmath::mat4& GetGlobalInverseTransform() const;

private:
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    void setVertexBoneDataToDefault(Vertex& vertex);
    void setVertexBoneData(Vertex& vertex, int boneID, float weight);
    void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);

    void readHierarchyData(AssimpNodeData& dest, const aiNode* src);
    static vmath::mat4 ConvertMatrixToVMath(const aiMatrix4x4& from);
};

#endif
