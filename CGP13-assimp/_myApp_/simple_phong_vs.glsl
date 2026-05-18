#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 3) in ivec4 boneIds;
layout (location = 4) in vec4 weights;

out vec3 vsPos;
out vec3 vsNormal;
out vec2 vsTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];
uniform int useAnimation;

void main()
{
    vec4 totalPosition = vec4(pos, 1.0);
    vec3 skinnedNormal = normal;

    if (useAnimation != 0)
    {
        totalPosition = vec4(0.0);
        skinnedNormal = vec3(0.0);

        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (boneIds[i] == -1)
                continue;

            if (boneIds[i] >= MAX_BONES)
            {
                totalPosition = vec4(pos, 1.0);
                skinnedNormal = normal;
                break;
            }

            vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(pos, 1.0);
            totalPosition += localPosition * weights[i];

            vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * normal;
            skinnedNormal += localNormal * weights[i];
        }
    }

    vsPos = vec3(model * totalPosition);
    vsNormal = mat3(transpose(inverse(model))) * skinnedNormal;
    vsTexCoord = texCoord;

    gl_Position = projection * view * vec4(vsPos, 1.0);
}
