cmake_minimum_required(VERSION 3.5.0)
project(video-app VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

pkg_check_modules(SFML REQUIRED IMPORTED_TARGET
    sfml-all
)

add_library(3rd-party-libs INTERFACE IMPORTED GLOBAL)
target_include_directories(3rd-party-libs INTERFACE
    ${FFMPEG_INCLUDE_DIRS}
    ${SFML_INCLUDE_DIRS}
)

target_link_options(3rd-party-libs INTERFACE
    ${FFMPEG_LINK_LIBRARIES}
    ${SFML_LINK_LIBRARIES}
)

include(cmake/copy-file.cmake)

set(BUNNY_FILE BigBuckBunny36010s5MB.mp4)
set(DESTINATION ${CMAKE_BINARY_DIR}/resources)

file(MAKE_DIRECTORY ${DESTINATION})

copy_resource_file(${BUNNY_FILE} ${DESTINATION})

add_executable(video-app main.cc
    FfmpegSmartPtrs.cc FrameExtractorFromFile.cc)

target_link_libraries(video-app 3rd-party-libs)
target_include_directories(video-app PRIVATE ${CMAKE_SOURCE_DIR})
