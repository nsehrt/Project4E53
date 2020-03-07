#pragma once

#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "xaudio2.lib")

#define MAX_CHANNELS 64
#define SAMPLE_RATE 11025 //??

enum class SoundType
{
    Music,
    Effect
};

struct AudioData
{
    tWAVEFORMATEX waveFormat;
    unsigned int waveLength = 0;
    std::vector<BYTE> data;
    XAUDIO2_BUFFER audioBuffer;
    double length = 0;
    SoundType soundType = SoundType::Effect;
};

class SoundChannel
{
private:
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
    SoundEngine();
    ~SoundEngine();

    void loadFile(const std::wstring& _fileName, SoundType _soundType);
    bool add(const std::string& id, bool loop = false);

    void run();
    void Stop();

    void forceStop(unsigned char channel);

private:

    void update();
    void loadFile(const std::wstring& _fileName, std::vector<BYTE>& _data, WAVEFORMATEX** formatEx, unsigned int& length);

    std::unordered_map<std::string, AudioData*> soundCollection;
    std::vector<SoundChannel*> channels;

    IXAudio2* xaudioMain = nullptr;
    IXAudio2MasteringVoice* masterVoice = nullptr;

    IMFAttributes* srcReaderConfig = nullptr;

    std::atomic<bool> looped = true;
};