#include "Model.h"
#include "Animation.h"
#include "Animator.h"

#include <sb7.h>
#include <vmath.h>
#include <shader.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <memory>
#include <string>

class my_application : public sb7::application {
public:
    GLuint compile_shader(const char* vs_file, const char* fs_file) {
        GLuint vertex_shader = sb7::shader::load(vs_file, GL_VERTEX_SHADER);
        GLuint fragment_shader = sb7::shader::load(fs_file, GL_FRAGMENT_SHADER);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        return program;
    }

    void startup() override {
        shader_program = compile_shader("simple_phong_vs.glsl", "simple_phong_fs.glsl");

        box.loadModel("model/fat_titan.gltf");
        boxPositions.push_back(vmath::vec3(0.0f, 0.0f, 0.0f));
        computeModelBounds();

        animation = std::make_unique<Animation>("model/fat_titan.gltf", &box);
        animator = std::make_unique<Animator>(animation.get());

        lastTime = 0.0;
    }

    void shutdown() override {
        glDeleteProgram(shader_program);
    }

    void render(double currentTime) override {
        const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glClearBufferfv(GL_COLOR, 0, black);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glViewport(0, 0, info.windowWidth, info.windowHeight);

        const float deltaTime = (lastTime == 0.0) ? 0.0f : static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        if (animator) {
            animator->UpdateAnimation(deltaTime);
        }

        const vmath::vec3 anchorPosition = boxPositions.empty() ? vmath::vec3(0.0f, 0.0f, 0.0f) : boxPositions[0];
        const vmath::vec3 center = anchorPosition + (modelCenterLocal * modelScale);

        const float worldRadius = std::max(modelRadiusLocal * modelScale, 0.5f);
        const vmath::vec3 target = center - vmath::vec3(0.0f, worldRadius * 0.4f, 0.0f);
        const vmath::vec3 eye = center + vmath::vec3(worldRadius * 1.2f, worldRadius * 2.3f, worldRadius * 2.2f);
        const vmath::vec3 up(0.0f, 1.0f, 0.0f);
        const vmath::mat4 viewM = vmath::lookat(eye, target, up);
        const vmath::mat4 projM = vmath::perspective(50.0f,
                                                     static_cast<float>(info.windowWidth) / static_cast<float>(info.windowHeight),
                                                     0.1f,
                                                     1000.0f);

        const vmath::vec3 lightPos(static_cast<float>(sin(currentTime * 0.5)),
                                   0.25f,
                                   static_cast<float>(cos(currentTime * 0.5)) * 0.7f);
        const vmath::vec3 viewPos = eye;
        const vmath::vec3 lightAmbient(0.2f, 0.2f, 0.2f);
        const vmath::vec3 lightDiffuse(0.5f, 0.5f, 0.5f);
        const vmath::vec3 lightSpecular(1.0f, 1.0f, 1.0f);

        glUseProgram(shader_program);

        glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, projM);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, viewM);

        glUniform3fv(glGetUniformLocation(shader_program, "viewPos"), 1, viewPos);
        glUniform3fv(glGetUniformLocation(shader_program, "light.position"), 1, lightPos);
        glUniform3fv(glGetUniformLocation(shader_program, "light.ambient"), 1, lightAmbient);
        glUniform3fv(glGetUniformLocation(shader_program, "light.diffuse"), 1, lightDiffuse);
        glUniform3fv(glGetUniformLocation(shader_program, "light.specular"), 1, lightSpecular);

        if (animator) {
            const auto& transforms = animator->GetFinalBoneMatrices();
            for (int i = 0; i < static_cast<int>(transforms.size()); ++i) {
                const std::string uniformName = "finalBonesMatrices[" + std::to_string(i) + "]";
                glUniformMatrix4fv(glGetUniformLocation(shader_program, uniformName.c_str()),
                                   1,
                                   GL_FALSE,
                                   transforms[i]);
            }
        }

        for (int i = 0; i < static_cast<int>(boxPositions.size()); ++i) {
            const float angle = 20.0f * i;
            vmath::mat4 modelM = vmath::translate(boxPositions[i])
                               * vmath::rotate(angle, 1.0f, 0.3f, 0.5f)
                               * vmath::scale(modelScale);
            glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelM);
            box.draw(shader_program);
        }
    }

private:
    void computeModelBounds() {
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();
        float maxZ = std::numeric_limits<float>::lowest();
        bool hasVertex = false;

        for (const auto& mesh : box.meshes) {
            for (const auto& vertex : mesh.vertices) {
                hasVertex = true;
                minX = std::min(minX, vertex.Position[0]);
                minY = std::min(minY, vertex.Position[1]);
                minZ = std::min(minZ, vertex.Position[2]);
                maxX = std::max(maxX, vertex.Position[0]);
                maxY = std::max(maxY, vertex.Position[1]);
                maxZ = std::max(maxZ, vertex.Position[2]);
            }
        }

        if (!hasVertex) {
            modelCenterLocal = vmath::vec3(0.0f, 0.0f, 0.0f);
            modelRadiusLocal = 1.0f;
            return;
        }

        modelCenterLocal = vmath::vec3((minX + maxX) * 0.5f,
                                       (minY + maxY) * 0.5f,
                                       (minZ + maxZ) * 0.5f);

        const float dx = maxX - minX;
        const float dy = maxY - minY;
        const float dz = maxZ - minZ;
        modelRadiusLocal = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.5f;
    }

    GLuint shader_program = 0;
    double lastTime = 0.0;
    float modelScale = 0.01f;
    vmath::vec3 modelCenterLocal = vmath::vec3(0.0f, 0.0f, 0.0f);
    float modelRadiusLocal = 1.0f;

    std::vector<vmath::vec3> boxPositions;
    Model box;

    std::unique_ptr<Animation> animation;
    std::unique_ptr<Animator> animator;
};

DECLARE_MAIN(my_application)
