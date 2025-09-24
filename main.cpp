#include <RtAudio.h>
#include <iostream>
#include <numbers>
#include <cmath>

constexpr unsigned int SampleRate = 44100;
constexpr unsigned int BufferSize = 256;
constexpr unsigned int Channels = 2;
constexpr float Gain = 0.2f;
constexpr float PeriodRad = 2.0f * std::numbers::pi;

struct SineState {
    float phase = 0.0f;
    float freq = 440.0f;
};

int sineWave(void *outputBuffer, void *,
             unsigned int nBufferFrames,
             double, RtAudioStreamStatus, void *userData) {

    auto& state = *static_cast<SineState *>(userData);
    auto buffer = static_cast<float *>(outputBuffer);
    for (int i = 0; i < nBufferFrames; i++) {
        float sample = std::sin(state.phase) * Gain;
        state.phase += PeriodRad * state.freq / static_cast<float>(SampleRate);
        if (state.phase >= PeriodRad) {
            state.phase -= PeriodRad;
        }

        for (int c = 0; c < Channels; c++) {
            *buffer++ = sample;
        }
    }
    return 0;
}

int main() {
    RtAudio audio;

    if (audio.getDeviceCount() == 0) {
        std::cerr << "No audio devices found.\n";
        return EXIT_FAILURE;
    }

    RtAudio::StreamParameters outParams{
        .deviceId = audio.getDefaultOutputDevice(),
        .nChannels = Channels,
        .firstChannel = 0};
    unsigned int bufferSize = BufferSize;

    SineState state;

    auto res = audio.openStream(&outParams,
        nullptr, RTAUDIO_FLOAT32,
                                SampleRate, &bufferSize, &sineWave, &state);
    if (res != RTAUDIO_NO_ERROR) {
        std::cerr << "Failed to open stream: " << res << "\n";
        return EXIT_FAILURE;
    }

    if (audio.startStream() != RTAUDIO_NO_ERROR) {
        std::cerr << "Failed to start stream.\n";
        return EXIT_FAILURE;
    }

    char input;
    std::cout << "Playing... press <enter> to quit.\n";
    std::cin.get(input);

    if (audio.isStreamRunning()) {
        audio.stopStream();
    }

    return EXIT_SUCCESS;
}