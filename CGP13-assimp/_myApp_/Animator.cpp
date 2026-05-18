#include "Animator.h"

#include <glm/gtc/matrix_transform.hpp>

Animator::Animator(Animation* animation)
{
    m_CurrentTime = 0.0f;
    m_CurrentAnimation = animation;
    m_FinalBoneMatrices.reserve(100);
    for (int i = 0; i < 100; ++i)
        m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
}

void Animator::UpdateAnimation(float dt)
{
    m_DeltaTime = dt;
    if (m_CurrentAnimation)
    {
        m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
        float duration = m_CurrentAnimation->GetDuration();
        if (duration > 0.0f)
            m_CurrentTime = fmod(m_CurrentTime, duration);
        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
    }
}

void Animator::PlayAnimation(Animation* pAnimation)
{
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform)
{
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone* bone = m_CurrentAnimation->FindBone(nodeName);
    if (bone)
    {
        bone->Update(m_CurrentTime);
        nodeTransform = bone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;
    const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();

    auto it = boneInfoMap.find(nodeName);
    if (it != boneInfoMap.end())
    {
        int index = it->second.id;
        glm::mat4 offset = it->second.offset;
        if (index >= 0 && index < static_cast<int>(m_FinalBoneMatrices.size()))
            m_FinalBoneMatrices[index] = globalTransformation * offset;
    }

    for (int i = 0; i < node->childrenCount; ++i)
        CalculateBoneTransform(&node->children[i], globalTransformation);
}
