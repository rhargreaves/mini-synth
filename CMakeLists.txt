cmake_minimum_required(VERSION 4.0)
project(mini_synth)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
        rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG        6.0.1 # pick a release tag
)
FetchContent_MakeAvailable(rtaudio)

add_executable(mini_synth main.cpp)

target_link_libraries(mini_synth PRIVATE rtaudio)