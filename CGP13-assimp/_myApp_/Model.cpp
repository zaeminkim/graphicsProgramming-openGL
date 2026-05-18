//Assimp::Importer로 scene.gltf 읽기
//aiScene 가져오기
//aiNode를 재귀적으로 탐색
//aiMesh를 Mesh 클래스로 변환
//material / texture 정보 읽기
//여러 개의 Mesh를 관리
//=Assimp로 파일을 읽는 역할

#include "Model.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Texture.h"

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
    if (diffuseMap)
    {
        glDeleteTextures(1, &diffuseMap);
    }

    if (specularMap)
    {
        glDeleteTextures(1, &specularMap);
    }
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
        aiProcess_GenSmoothNormals
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }

    size_t slashPos = path.find_last_of("/\\");

    if (slashPos == std::string::npos)
    {
        directory = ".";
    }
    else
    {
        directory = path.substr(0, slashPos);
    }

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

// position, normal, index, material texture 정보를 Mesh로 넘기는 함수
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<vmath::vec3> vertices;
    std::vector<vmath::vec2> texCoords;
    std::vector<vmath::vec3> normals;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vmath::vec3 position;
        position[0] = mesh->mVertices[i].x;
        position[1] = mesh->mVertices[i].y;
        position[2] = mesh->mVertices[i].z;
        vertices.push_back(position);

        if (mesh->HasNormals())
        {
            vmath::vec3 normal;
            normal[0] = mesh->mNormals[i].x;
            normal[1] = mesh->mNormals[i].y;
            normal[2] = mesh->mNormals[i].z;
            normals.push_back(normal);
        }

        if (mesh->mTextureCoords[0])
        {
            vmath::vec2 texCoord;
            texCoord[0] = mesh->mTextureCoords[0][i].x;
            texCoord[1] = mesh->mTextureCoords[0][i].y;
            texCoords.push_back(texCoord);
        }
        else
        {
            texCoords.push_back(vmath::vec2(0.0f, 0.0f));
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }


    GLuint diffuseTexture = 0;

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiString texturePath;

        bool foundTexture = false;

        // glTF에서는 base color texture가 diffuse 역할을 함
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS)
        {
            foundTexture = true;
        }
        else if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
        {
            foundTexture = true;
        }

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

    return Mesh(vertices, texCoords, normals, indices, diffuseTexture);
}

void Model::setupMesh(
    int numVertices,
    GLfloat* positions,
    GLfloat* texCoords,
    GLfloat* normals
)
{
    std::vector<vmath::vec3> vertexPositions;
    std::vector<vmath::vec2> vertexTexCoords;
    std::vector<vmath::vec3> vertexNormals;
    std::vector<unsigned int> indices;

    for (int i = 0; i < numVertices; i++)
    {
        vertexPositions.push_back(
            vmath::vec3(
                positions[i * 3],
                positions[i * 3 + 1],
                positions[i * 3 + 2]
            )
        );
    }

    if (texCoords)
    {
        for (int i = 0; i < numVertices; i++)
        {
            vertexTexCoords.push_back(
                vmath::vec2(
                    texCoords[i * 2],
                    texCoords[i * 2 + 1]
                )
            );
        }
    }

    if (normals)
    {
        for (int i = 0; i < numVertices; i++)
        {
            vertexNormals.push_back(
                vmath::vec3(
                    normals[i * 3],
                    normals[i * 3 + 1],
                    normals[i * 3 + 2]
                )
            );
        }
    }

    meshes.push_back(
        Mesh(vertexPositions, vertexTexCoords, vertexNormals, indices)
    );
}

void Model::setupIndices(int numIndices, GLuint* indices)
{
    if (meshes.empty())
    {
        return;
    }

    std::vector<unsigned int> newIndices;

    for (int i = 0; i < numIndices; i++)
    {
        newIndices.push_back(indices[i]);
    }

    std::vector<vmath::vec3> vertices = meshes[0].vertices;
    std::vector<vmath::vec2> texCoords = meshes[0].texCoords;
    std::vector<vmath::vec3> normals = meshes[0].normals;

    meshes.clear();
    meshes.push_back(Mesh(vertices, texCoords, normals, newIndices));
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
    glUniform3fv(
        glGetUniformLocation(shaderID, "material.defaultAmbient"),
        1,
        defaultAmbient
    );

    glUniform3fv(
        glGetUniformLocation(shaderID, "material.defaultDiffuse"),
        1,
        defaultDiffuse
    );

    glUniform3fv(
        glGetUniformLocation(shaderID, "material.defaultSpecular"),
        1,
        defaultSpecular
    );

    glUniform1i(
        glGetUniformLocation(shaderID, "material.useDiffuseMap"),
        static_cast<int>(useDiffuseMap)
    );

    glUniform1i(
        glGetUniformLocation(shaderID, "material.useSpecularMap"),
        static_cast<int>(useSpecularMap)
    );

    glUniform1i(
        glGetUniformLocation(shaderID, "useNormal"),
        meshes.empty() ? 0 : static_cast<int>(!meshes[0].normals.empty())
    );

    glUniform1f(
        glGetUniformLocation(shaderID, "material.shininess"),
        shininess
    );

    //if (useDiffuseMap)
    //{
    //    glUniform1i(glGetUniformLocation(shaderID, "material.diffuse"), 0);
    //    glActiveTexture(GL_TEXTURE0);
    //    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    //}

    if (useSpecularMap)
    {
        glUniform1i(glGetUniformLocation(shaderID, "material.specular"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);
    }

    for (auto& mesh : meshes)
    {
        mesh.draw(shaderID);
    }
}