#include "Model.h"

#include <fstream>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Texture.h"

namespace {
std::string resolveTexturePath(const std::string& directory, const std::string& textureFile) {
    const std::string directPath = directory + "/" + textureFile;
    std::ifstream directFile(directPath, std::ios::binary);
    if (directFile.good()) {
        return directPath;
    }

    const std::string texturesSubdirPath = directory + "/textures/" + textureFile;
    std::ifstream texturesSubdirFile(texturesSubdirPath, std::ios::binary);
    if (texturesSubdirFile.good()) {
        return texturesSubdirPath;
    }

    return directPath;
}
}

vmath::mat4 Model::ConvertMatrixToVMath(const aiMatrix4x4& from) {
    vmath::mat4 out;
    out[0] = vmath::vec4(from.a1, from.b1, from.c1, from.d1);
    out[1] = vmath::vec4(from.a2, from.b2, from.c2, from.d2);
    out[2] = vmath::vec4(from.a3, from.b3, from.c3, from.d3);
    out[3] = vmath::vec4(from.a4, from.b4, from.c4, from.d4);
    return out;
}

Model::Model()
    : shininess(32.0f),
      defaultAmbient(1.0f, 1.0f, 1.0f),
      defaultDiffuse(1.0f, 1.0f, 1.0f),
      defaultSpecular(0.0f, 0.0f, 0.0f),
      useSpecularMap(false),
      m_BoneCounter(0),
      m_GlobalInverseTransform(vmath::mat4::identity()) {
}

Model::~Model() {
}

bool Model::loadModel(const std::string& path) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }

    meshes.clear();
    m_BoneInfoMap.clear();
    m_BoneCounter = 0;

    size_t slashPos = path.find_last_of("\\/");
    directory = (slashPos == std::string::npos) ? "." : path.substr(0, slashPos);

    aiMatrix4x4 globalTransform = scene->mRootNode->mTransformation;
    globalTransform.Inverse();
    m_GlobalInverseTransform = ConvertMatrixToVMath(globalTransform);

    readHierarchyData(m_RootNode, scene->mRootNode);
    processNode(scene->mRootNode, scene);

    return true;
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    vertices.resize(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;
        setVertexBoneDataToDefault(vertex);

        vertex.Position = vmath::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals()) {
            vertex.Normal = vmath::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = vmath::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        } else {
            vertex.TexCoords = vmath::vec2(0.0f, 0.0f);
        }

        vertices[i] = vertex;
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    extractBoneWeightForVertices(vertices, mesh, scene);

    GLuint diffuseTexture = 0;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texturePath;

        bool foundTexture = false;
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS) {
            foundTexture = true;
        } else if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            foundTexture = true;
        }

        if (foundTexture) {
            std::string textureFile = texturePath.C_Str();
            std::string fullPath = resolveTexturePath(directory, textureFile);

            glGenTextures(1, &diffuseTexture);
            if (!loadTextureFile(diffuseTexture, fullPath.c_str())) {
                std::cout << "Texture load failed: " << fullPath << std::endl;
                glDeleteTextures(1, &diffuseTexture);
                diffuseTexture = 0;
            }
        }
    }

    return Mesh(vertices, indices, diffuseTexture);
}

void Model::setVertexBoneDataToDefault(Vertex& vertex) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        vertex.BoneIDs[i] = -1;
        vertex.Weights[i] = 0.0f;
    }
}

void Model::setVertexBoneData(Vertex& vertex, int boneID, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        if (vertex.BoneIDs[i] < 0) {
            vertex.BoneIDs[i] = boneID;
            vertex.Weights[i] = weight;
            return;
        }
    }
}

void Model::extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene) {
    (void)scene;

    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        auto it = m_BoneInfoMap.find(boneName);
        if (it == m_BoneInfoMap.end()) {
            if (m_BoneCounter >= MAX_BONES) {
                continue;
            }

            BoneInfo newBoneInfo;
            newBoneInfo.id = m_BoneCounter;
            newBoneInfo.offset = ConvertMatrixToVMath(mesh->mBones[boneIndex]->mOffsetMatrix);
            m_BoneInfoMap[boneName] = newBoneInfo;
            boneID = m_BoneCounter;
            ++m_BoneCounter;
        } else {
            boneID = it->second.id;
        }

        const aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
        const unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (unsigned int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
            const int vertexId = static_cast<int>(weights[weightIndex].mVertexId);
            const float weight = weights[weightIndex].mWeight;

            if (vertexId >= 0 && vertexId < static_cast<int>(vertices.size())) {
                setVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }
}

void Model::readHierarchyData(AssimpNodeData& dest, const aiNode* src) {
    dest.name = src->mName.C_Str();
    dest.transformation = ConvertMatrixToVMath(src->mTransformation);
    dest.childrenCount = static_cast<int>(src->mNumChildren);
    dest.children.clear();
    dest.children.reserve(src->mNumChildren);

    for (unsigned int i = 0; i < src->mNumChildren; ++i) {
        AssimpNodeData child;
        readHierarchyData(child, src->mChildren[i]);
        dest.children.push_back(child);
    }
}

void Model::draw(GLuint shaderID) {
    glUniform3fv(glGetUniformLocation(shaderID, "material.defaultAmbient"), 1, defaultAmbient);
    glUniform3fv(glGetUniformLocation(shaderID, "material.defaultDiffuse"), 1, defaultDiffuse);
    glUniform3fv(glGetUniformLocation(shaderID, "material.defaultSpecular"), 1, defaultSpecular);

    glUniform1i(glGetUniformLocation(shaderID, "material.useSpecularMap"), static_cast<int>(useSpecularMap));
    glUniform1i(glGetUniformLocation(shaderID, "useNormal"), 1);
    glUniform1f(glGetUniformLocation(shaderID, "material.shininess"), shininess);

    for (auto& mesh : meshes) {
        mesh.draw(shaderID);
    }
}

std::unordered_map<std::string, BoneInfo>& Model::GetBoneInfoMap() {
    return m_BoneInfoMap;
}

int& Model::GetBoneCount() {
    return m_BoneCounter;
}

const AssimpNodeData& Model::GetRootNode() const {
    return m_RootNode;
}

const vmath::mat4& Model::GetGlobalInverseTransform() const {
    return m_GlobalInverseTransform;
}
