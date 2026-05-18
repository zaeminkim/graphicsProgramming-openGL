#pragma once

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <sb7.h>
#include <vmath.h>

constexpr int MAX_BONE_INFLUENCE = 4;

struct Vertex {
    vmath::vec3 Position;
    vmath::vec2 TexCoords;
    vmath::vec3 Normal;
    int BoneIDs[MAX_BONE_INFLUENCE];
    float Weights[MAX_BONE_INFLUENCE];

    Vertex()
        : Position(0.0f), TexCoords(0.0f), Normal(0.0f) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            BoneIDs[i] = -1;
            Weights[i] = 0.0f;
        }
    }
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    GLuint diffuseTexture;
    bool hasDiffuseTexture;
    bool hasNormals;

public:
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<unsigned int>& indices,
         GLuint diffuseTexture = 0);

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
