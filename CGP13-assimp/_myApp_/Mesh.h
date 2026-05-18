#pragma once

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <sb7.h>
#include <vmath.h>
#include <glm/glm.hpp>

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
    vmath::vec3 Position;
    vmath::vec2 TexCoords;
    vmath::vec3 Normal;
    int BoneIDs[MAX_BONE_INFLUENCE];
    float Weights[MAX_BONE_INFLUENCE];
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLuint diffuseTexture = 0;
    bool hasDiffuseTexture = false;

public:
    Mesh(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        GLuint diffuseTexture = 0
    );

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    ~Mesh();

    void setupMesh();
    void draw(GLuint shaderID);
    void cleanup();
};

#endif
