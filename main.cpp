#include <RtAudio.h>
#include <iostream>
#include <numbers>
#include <cmath>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std::string_literals;

constexpr unsigned int SampleRate = 44100;
constexpr unsigned int BufferSize = 256;
constexpr unsigned int Channels = 2;
constexpr float Gain = 0.2f;
constexpr float PeriodRad = 2.0f * std::numbers::pi;

struct SineState {
    float phase = 0.0f;
    float freq = 440.0f;
};

struct NoteInfo {
    float hz;
    const char* name;
};

constexpr float midiToHz(const int midi) {
    return 440.0f * std::exp2((static_cast<float>(midi) - 69.0f) / 12.0f);
}

void printStatus(const std::string& s) {
    static std::size_t last_len = 0;
    std::cout << '\r' << s;
    if (last_len > s.size()) std::cout << std::string(last_len - s.size(), ' ');
    std::cout << std::flush;
    last_len = s.size();
}

static const std::unordered_map<unsigned char, NoteInfo> KeyMap = {
    {'z', {midiToHz(60), "C4"}},
    {'x', {midiToHz(62), "D4"}},
    {'c', {midiToHz(64), "E4"}},
    {'v', {midiToHz(65), "F4"}},
    {'b', {midiToHz(67), "G4"}},
    {'n', {midiToHz(69), "A4"}},
    {'m', {midiToHz(71), "B4"}},
    {'s', {midiToHz(61), "C#4"}},
    {'d', {midiToHz(63), "D#4"}},
    {'g', {midiToHz(66), "F#4"}},
    {'h', {midiToHz(68), "G#4"}},
    {'j', {midiToHz(70), "A#4"}},
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
                auto noteIter = KeyMap.find(c);
                if (noteIter != KeyMap.end()) {
                    state.freq = noteIter->second.hz;
                    printStatus("Note: "s + noteIter->second.name);
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