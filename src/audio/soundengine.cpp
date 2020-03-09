#include "soundengine.h"
#include "../util/serviceprovider.h"

void SoundEngine::init()
{
    HRESULT hr = CoInitialize(0);

    if (hr != S_OK)
    {
        ServiceProvider::getVSLogger()->print<Severity::Warning>("Failed to initialize COM.");
        return;
    }

    XAudio2Create(&xaudioMain);
    xaudioMain->CreateMasteringVoice(&masterVoice);

    MFStartup(MF_VERSION);

    MFCreateAttributes(&srcReaderConfig, 1);
    srcReaderConfig->SetUINT32(MF_LOW_LATENCY, true);

    masterVoice->SetVolume(ServiceProvider::getSettings()->audioSettings.MasterVolume);

    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        channels.push_back(new SoundChannel());
    }

    isInit = true;
}


void SoundEngine::uninit()
{
    for (auto& i : soundCollection)
    {
        delete i.second;
    }

    for (auto& i : channels)
    {
        delete i;
    }

    MFShutdown();
    masterVoice->DestroyVoice();
    xaudioMain->StopEngine();

    CoUninitialize();

    isInit = false;
}


void SoundEngine::run()
{
    /*logger*/
    ServiceProvider::getVSLogger()->setThreadName("audioThread");
    ServiceProvider::getVSLogger()->print<Severity::Info>("Starting the audio engine.");

    if (!isInit)
    {
        ServiceProvider::getVSLogger()->print<Severity::Critical>("Sound engine is not initialized!");
        return;
    }

    /*Main loop*/

    while (looped)
    {
        audioTimer.Tick();

        update();
    }


    ServiceProvider::getVSLogger()->print<Severity::Info>("Shutting down audio engine.");

}

void SoundEngine::Stop()
{
    looped = false;
}



void SoundEngine::loadFile(const std::wstring& file, std::vector<BYTE>& data, WAVEFORMATEX** formatEx, unsigned int& length)
{

    DWORD streamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;

    /*open audio file*/
    IMFSourceReader* reader;
    MFCreateSourceReaderFromURL(file.c_str(), srcReaderConfig, &reader);
    reader->SetStreamSelection(streamIndex, true);

    /*get data type*/
    IMFMediaType* mediaType;
    reader->GetNativeMediaType(streamIndex, 0, &mediaType);

    /*check compressed or uncompressed*/
    GUID subType{};
    mediaType->GetGUID(MF_MT_MAJOR_TYPE, &subType);

    if (subType == MFAudioFormat_Float || subType == MFAudioFormat_PCM)
    {
        //uncompressed
    }
    else
    {
        //compressed
        IMFMediaType* partType = nullptr;
        MFCreateMediaType(&partType);

        partType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        partType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        reader->SetCurrentMediaType(streamIndex, NULL, partType);

    }

    IMFMediaType* uncompressedType = nullptr;
    reader->GetCurrentMediaType(streamIndex, &uncompressedType);
    MFCreateWaveFormatExFromMFMediaType(uncompressedType, formatEx, &length);
    reader->SetStreamSelection(streamIndex, true);

    //copy data
    IMFSample* sample = nullptr;
    IMFMediaBuffer* buffer = nullptr;
    BYTE* localAudioData = nullptr;
    DWORD localAudioDataLength = 0;

    while (true)
    {
        DWORD flags = 0;
        reader->ReadSample(streamIndex, 0, nullptr, &flags, nullptr, &sample);

        if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED || flags & MF_SOURCE_READERF_ENDOFSTREAM)
            break;

        if (sample == nullptr)
            continue;

        sample->ConvertToContiguousBuffer(&buffer);
        buffer->Lock(&localAudioData, nullptr, &localAudioDataLength);

        for (size_t i = 0; i < localAudioDataLength; i++)
            data.push_back(localAudioData[i]);

        buffer->Unlock();
        localAudioData = nullptr;
    }

    return;
}


void SoundEngine::loadFile(const std::wstring& fileName, SoundType st)
{
    AudioData* data = new AudioData();
    WAVEFORMATEX* wfx;
    loadFile(fileName, data->data, &wfx, data->waveLength);

    char id[128];
    char ext[8];

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &fileName[0], (int)fileName.size(), NULL, 0, NULL, NULL);
    std::string tStr(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &fileName[0], (int)fileName.size(), &tStr[0], size_needed, NULL, NULL);
    _splitpath_s(tStr.c_str(), NULL, 0, NULL, 0, id, 128, ext, 8);

    ZeroMemory(&data->audioBuffer, sizeof(XAUDIO2_BUFFER));
    data->waveFormat = *wfx;
    data->audioBuffer.AudioBytes = (UINT32)data->data.size();
    data->audioBuffer.pAudioData = (BYTE* const)&data->data[0];
    data->audioBuffer.pContext = nullptr;
    if (st == SoundType::Music)
    {
        data->audioBuffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }
    data->length = static_cast<double>(data->audioBuffer.AudioBytes) / SAMPLE_RATE;

    data->soundType = st;

    soundCollection.insert(std::make_pair(id, data));

    std::stringstream str;
    str << "Loaded file '" << id << ext << "' as " << (st == SoundType::Effect ? "effect" : "music") << ".";

    ServiceProvider::getVSLogger()->print<Severity::Info>(str.str().c_str());
}

void SoundEngine::add(unsigned int audioGuid, const std::string& fileId)
{
    std::lock_guard<std::mutex> lock(queueLock);

    AudioInData data(audioGuid, fileId);
    addQueue.push_back(std::move(data));
}


void SoundEngine::update()
{
    std::lock_guard<std::mutex> lock(channelLock);

    unsigned int usedChannel = MAX_CHANNELS + 1;

    /*check newly added queue*/
    queueLock.lock();

    if (!addQueue.size() == 0)
    {
        AudioInData data = addQueue.front();
        addQueue.pop_front();

        queueLock.unlock();


        if (soundCollection.find(data.fileId) == soundCollection.end() || data.id == 0)
        {
            std::stringstream str;
            str << "Trying to play unknown audio file: " << data.fileId << "!";

            ServiceProvider::getVSLogger()->print<Severity::Warning>(str.str().c_str());
        }
        else
        {

            /*find open voice channel*/
            for (int i = 0; i < MAX_CHANNELS; i++)
            {
                if (channels[i]->available)
                {
                    usedChannel = i;
                    channels[i]->available = false;
                    break;
                }
            }

            if (usedChannel == MAX_CHANNELS + 1)
            {
                ServiceProvider::getVSLogger()->print<Severity::Warning>("All sound channels in use!");
            }
            else
            {
                /* add data to channel */
                channels[usedChannel]->identifier = data.id;
                channels[usedChannel]->audio = soundCollection[data.fileId];
                channels[usedChannel]->timePlaying = 0.f;
                HRESULT hr = xaudioMain->CreateSourceVoice(&channels[usedChannel]->srcVoice, &channels[usedChannel]->audio->waveFormat);
                if (FAILED(hr))
                {
                    ServiceProvider::getVSLogger()->print<Severity::Warning>("Failed to create xaudio2 source voice!");
                }
            }

        }
    }
    else
    {
        queueLock.unlock();
    }
    

    /*check queue and play if necessary*/

    for (auto& c : channels)
    {

        if (c->available == false)
        {

            c->timePlaying += audioTimer.DeltaTime();

            if (c->isPlaying == false)
            {
                c->srcVoice->SubmitSourceBuffer(&c->audio->audioBuffer);

                /*custom volume per type*/
                if (c->audio->soundType == SoundType::Music)
                {
                    c->srcVoice->SetVolume(ServiceProvider::getSettings()->audioSettings.MusicVolume);
                }
                else if (c->audio->soundType == SoundType::Effect)
                {
                    c->srcVoice->SetVolume(ServiceProvider::getSettings()->audioSettings.EffectVolume);
                }

                c->srcVoice->Start();
                c->isPlaying = true;
            }
            //check if time is over length
            else
            {
                /*release voice if sound fully played*/
                if (c->timePlaying > c->audio->length)
                {
                    c->available = true;
                    c->srcVoice->Stop();
                    c->audio = nullptr;
                    c->timePlaying = 0;
                    c->isPlaying = false;
                }

            }
        }
    }

}


void SoundEngine::forceStop(unsigned int audioGuid)
{
    std::lock_guard<std::mutex> lock(channelLock);

    for (auto& c : channels)
    {
        if (c->identifier == audioGuid)
        {
            c->available = true;
            c->srcVoice->Stop();
            c->audio = nullptr;
            c->timePlaying = 0;
            c->isPlaying = false;
        }
    }
}