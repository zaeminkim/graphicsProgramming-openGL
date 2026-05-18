#include "Mesh.h"
#include <cstddef>

Mesh::Mesh(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    GLuint diffuseTexture
)
    : vertices(vertices),
    indices(indices),
    diffuseTexture(diffuseTexture),
    hasDiffuseTexture(diffuseTexture != 0)
{
    setupMesh();
}

Mesh::Mesh(Mesh&& other) noexcept
{
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);

    VAO = other.VAO;
    VBO = other.VBO;
    EBO = other.EBO;
    diffuseTexture = other.diffuseTexture;
    hasDiffuseTexture = other.hasDiffuseTexture;

    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.diffuseTexture = 0;
    other.hasDiffuseTexture = false;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        cleanup();
        vertices = std::move(other.vertices);
        indices = std::move(other.indices);

        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        diffuseTexture = other.diffuseTexture;
        hasDiffuseTexture = other.hasDiffuseTexture;

        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.diffuseTexture = 0;
        other.hasDiffuseTexture = false;
    }
    return *this;
}

Mesh::~Mesh()
{
    cleanup();
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, BoneIDs));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Weights));

    glBindVertexArray(0);
}

void Mesh::draw(GLuint shaderID)
{
    if (hasDiffuseTexture)
    {
        glUniform1i(glGetUniformLocation(shaderID, "material.useDiffuseMap"), 1);
        glUniform1i(glGetUniformLocation(shaderID, "material.diffuse"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
    }
    else
    {
        glUniform1i(glGetUniformLocation(shaderID, "material.useDiffuseMap"), 0);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::cleanup()
{
    if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);

    EBO = 0;
    VBO = 0;
    VAO = 0;
}
