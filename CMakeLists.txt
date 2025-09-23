cmake_minimum_required(VERSION 4.0)
project(sine_wave)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
        rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG        6.0.1 # pick a release tag
)
FetchContent_MakeAvailable(rtaudio)

add_executable(sine_wave main.cpp)

target_link_libraries(sine_wave PRIVATE rtaudio)