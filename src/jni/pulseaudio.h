#pragma once

#include <fake-jni/fake-jni.h>
#include <alsa/asoundlib.h>

class AudioDevice : public FakeJni::JObject {
    int err;
    unsigned int i;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;
    int dsize;


public:
    DEFINE_CLASS_NAME("org/fmod/AudioDevice")

    AudioDevice();

    FakeJni::JBoolean init(FakeJni::JInt channels, FakeJni::JInt samplerate, FakeJni::JInt c, FakeJni::JInt d);

    void write(std::shared_ptr<FakeJni::JByteArray> data, FakeJni::JInt length);

    void close();
};
