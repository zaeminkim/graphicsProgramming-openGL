#pragma once

#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "Bone.h"
#include "Model.h"

struct AssimpNodeData
{
    glm::mat4 transformation = glm::mat4(1.0f);
    std::string name;
    int childrenCount = 0;
    std::vector<AssimpNodeData> children;
};

// glTF 파일 안의 애니메이션 전체를 읽어오는 클래스
// Assimp의 aiAnimation 읽기
// 애니메이션에 포함된 모든 Bone Channel 읽기
// 노드 계층 구조, duration, ticksPerSecond 저장
class Animation
{
public:
    Animation() = default;
    Animation(const std::string& animationPath, Model* model);
    Animation(const std::string& animationPath, Model* model, const std::string& animationName); // Animation 클래스가 특정 애니메이션 이름을 읽게 수정하기

    Bone* FindBone(const std::string& name);

    float GetTicksPerSecond() const { return m_TicksPerSecond; }
    float GetDuration() const { return m_Duration; }
    const AssimpNodeData& GetRootNode() const { return m_RootNode; }
    const std::map<std::string, BoneInfo>& GetBoneIDMap() const { return m_BoneInfoMap; }

private:
    void ReadMissingBones(const aiAnimation* animation, Model& model);
    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);

private:
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 1.0f;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
};
