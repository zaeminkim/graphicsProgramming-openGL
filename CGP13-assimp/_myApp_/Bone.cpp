#include "Bone.h"
#include "AssimpGLMHelpers.h"

#include <glm/gtc/matrix_transform.hpp>

Bone::Bone(const std::string& name, int ID, const aiNodeAnim* channel)
    : m_Name(name), m_ID(ID)
{
    m_NumPositions = channel->mNumPositionKeys;
    for (int i = 0; i < m_NumPositions; ++i)
    {
        KeyPosition data;
        data.position = AssimpGLMHelpers::GetGLMVec(channel->mPositionKeys[i].mValue);
        data.timeStamp = static_cast<float>(channel->mPositionKeys[i].mTime);
        m_Positions.push_back(data);
    }

    m_NumRotations = channel->mNumRotationKeys;
    for (int i = 0; i < m_NumRotations; ++i)
    {
        KeyRotation data;
        data.orientation = AssimpGLMHelpers::GetGLMQuat(channel->mRotationKeys[i].mValue);
        data.timeStamp = static_cast<float>(channel->mRotationKeys[i].mTime);
        m_Rotations.push_back(data);
    }

    m_NumScalings = channel->mNumScalingKeys;
    for (int i = 0; i < m_NumScalings; ++i)
    {
        KeyScale data;
        data.scale = AssimpGLMHelpers::GetGLMVec(channel->mScalingKeys[i].mValue);
        data.timeStamp = static_cast<float>(channel->mScalingKeys[i].mTime);
        m_Scales.push_back(data);
    }
}

void Bone::Update(float animationTime)
{
    glm::mat4 translation = InterpolatePosition(animationTime);
    glm::mat4 rotation = InterpolateRotation(animationTime);
    glm::mat4 scale = InterpolateScaling(animationTime);
    m_LocalTransform = translation * rotation * scale;
}

float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    if (framesDiff <= 0.0f) return 0.0f;
    return midWayLength / framesDiff;
}

int Bone::GetPositionIndex(float animationTime)
{
    for (int index = 0; index < m_NumPositions - 1; ++index)
        if (animationTime < m_Positions[index + 1].timeStamp)
            return index;
    return m_NumPositions - 2;
}

int Bone::GetRotationIndex(float animationTime)
{
    for (int index = 0; index < m_NumRotations - 1; ++index)
        if (animationTime < m_Rotations[index + 1].timeStamp)
            return index;
    return m_NumRotations - 2;
}

int Bone::GetScaleIndex(float animationTime)
{
    for (int index = 0; index < m_NumScalings - 1; ++index)
        if (animationTime < m_Scales[index + 1].timeStamp)
            return index;
    return m_NumScalings - 2;
}

glm::mat4 Bone::InterpolatePosition(float animationTime)
{
    if (m_NumPositions == 0) return glm::mat4(1.0f);
    if (m_NumPositions == 1) return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
    glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Bone::InterpolateRotation(float animationTime)
{
    if (m_NumRotations == 0) return glm::mat4(1.0f);
    if (m_NumRotations == 1)
    {
        glm::quat rotation = glm::normalize(m_Rotations[0].orientation);
        return glm::toMat4(rotation);
    }

    int p0Index = GetRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
    glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);
    finalRotation = glm::normalize(finalRotation);
    return glm::toMat4(finalRotation);
}

glm::mat4 Bone::InterpolateScaling(float animationTime)
{
    if (m_NumScalings == 0) return glm::mat4(1.0f);
    if (m_NumScalings == 1) return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

    int p0Index = GetScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}
