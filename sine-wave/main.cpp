#include <iostream>
#include <RtAudio.h>

int main() {
    RtAudio audio;

    std::cout << "Audio devices:" << std::endl;
    for (auto& device : audio.getDeviceNames()) {
        std::cout << device << std::endl;
    }

    return EXIT_SUCCESS;
}