#pragma once

#include <fake-jni/fake-jni.h>
#include <alsa/asoundlib.h>

class AudioDevice : public FakeJni::JObject {

    int err;
    unsigned int i;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;
    struct AudioInfo {
        int channels = 0;
        int samplerate = 0;
        int c = 0;
        int d = 0;
    };
    AudioInfo fmtinfo;

public:
    DEFINE_CLASS_NAME("org/fmod/AudioDevice")

    AudioDevice();

    FakeJni::JBoolean init(FakeJni::JInt channels = 0, FakeJni::JInt samplerate = 0, FakeJni::JInt c = 0, FakeJni::JInt d = 0);

    void write(std::shared_ptr<FakeJni::JByteArray> data, FakeJni::JInt length);

    void close();
};
