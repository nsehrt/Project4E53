#include "cliploader.h"

std::unique_ptr<AnimationClip> ClipLoader::loadCLP(const std::filesystem::directory_entry& fileName)
{
    /*open file*/
    std::streampos fileSize;
    std::ifstream file(fileName.path(), std::ios::binary);

    /*get file size*/
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    /*check header*/
    bool header = true;
    char headerBuffer[4] = { 'c','l','p','f' };

    for (int i = 0; i < 4; i++)
    {
        char temp;
        file.read(&temp, sizeof(temp));

        if (temp != headerBuffer[i])
        {
            header = false;
            break;
        }
    }

    if (header == false)
    {
        /*header incorrect*/
        return nullptr;
    }

    std::unique_ptr<AnimationClip> anim = std::make_unique<AnimationClip>();

    int slen = 0;
    file.read((char*)(&slen), sizeof(int));

    char* animName = new char[(INT_PTR)slen + 1];
    file.read(animName, slen);
    animName[slen] = '\0';

    anim->name = animName;

    delete[] animName;

    UINT numBones = 0;
    file.read((char*)(&numBones), sizeof(int));

    anim->boneAnimations.resize(numBones);

    for (UINT i = 0; i < numBones; i++)
    {
        UINT keyfrCount = 0;
        file.read((char*)(&keyfrCount), sizeof(int));

        if (keyfrCount == -1)
        {
            anim->boneAnimations[i].isEmpty = true;
            continue;
        }

        anim->boneAnimations[i].keyFrames.resize(keyfrCount);

        for (UINT j = 0; j < keyfrCount; j++)
        {

            float temp, temp2, temp3, temp4;
            file.read((char*)(&temp), sizeof(float));

            anim->boneAnimations[i].keyFrames[j].timeStamp = temp;

            file.read((char*)(&temp), sizeof(float));
            file.read((char*)(&temp2), sizeof(float));
            file.read((char*)(&temp3), sizeof(float));

            anim->boneAnimations[i].keyFrames[j].translation.x = temp;
            anim->boneAnimations[i].keyFrames[j].translation.y = temp2;
            anim->boneAnimations[i].keyFrames[j].translation.z = temp3;

            file.read((char*)(&temp), sizeof(float));
            file.read((char*)(&temp2), sizeof(float));
            file.read((char*)(&temp3), sizeof(float));

            anim->boneAnimations[i].keyFrames[j].scale.x = temp;
            anim->boneAnimations[i].keyFrames[j].scale.y = temp2;
            anim->boneAnimations[i].keyFrames[j].scale.z = temp3;

            file.read((char*)(&temp), sizeof(float));
            file.read((char*)(&temp2), sizeof(float));
            file.read((char*)(&temp3), sizeof(float));
            file.read((char*)(&temp4), sizeof(float));

            anim->boneAnimations[i].keyFrames[j].rotationQuat.x = temp;
            anim->boneAnimations[i].keyFrames[j].rotationQuat.y = temp2;
            anim->boneAnimations[i].keyFrames[j].rotationQuat.z = temp3;
            anim->boneAnimations[i].keyFrames[j].rotationQuat.w = temp4;

        }

    }

    anim->getStartTime();
    anim->getEndTime();

    return anim;
}
