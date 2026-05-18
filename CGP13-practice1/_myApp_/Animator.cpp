#include "Animator.h"

#include <cmath>

Animator::Animator(Animation* currentAnimation)
    : m_CurrentAnimation(currentAnimation),
      m_CurrentTime(0.0f) {
    m_FinalBoneMatrices.reserve(MAX_BONES);
    for (int i = 0; i < MAX_BONES; ++i) {
        m_FinalBoneMatrices.push_back(vmath::mat4::identity());
    }
}

void Animator::UpdateAnimation(float deltaTime) {
    if (!m_CurrentAnimation) {
        return;
    }

    m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * deltaTime;
    const float duration = m_CurrentAnimation->GetDuration();
    if (duration > 0.0f) {
        m_CurrentTime = std::fmod(m_CurrentTime, duration);
    }

    calculateBoneTransform(&m_CurrentAnimation->GetRootNode(), vmath::mat4::identity());
}

void Animator::PlayAnimation(Animation* animation) {
    m_CurrentAnimation = animation;
    m_CurrentTime = 0.0f;
}

void Animator::calculateBoneTransform(const AssimpNodeData* node, const vmath::mat4& parentTransform) {
    const std::string nodeName = node->name;
    vmath::mat4 nodeTransform = node->transformation;

    Bone* bone = m_CurrentAnimation->FindBone(nodeName);
    if (bone) {
        bone->Update(m_CurrentTime);
        nodeTransform = bone->GetLocalTransform();
    }

    const vmath::mat4 globalTransformation = parentTransform * nodeTransform;

    const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    const auto it = boneInfoMap.find(nodeName);
    if (it != boneInfoMap.end()) {
        const int index = it->second.id;
        if (index >= 0 && index < MAX_BONES) {
            m_FinalBoneMatrices[index] =
                m_CurrentAnimation->GetGlobalInverseTransform() * globalTransformation * it->second.offset;
        }
    }

    for (int i = 0; i < node->childrenCount; ++i) {
        calculateBoneTransform(&node->children[i], globalTransformation);
    }
}

const std::vector<vmath::mat4>& Animator::GetFinalBoneMatrices() const {
    return m_FinalBoneMatrices;
}
