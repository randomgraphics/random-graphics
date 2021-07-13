#!/bin/bash
echo Login to dockerhub.com as randomgraphics...
docker login -u randomgraphics
docker push randomgraphics/vulkan:11.2.182-cuda-11.3.1-ubuntu-20.04
docker push randomgraphics/vulkan-android:1.2.182-ndk-22.1.7171670-cuda-11.3.1-ubuntu-20.04
