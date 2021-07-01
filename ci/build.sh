#!/bin/bash

set -e
set -x

if [[ -z $BUILD_DOCKER ]]; then
   mkdir build
   cd build

   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --config Release --parallel 3
else
   TAG="$TRAVIS_BRANCH"
   if [ "$TAG" = "master" ]; then
      TAG="latest"
   fi

   cp -R ~/.ccache ./.ccache
   docker build . -t koinos-tools-ccache --target builder
   docker build . -t koinos/koinos-tools:$TAG
   docker run -td --name ccache koinos-tools-ccache
   docker cp ccache:/koinos-tools/.ccache ~/
fi
