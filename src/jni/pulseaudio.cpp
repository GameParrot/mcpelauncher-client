#include "pulseaudio.h"
#include <game_window_manager.h>
#include <alsa/asoundlib.h>

AudioDevice::AudioDevice() {

}
FakeJni::JBoolean AudioDevice::init(FakeJni::JInt channels, FakeJni::JInt samplerate, FakeJni::JInt c, FakeJni::JInt d) 
{
    if (channels != 0) {
        fmtinfo.channels = channels;
        fmtinfo.samplerate = samplerate;
        fmtinfo.c = c;
        fmtinfo.d = d;
    }
    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) != 0) {
        GameWindowManager::getManager()->getErrorHandler()->onError("ALSA failed", snd_strerror(err));
        return false;
    }
    if ((err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, fmtinfo.channels, fmtinfo.samplerate, 1, 0)) != 0) {
        GameWindowManager::getManager()->getErrorHandler()->onError("ALSA failed", snd_strerror(err));
        return false;
    }
    return true;
}

void AudioDevice::write(std::shared_ptr<FakeJni::JByteArray> data, FakeJni::JInt length) {
    signed char *dbuffer = data->getArray();
    if (snd_pcm_state(handle) == 4) {
        init();
    }
    snd_pcm_writei(handle, dbuffer, (fmtinfo.c / fmtinfo.d) * fmtinfo.channels * 2);
}

void AudioDevice::close() {
    err = snd_pcm_drain(handle);
    if (err != 0)
        GameWindowManager::getManager()->getErrorHandler()->onError("snd_pcm_drain failed", snd_strerror(err));
    snd_pcm_close(handle);
}
