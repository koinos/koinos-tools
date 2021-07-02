#!/bin/bash

if ! [[ -z $BUILD_DOCKER ]]; then
   TAG="$TRAVIS_BRANCH"
   if [ "$TAG" = "master" ]; then
      TAG="latest"
   fi

   if echo "$DOCKER_PASSWORD" | docker login -u $DOCKER_USERNAME --password-stdin
   then
      echo "docker login succeeded"
   else
      echo "docker login failed"
      exit 1
   fi

   if docker push koinos/koinos-tools:$TAG
   then
      echo "docker push succeeded"
   else
      echo "docker push failed"
      exit 1
   fi
fi
