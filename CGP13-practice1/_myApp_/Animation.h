#ifndef ANIMATION_H
#define ANIMATION_H

#include <vector>
#include <unordered_map>
#include <string>

#include <assimp/scene.h>
#include <vmath.h>

#include "Bone.h"
#include "Model.h"

class Animation {
public:
    Animation() = default;
    Animation(const std::string& animationPath, Model* model);

    Bone* FindBone(const std::string& name);

    float GetTicksPerSecond() const;
    float GetDuration() const;
    const AssimpNodeData& GetRootNode() const;
    const std::unordered_map<std::string, BoneInfo>& GetBoneIDMap() const;
    const vmath::mat4& GetGlobalInverseTransform() const;

private:
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 25.0f;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    vmath::mat4 m_GlobalInverseTransform{vmath::mat4::identity()};

private:
    void readMissingBones(const aiAnimation* animation, Model& model);
    void readHierarchyData(AssimpNodeData& dest, const aiNode* src);
    static vmath::mat4 ConvertMatrixToVMath(const aiMatrix4x4& from);
};

#endif
