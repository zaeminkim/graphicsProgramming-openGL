#include "Animation.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

vmath::mat4 Animation::ConvertMatrixToVMath(const aiMatrix4x4& from) {
    vmath::mat4 out;
    out[0] = vmath::vec4(from.a1, from.b1, from.c1, from.d1);
    out[1] = vmath::vec4(from.a2, from.b2, from.c2, from.d2);
    out[2] = vmath::vec4(from.a3, from.b3, from.c3, from.d3);
    out[3] = vmath::vec4(from.a4, from.b4, from.c4, from.d4);
    return out;
}

Animation::Animation(const std::string& animationPath, Model* model) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        animationPath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals
    );

    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0) {
        std::cout << "ERROR::ASSIMP::Animation load failed: " << animationPath << std::endl;
        return;
    }

    const aiAnimation* animation = scene->mAnimations[0];
    m_Duration = static_cast<float>(animation->mDuration);
    m_TicksPerSecond = (animation->mTicksPerSecond != 0.0)
        ? static_cast<float>(animation->mTicksPerSecond)
        : 25.0f;

    m_GlobalInverseTransform = model->GetGlobalInverseTransform();

    readHierarchyData(m_RootNode, scene->mRootNode);
    readMissingBones(animation, *model);
}

void Animation::readMissingBones(const aiAnimation* animation, Model& model) {
    std::unordered_map<std::string, BoneInfo>& boneInfoMap = model.GetBoneInfoMap();
    int& boneCount = model.GetBoneCount();

    for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
        const aiNodeAnim* channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
            if (boneCount >= MAX_BONES) {
                continue;
            }

            BoneInfo info;
            info.id = boneCount;
            info.offset = vmath::mat4::identity();
            boneInfoMap[boneName] = info;
            ++boneCount;
        }

        m_Bones.emplace_back(channel->mNodeName.data, boneInfoMap[boneName].id, channel);
    }

    m_BoneInfoMap = boneInfoMap;
}

void Animation::readHierarchyData(AssimpNodeData& dest, const aiNode* src) {
    dest.name = src->mName.data;
    dest.transformation = ConvertMatrixToVMath(src->mTransformation);
    dest.childrenCount = static_cast<int>(src->mNumChildren);
    dest.children.clear();
    dest.children.reserve(src->mNumChildren);

    for (unsigned int i = 0; i < src->mNumChildren; ++i) {
        AssimpNodeData child;
        readHierarchyData(child, src->mChildren[i]);
        dest.children.push_back(child);
    }
}

Bone* Animation::FindBone(const std::string& name) {
    for (auto& bone : m_Bones) {
        if (bone.GetBoneName() == name) {
            return &bone;
        }
    }
    return nullptr;
}

float Animation::GetTicksPerSecond() const {
    return m_TicksPerSecond;
}

float Animation::GetDuration() const {
    return m_Duration;
}

const AssimpNodeData& Animation::GetRootNode() const {
    return m_RootNode;
}

const std::unordered_map<std::string, BoneInfo>& Animation::GetBoneIDMap() const {
    return m_BoneInfoMap;
}

const vmath::mat4& Animation::GetGlobalInverseTransform() const {
    return m_GlobalInverseTransform;
}
