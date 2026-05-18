#include "Model.h"
#include "Texture.h"
#include "AssimpGLMHelpers.h"

#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

Model::Model()
    : diffuseMap(0),
    specularMap(0),
    shininess(32.0f),
    defaultAmbient(1.0f, 1.0f, 1.0f),
    defaultDiffuse(1.0f, 1.0f, 1.0f),
    defaultSpecular(0.0f, 0.0f, 0.0f),
    useDiffuseMap(false),
    useSpecularMap(false)
{
}

Model::~Model()
{
    if (diffuseMap) glDeleteTextures(1, &diffuseMap);
    if (specularMap) glDeleteTextures(1, &specularMap);
}

void Model::init()
{
    glGenTextures(1, &diffuseMap);
    glGenTextures(1, &specularMap);
}

bool Model::loadModel(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }

    size_t slashPos = path.find_last_of("/\\");
    directory = (slashPos == std::string::npos) ? "." : path.substr(0, slashPos);

    meshes.clear();
    m_BoneInfoMap.clear();
    m_BoneCounter = 0;

    processNode(scene->mRootNode, scene);
    return true;
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

void Model::SetVertexBoneDataToDefault(Vertex& vertex)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        vertex.BoneIDs[i] = -1;
        vertex.Weights[i] = 0.0f;
    }
}

void Model::SetVertexBoneData(Vertex& vertex, int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (vertex.BoneIDs[i] < 0)
        {
            vertex.BoneIDs[i] = boneID;
            vertex.Weights[i] = weight;
            return;
        }
    }
}

void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
{
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
        {
            BoneInfo newBoneInfo;
            newBoneInfo.id = m_BoneCounter;
            newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
            m_BoneInfoMap[boneName] = newBoneInfo;
            boneID = m_BoneCounter;
            m_BoneCounter++;
        }
        else
        {
            boneID = m_BoneInfoMap[boneName].id;
        }

        aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
        unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (unsigned int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            unsigned int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            if (vertexId < vertices.size())
                SetVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        SetVertexBoneDataToDefault(vertex);

        vertex.Position = vmath::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
            vertex.Normal = vmath::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        else
            vertex.Normal = vmath::vec3(0.0f, 1.0f, 0.0f);

        if (mesh->mTextureCoords[0])
            vertex.TexCoords = vmath::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.TexCoords = vmath::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    ExtractBoneWeightForVertices(vertices, mesh, scene);

    GLuint diffuseTexture = 0;

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texturePath;
        bool foundTexture = false;

        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS)
            foundTexture = true;
        else if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
            foundTexture = true;

        if (foundTexture)
        {
            std::string textureFile = texturePath.C_Str();
            std::string fullPath = directory + "/" + textureFile;

            glGenTextures(1, &diffuseTexture);
            if (!loadTextureFile(diffuseTexture, fullPath.c_str()))
            {
                std::cout << "Texture load failed: " << fullPath << std::endl;
                glDeleteTextures(1, &diffuseTexture);
                diffuseTexture = 0;
            }
            else
            {
                std::cout << "Texture loaded: " << fullPath << std::endl;
            }
        }
    }

    return Mesh(vertices, indices, diffuseTexture);
}

void Model::setupMesh(int numVertices, GLfloat* positions, GLfloat* texCoords, GLfloat* normals)
{
    std::vector<Vertex> vertexData;
    std::vector<unsigned int> indices;

    for (int i = 0; i < numVertices; i++)
    {
        Vertex vertex;
        SetVertexBoneDataToDefault(vertex);
        vertex.Position = vmath::vec3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
        vertex.TexCoords = texCoords ? vmath::vec2(texCoords[i * 2], texCoords[i * 2 + 1]) : vmath::vec2(0.0f, 0.0f);
        vertex.Normal = normals ? vmath::vec3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]) : vmath::vec3(0.0f, 1.0f, 0.0f);
        vertexData.push_back(vertex);
        indices.push_back(i);
    }

    meshes.push_back(Mesh(vertexData, indices));
}

void Model::setupIndices(int numIndices, GLuint* newIndices)
{
    if (meshes.empty()) return;

    std::vector<unsigned int> indices;
    for (int i = 0; i < numIndices; i++)
        indices.push_back(newIndices[i]);

    std::vector<Vertex> vertices = meshes[0].vertices;
    meshes.clear();
    meshes.push_back(Mesh(vertices, indices));
}

bool Model::loadDiffuseMap(const char* filepath)
{
    if (loadTextureFile(diffuseMap, filepath))
    {
        useDiffuseMap = true;
        return true;
    }
    useDiffuseMap = false;
    return false;
}

bool Model::loadSpecularMap(const char* filepath)
{
    if (loadTextureFile(specularMap, filepath))
    {
        useSpecularMap = true;
        return true;
    }
    useSpecularMap = false;
    return false;
}

void Model::draw(GLuint shaderID)
{
    glUniform3fv(glGetUniformLocation(shaderID, "material.defaultAmbient"), 1, defaultAmbient);
    glUniform3fv(glGetUniformLocation(shaderID, "material.defaultDiffuse"), 1, defaultDiffuse);
    glUniform3fv(glGetUniformLocation(shaderID, "material.defaultSpecular"), 1, defaultSpecular);

    glUniform1i(glGetUniformLocation(shaderID, "material.useDiffuseMap"), static_cast<int>(useDiffuseMap));
    glUniform1i(glGetUniformLocation(shaderID, "material.useSpecularMap"), static_cast<int>(useSpecularMap));
    glUniform1i(glGetUniformLocation(shaderID, "useNormal"), meshes.empty() ? 0 : 1);
    glUniform1f(glGetUniformLocation(shaderID, "material.shininess"), shininess);

    if (useSpecularMap)
    {
        glUniform1i(glGetUniformLocation(shaderID, "material.specular"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);
    }

    for (auto& mesh : meshes)
        mesh.draw(shaderID);
}
