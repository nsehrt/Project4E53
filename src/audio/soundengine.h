#pragma once

#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <deque>
#include "../core/gametime.h"

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "xaudio2.lib")

#define MAX_CHANNELS 64
#define SAMPLE_RATE 11025 //??

#define SOUND_PATH_MUSIC "data/sound/music"
#define SOUND_PATH_EFFECTS "data/sound/effects"

enum class SoundType
{
    Music,
    Effect
};

struct AudioInData
{
    unsigned int id = 0;
    std::string fileId = "";

    AudioInData(unsigned int _id, const std::string& _fileId) :
        id(_id), fileId(_fileId) {}
    AudioInData() : id(0), fileId(""){}
};

struct AudioData
{
    tWAVEFORMATEX waveFormat = {};
    unsigned int waveLength = 0;
    std::vector<BYTE> data;
    XAUDIO2_BUFFER audioBuffer = {};
    double length = 0;
    SoundType soundType = SoundType::Effect;
};

class SoundChannel
{
private:
    unsigned int identifier = 0;
    IXAudio2SourceVoice* srcVoice = nullptr;
    AudioData* audio = nullptr;
    float timePlaying = 0;
    bool available = true;
    bool isPlaying = false;

public:
    SoundChannel() = default;
    friend class SoundEngine;
};


class SoundEngine
{
public:
    SoundEngine() = default;
    ~SoundEngine() = default;

    /*request to play audio*/
    void add(unsigned int _audioGuid, const std::string& _fileId);

    /*stop a sound with the id given in add*/
    void forceStop(unsigned int audioGuid);

    /*call this before run*/
    void init();

    /*call this after stop*/
    void uninit();

    /* call this in its own thread*/
    void run();

    /*use this to stop the engine/run loop*/
    void Stop();

    /*check if loading the audio data is finished*/
    bool loadingFinished()
    {
        return isLoaded;
    }

private:

    void update();
    bool loadFile(const std::wstring& _fileName, std::vector<BYTE>& _data, WAVEFORMATEX** formatEx, unsigned int& length);

    /*load files into the collection before calling run*/
    bool loadFile(const std::wstring& _fileName, SoundType _soundType);

    /*holds all loaded audio files*/
    std::unordered_map<std::string, AudioData*> soundCollection;

    /*holds all the channels*/
    std::vector<SoundChannel*> channels;

    /*holds items that will be added to the actual channels*/
    std::deque<AudioInData> addQueue;
    std::mutex queueLock;
    std::mutex channelLock;

    /*use for update delta time*/
    GameTime audioTimer;

    /*xaudio2 specific*/
    IXAudio2* xaudioMain = nullptr;
    IXAudio2MasteringVoice* masterVoice = nullptr;
    IMFAttributes* srcReaderConfig = nullptr;

    /*run the engine while this is true*/
    std::atomic<bool> looped = true;

    /*bool whether audio engine is initialized*/
    std::atomic<bool> isInit = false;
    std::atomic<bool> isLoaded = false;
};