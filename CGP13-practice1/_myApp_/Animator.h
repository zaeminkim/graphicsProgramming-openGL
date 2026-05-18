#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <vector>

#include <vmath.h>

#include "Animation.h"

class Animator {
public:
    explicit Animator(Animation* currentAnimation);

    void UpdateAnimation(float deltaTime);
    void PlayAnimation(Animation* animation);

    const std::vector<vmath::mat4>& GetFinalBoneMatrices() const;

private:
    void calculateBoneTransform(const AssimpNodeData* node, const vmath::mat4& parentTransform);

private:
    std::vector<vmath::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
};

#endif
