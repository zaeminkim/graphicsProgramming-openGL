#pragma once

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <sb7.h>
#include <vmath.h>

class Mesh
{
public:
    std::vector<vmath::vec3> vertices;
    std::vector<vmath::vec2> texCoords;
    std::vector<vmath::vec3> normals;
    std::vector<unsigned int> indices;

    GLuint VAO;
    GLuint VBO_pos;
    GLuint VBO_tex;
    GLuint VBO_norm;
    GLuint EBO;

    // Mesh마다 다른 Texture 적용을 위해 추가
    GLuint diffuseTexture;
    bool hasDiffuseTexture;

public:
    Mesh(
        const std::vector<vmath::vec3>& vertices,
        const std::vector<vmath::vec2>& texCoords,
        const std::vector<vmath::vec3>& normals,
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