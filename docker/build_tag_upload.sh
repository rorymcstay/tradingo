#!/bin/bash


source "$(dirname ${BASH_SOURCE[0]})/profile.env"

version=$(get_version $1)
component=$1

local_tag=tradingo/$component:$version
remote_tag=rmcstay95/"$(if [[ $component == "tradingo" ]]; then echo tradingo; else echo tradingo-$component; fi ):$version"

docker_file=$(if [[ $component == "tradingo" ]]; then echo "tradingo.dockerfile"; else echo "tradingo-$component.dockerfile"; fi)

docker build -f docker/$docker_file --tag $local_tag .

set -x
docker tag $local_tag $remote_tag

docker login --username $DOCKER_USERNAME --password $DOCKER_PASSWORD
docker push $remote_tag
