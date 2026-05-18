//vertex 데이터 저장
//index 데이터 저장
//texture 정보 저장
//VAO / VBO / EBO 생성
//glVertexAttribPointer 설정
//glDrawElements 호출
//=실제 OpenGL 버퍼를 관리하는 역할

#include "Mesh.h"

Mesh::Mesh(
    const std::vector<vmath::vec3>& vertices,
    const std::vector<vmath::vec2>& texCoords,
    const std::vector<vmath::vec3>& normals,
    const std::vector<unsigned int>& indices,
    GLuint diffuseTexture
)
    : vertices(vertices),
    texCoords(texCoords),
    normals(normals),
    indices(indices),
    VAO(0),
    VBO_pos(0),
    VBO_tex(0),
    VBO_norm(0),
    EBO(0),
    diffuseTexture(diffuseTexture),
    hasDiffuseTexture(diffuseTexture != 0)
{
    setupMesh();
}

Mesh::Mesh(Mesh&& other) noexcept
{
    vertices = std::move(other.vertices);
    texCoords = std::move(other.texCoords);
    normals = std::move(other.normals);
    indices = std::move(other.indices);

    VAO = other.VAO;
    VBO_pos = other.VBO_pos;
    VBO_tex = other.VBO_tex;
    VBO_norm = other.VBO_norm;
    EBO = other.EBO;

    diffuseTexture = other.diffuseTexture;
    hasDiffuseTexture = other.hasDiffuseTexture;

    other.VAO = 0;
    other.VBO_pos = 0;
    other.VBO_tex = 0;
    other.VBO_norm = 0;
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
        texCoords = std::move(other.texCoords);
        normals = std::move(other.normals);
        indices = std::move(other.indices);

        VAO = other.VAO;
        VBO_pos = other.VBO_pos;
        VBO_tex = other.VBO_tex;
        VBO_norm = other.VBO_norm;
        EBO = other.EBO;

        diffuseTexture = other.diffuseTexture;
        hasDiffuseTexture = other.hasDiffuseTexture;

        other.VAO = 0;
        other.VBO_pos = 0;
        other.VBO_tex = 0;
        other.VBO_norm = 0;
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
    glGenBuffers(1, &VBO_pos);
    glGenBuffers(1, &VBO_tex);
    glGenBuffers(1, &VBO_norm);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(vmath::vec3),
        vertices.data(),
        GL_STATIC_DRAW
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    if (!texCoords.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_tex);
        glBufferData(
            GL_ARRAY_BUFFER,
            texCoords.size() * sizeof(vmath::vec2),
            texCoords.data(),
            GL_STATIC_DRAW
        );
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    if (!normals.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_norm);
        glBufferData(
            GL_ARRAY_BUFFER,
            normals.size() * sizeof(vmath::vec3),
            normals.data(),
            GL_STATIC_DRAW
        );
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    if (!indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW
        );
    }

    glBindVertexArray(0);
}

void Mesh::draw(GLuint shaderID)
{
    if (hasDiffuseTexture)
    {
        glUniform1i(
            glGetUniformLocation(shaderID, "material.useDiffuseMap"),
            1
        );

        glUniform1i(
            glGetUniformLocation(shaderID, "material.diffuse"),
            0
        );

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
    }
    else
    {
        glUniform1i(
            glGetUniformLocation(shaderID, "material.useDiffuseMap"),
            0
        );
    }

    glBindVertexArray(VAO);

    if (indices.empty())
    {
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    }
    else
    {
        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indices.size()),
            GL_UNSIGNED_INT,
            nullptr
        );
    }

    glBindVertexArray(0);
}

void Mesh::cleanup()
{
    if (VBO_pos) glDeleteBuffers(1, &VBO_pos);
    if (VBO_tex) glDeleteBuffers(1, &VBO_tex);
    if (VBO_norm) glDeleteBuffers(1, &VBO_norm);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);

    VBO_pos = 0;
    VBO_tex = 0;
    VBO_norm = 0;
    EBO = 0;
    VAO = 0;
}