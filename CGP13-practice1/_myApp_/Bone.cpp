#include "Bone.h"

#include <assimp/quaternion.h>

vmath::mat4 Bone::ConvertMatrixToVMath(const aiMatrix4x4& from) {
    vmath::mat4 out;
    out[0] = vmath::vec4(from.a1, from.b1, from.c1, from.d1);
    out[1] = vmath::vec4(from.a2, from.b2, from.c2, from.d2);
    out[2] = vmath::vec4(from.a3, from.b3, from.c3, from.d3);
    out[3] = vmath::vec4(from.a4, from.b4, from.c4, from.d4);
    return out;
}

Bone::Bone(const std::string& name, int id, const aiNodeAnim* channel)
    : m_Name(name), m_ID(id) {
    m_NumPositions = static_cast<int>(channel->mNumPositionKeys);
    for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex) {
        const aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        const float timeStamp = static_cast<float>(channel->mPositionKeys[positionIndex].mTime);
        m_Positions.push_back({ aiPosition, timeStamp });
    }

    m_NumRotations = static_cast<int>(channel->mNumRotationKeys);
    for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex) {
        const aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        const float timeStamp = static_cast<float>(channel->mRotationKeys[rotationIndex].mTime);
        m_Rotations.push_back({ aiOrientation, timeStamp });
    }

    m_NumScalings = static_cast<int>(channel->mNumScalingKeys);
    for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex) {
        const aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
        const float timeStamp = static_cast<float>(channel->mScalingKeys[keyIndex].mTime);
        m_Scales.push_back({ scale, timeStamp });
    }
}

void Bone::Update(float animationTime) {
    const vmath::mat4 translation = interpolatePosition(animationTime);
    const vmath::mat4 rotation = interpolateRotation(animationTime);
    const vmath::mat4 scale = interpolateScaling(animationTime);
    m_LocalTransform = translation * rotation * scale;
}

const vmath::mat4& Bone::GetLocalTransform() const {
    return m_LocalTransform;
}

const std::string& Bone::GetBoneName() const {
    return m_Name;
}

int Bone::GetBoneID() const {
    return m_ID;
}

int Bone::getPositionIndex(float animationTime) const {
    for (int index = 0; index < m_NumPositions - 1; ++index) {
        if (animationTime < m_Positions[index + 1].timeStamp) {
            return index;
        }
    }
    return m_NumPositions - 2;
}

int Bone::getRotationIndex(float animationTime) const {
    for (int index = 0; index < m_NumRotations - 1; ++index) {
        if (animationTime < m_Rotations[index + 1].timeStamp) {
            return index;
        }
    }
    return m_NumRotations - 2;
}

int Bone::getScaleIndex(float animationTime) const {
    for (int index = 0; index < m_NumScalings - 1; ++index) {
        if (animationTime < m_Scales[index + 1].timeStamp) {
            return index;
        }
    }
    return m_NumScalings - 2;
}

float Bone::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const {
    const float scale = animationTime - lastTimeStamp;
    const float frameDiff = nextTimeStamp - lastTimeStamp;
    if (frameDiff == 0.0f) {
        return 0.0f;
    }
    return scale / frameDiff;
}

vmath::mat4 Bone::interpolatePosition(float animationTime) const {
    if (m_NumPositions == 1) {
        const aiVector3D p = m_Positions[0].position;
        return vmath::translate(p.x, p.y, p.z);
    }

    const int p0Index = getPositionIndex(animationTime);
    const int p1Index = p0Index + 1;
    const float t = getScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);

    const aiVector3D& start = m_Positions[p0Index].position;
    const aiVector3D& end = m_Positions[p1Index].position;
    const aiVector3D blended = start + (end - start) * t;

    return vmath::translate(blended.x, blended.y, blended.z);
}

vmath::mat4 Bone::interpolateRotation(float animationTime) const {
    if (m_NumRotations == 1) {
        aiQuaternion rotation = m_Rotations[0].orientation;
        rotation.Normalize();
        aiMatrix4x4 rotationMatrix(rotation.GetMatrix());
        return ConvertMatrixToVMath(rotationMatrix);
    }

    const int p0Index = getRotationIndex(animationTime);
    const int p1Index = p0Index + 1;
    const float t = getScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);

    aiQuaternion blended;
    aiQuaternion::Interpolate(blended, m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, t);
    blended.Normalize();

    aiMatrix4x4 rotationMatrix(blended.GetMatrix());
    return ConvertMatrixToVMath(rotationMatrix);
}

vmath::mat4 Bone::interpolateScaling(float animationTime) const {
    if (m_NumScalings == 1) {
        const aiVector3D s = m_Scales[0].scale;
        return vmath::scale(s.x, s.y, s.z);
    }

    const int p0Index = getScaleIndex(animationTime);
    const int p1Index = p0Index + 1;
    const float t = getScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);

    const aiVector3D& start = m_Scales[p0Index].scale;
    const aiVector3D& end = m_Scales[p1Index].scale;
    const aiVector3D blended = start + (end - start) * t;

    return vmath::scale(blended.x, blended.y, blended.z);
}
