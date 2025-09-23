#include <RtAudio.h>
#include <iostream>

constexpr unsigned int SampleRate = 44100;
constexpr unsigned int BufferSize = 1024;
constexpr unsigned int Channels = 2;

int sineWave(void *outputBuffer, void *inputBuffer,
             unsigned int nBufferFrames, double streamTime,
             RtAudioStreamStatus status, void *userData) {

    auto buffer = static_cast<float *>(outputBuffer);
    for (int i = 0; i < nBufferFrames; i++) {
        buffer[i] = sin(2 * M_PI * 440 * i / SampleRate);
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

    auto res = audio.openStream(&outParams,
        nullptr, RTAUDIO_FLOAT32,
                                SampleRate, &bufferSize, &sineWave);
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