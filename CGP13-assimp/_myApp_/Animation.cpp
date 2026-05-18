#include "Animation.h"
#include "AssimpGLMHelpers.h"

#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

Animation::Animation(const std::string& animationPath, Model* model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights
    );

    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0)
    {
        std::cout << "ERROR::ANIMATION:: animation not found: " << animationPath << std::endl;
        return;
    }

    const aiAnimation* animation = scene->mAnimations[0];
    m_Duration = static_cast<float>(animation->mDuration);
    m_TicksPerSecond = animation->mTicksPerSecond != 0.0 ? static_cast<float>(animation->mTicksPerSecond) : 25.0f;

    ReadHeirarchyData(m_RootNode, scene->mRootNode);
    ReadMissingBones(animation, *model);
}

Bone* Animation::FindBone(const std::string& name)
{
    for (auto& bone : m_Bones)
    {
        if (bone.GetBoneName() == name)
            return &bone;
    }
    return nullptr;
}

void Animation::ReadMissingBones(const aiAnimation* animation, Model& model)
{
    int size = animation->mNumChannels;
    auto& boneInfoMap = model.GetBoneInfoMap();
    int& boneCount = model.GetBoneCount();

    for (int i = 0; i < size; ++i)
    {
        const aiNodeAnim* channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            BoneInfo info;
            info.id = boneCount;
            info.offset = glm::mat4(1.0f);
            boneInfoMap[boneName] = info;
            boneCount++;
        }

        m_Bones.push_back(Bone(channel->mNodeName.data, boneInfoMap[boneName].id, channel));
    }

    m_BoneInfoMap = boneInfoMap;
}

void Animation::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
{
    dest.name = src->mName.data;
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (unsigned int i = 0; i < src->mNumChildren; ++i)
    {
        AssimpNodeData newData;
        ReadHeirarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}
