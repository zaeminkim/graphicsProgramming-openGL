#ifndef BONE_H
#define BONE_H

#include <string>
#include <vector>

#include <assimp/scene.h>
#include <vmath.h>

struct KeyPosition {
    aiVector3D position;
    float timeStamp;
};

struct KeyRotation {
    aiQuaternion orientation;
    float timeStamp;
};

struct KeyScale {
    aiVector3D scale;
    float timeStamp;
};

class Bone {
public:
    Bone() = default;
    Bone(const std::string& name, int id, const aiNodeAnim* channel);

    void Update(float animationTime);

    const vmath::mat4& GetLocalTransform() const;
    const std::string& GetBoneName() const;
    int GetBoneID() const;

private:
    int getPositionIndex(float animationTime) const;
    int getRotationIndex(float animationTime) const;
    int getScaleIndex(float animationTime) const;
    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;

    vmath::mat4 interpolatePosition(float animationTime) const;
    vmath::mat4 interpolateRotation(float animationTime) const;
    vmath::mat4 interpolateScaling(float animationTime) const;
    static vmath::mat4 ConvertMatrixToVMath(const aiMatrix4x4& from);

private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;

    int m_NumPositions = 0;
    int m_NumRotations = 0;
    int m_NumScalings = 0;

    vmath::mat4 m_LocalTransform{vmath::mat4::identity()};
    std::string m_Name;
    int m_ID = -1;
};

#endif
