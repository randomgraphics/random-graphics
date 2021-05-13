#!/bin/bash
set -e

docker build -t randomgraphics/vulkan:11.2.162-cuda-11.3.0-ubuntu-20.04 $@ -<<EOF
FROM nvidia/cudagl:11.3.0-devel-ubuntu20.04

LABEL description="random-graphics-vulkan"

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y wget gnupg2
RUN wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
RUN wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.162-focal.list https://packages.lunarg.com/vulkan/1.2.162/lunarg-vulkan-1.2.162-focal.list

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    git-lfs \
    ninja-build \
    vulkan-sdk \
    libglfw3-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev
EOF

docker build -t randomgraphics/vulkan-android:1.2.162-ndk-22.1.7171670-cuda-11.3.0-ubuntu-20.04 $@ -<<EOF
FROM randomgraphics/vulkan:11.2.162-cuda-11.3.0-ubuntu-20.04

# download android studio (maybe not needed?)
RUN wget -qO /opt/android-studio-ide-201.7042882-linux.tar.gz https://redirector.gvt1.com/edgedl/android/studio/ide-zips/4.1.2.0/android-studio-ide-201.7042882-linux.tar.gz
RUN cd /opt;tar xzf android-studio-ide-201.7042882-linux.tar.gz
RUN mkdir /opt/android;mv /opt/android-studio /opt/android/studio
RUN rm /opt/android-studio-ide-201.7042882-linux.tar.gz

# download sdk tools
RUN apt-get install unzip
RUN cd /opt;wget -q https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip
RUN cd /opt;unzip commandlinetools-linux-6858069_latest.zip
RUN mkdir -p /opt/android/sdk/cmdline-tools
RUN mv /opt/cmdline-tools /opt/android/sdk/cmdline-tools/latest

# download sdk components
ARG JAVA_HOME=/opt/android/studio/jre
RUN yes|/opt/android/sdk/cmdline-tools/latest/bin/sdkmanager --licenses
RUN /opt/android/sdk/cmdline-tools/latest/bin/sdkmanager --install "ndk;22.1.7171670"
RUN /opt/android/sdk/cmdline-tools/latest/bin/sdkmanager --install "build-tools;30.0.3"
RUN /opt/android/sdk/cmdline-tools/latest/bin/sdkmanager --install "platforms;android-30"
RUN /opt/android/sdk/cmdline-tools/latest/bin/sdkmanager --install "cmake;3.10.2.4988404"
EOF
