#version 430 core

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 5) in ivec4 boneIDs;
layout (location = 6) in vec4 weights;

out vec3 vsPos;
out vec3 vsNormal;
out vec2 vsTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main() {
    vec4 skinnedPosition = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        int boneID = boneIDs[i];
        float weight = weights[i];

        if (boneID >= 0 && boneID < MAX_BONES && weight > 0.0) {
            mat4 boneTransform = finalBonesMatrices[boneID];
            skinnedPosition += (boneTransform * vec4(pos, 1.0)) * weight;
            skinnedNormal += mat3(boneTransform) * normal * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight <= 0.0) {
        skinnedPosition = vec4(pos, 1.0);
        skinnedNormal = normal;
    }

    vec4 worldPos = model * skinnedPosition;
    vsPos = worldPos.xyz;
    vsNormal = normalize(mat3(transpose(inverse(model))) * skinnedNormal);
    vsTexCoord = texCoord;

    gl_Position = projection * view * worldPos;
}
