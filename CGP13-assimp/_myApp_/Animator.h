#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Animation.h"

class Animator
{
public:
    Animator(Animation* animation);

    void UpdateAnimation(float dt);
    void PlayAnimation(Animation* pAnimation);
    const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }

private:
    void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform);

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation = nullptr;
    float m_CurrentTime = 0.0f;
    float m_DeltaTime = 0.0f;
};
