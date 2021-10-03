#!/bin/bash


source "$(dirname ${BASH_SOURCE[0]})/profile.env"

version=$(get_version $1)

docker login --username $DOCKER_USERNAME --password $DOCKER_PASSWORD

docker build -f docker/tradingo-$1.dockerfile --tag tradingo/$1:$version --tag tradingo/$1:$version .

docker tag tradingo/$1:$version rmcstay95/tradingo-$1:$version

docker push rmcstay95/tradingo-$1:$version
