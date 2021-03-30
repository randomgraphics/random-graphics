#!/bin/bash
echo Login to dockerhub.com as randomgraphics...
docker login -u randomgraphics
docker push randomgraphics/vulkan-android:latest
