#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Animation.h"

// 매 프레임 실행되는 애니메이션 재생기
class Animator
{
public:
    Animator(Animation* animation);

    void UpdateAnimation(float dt);
    void PlayAnimation(Animation* pAnimation);
    void PlayAnimation(Animation* pAnimation, bool loop);
    bool IsAnimationFinished() const { return m_Finished; }

    const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }

private:
    void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform);

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation = nullptr;
    float m_CurrentTime = 0.0f;
    float m_DeltaTime = 0.0f;

    bool m_Loop = true;
    bool m_Finished = false;
};
