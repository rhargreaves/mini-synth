#include <RtAudio.h>
#include <iostream>
#include <numbers>
#include <cmath>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

constexpr unsigned int SampleRate = 44100;
constexpr unsigned int BufferSize = 256;
constexpr unsigned int Channels = 2;
constexpr float Gain = 0.2f;
constexpr float PeriodRad = 2.0f * std::numbers::pi;

struct SineState {
    float phase = 0.0f;
    float freq = 440.0f;
};

struct TermiosGuard {
    termios old{};
    bool ok = false;
    TermiosGuard() {
        if (tcgetattr(STDIN_FILENO, &old) == 0) ok = true;
        termios raw = old;
        raw.c_lflag &= ~(ICANON | ECHO);   // raw-ish: no canonical, no echo
        raw.c_cc[VMIN]  = 0;               // non-blocking read
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, fl | O_NONBLOCK); // non-blocking
    }
    ~TermiosGuard() {
        if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &old);
        int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, fl & ~O_NONBLOCK);
    }
};

int sineWave(void *outputBuffer, void *,
             unsigned int nBufferFrames,
             double, RtAudioStreamStatus, void *userData) {

    auto &[phase, freq] = *static_cast<SineState *>(userData);
    auto buffer = static_cast<float *>(outputBuffer);
    for (int i = 0; i < nBufferFrames; i++) {
        const float sample = std::sin(phase) * Gain;
        phase += PeriodRad * freq / static_cast<float>(SampleRate);
        if (phase >= PeriodRad) {
            phase -= PeriodRad;
        }

        for (int c = 0; c < Channels; c++) {
            *buffer++ = sample;
        }
    }
    return 0;
}

int main() {
    RtAudio audio;
    TermiosGuard termiosGuard;

    if (audio.getDeviceCount() == 0) {
        std::cerr << "No audio devices found.\n";
        return EXIT_FAILURE;
    }

    RtAudio::StreamParameters outParams{
        .deviceId = audio.getDefaultOutputDevice(),
        .nChannels = Channels,
        .firstChannel = 0};
    SineState state;
    unsigned int bufferSize = BufferSize;
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

    std::cout << "Playing... press <q> to quit.\n";
    bool running = true;
    while (running) {
        unsigned char buf[32];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n > 0) {
            for (ssize_t i = 0; i < n; ++i) {
                unsigned char c = buf[i];
                // handle key c
                if (c == 'q') {
                    running = false;
                }
                // map keys to notes here
                if (c == 'z') {
                    // set to middle C (C4)
                    state.freq = 261.625565f;
                }
            }
        }
        // do other work or sleep a bit
        usleep(1000); // 1 ms
    }

    if (audio.isStreamRunning()) {
        audio.stopStream();
    }

    return EXIT_SUCCESS;
}