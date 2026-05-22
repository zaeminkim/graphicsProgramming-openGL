#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/anim.h>

struct KeyPosition
{
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation
{
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale
{
    glm::vec3 scale;
    float timeStamp;
};

// 애니메이션 키프레임 하나하나(팔 하나, 다리 하나, ...)를 보관하고, 현재 시간에 맞는 Bone 변환 행렬을 계산하는 클래스
// 현재 애니메이션 시간 입력 
// -> 해당 Bone의 position keyframe 찾기
// -> rotation keyframe 찾기
// -> scale keyframe 찾기
// -> 두 keyframe 사이를 보간
// -> localTransform 행렬 생성
class Bone
{
public:
    Bone(const std::string& name, int ID, const aiNodeAnim* channel);

    void Update(float animationTime);
    glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() const { return m_ID; }

private:
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);

    glm::mat4 InterpolatePosition(float animationTime);
    glm::mat4 InterpolateRotation(float animationTime);
    glm::mat4 InterpolateScaling(float animationTime);

    int GetPositionIndex(float animationTime);
    int GetRotationIndex(float animationTime);
    int GetScaleIndex(float animationTime);

private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;

    int m_NumPositions = 0;
    int m_NumRotations = 0;
    int m_NumScalings = 0;

    glm::mat4 m_LocalTransform = glm::mat4(1.0f);
    std::string m_Name;
    int m_ID = -1;
};
