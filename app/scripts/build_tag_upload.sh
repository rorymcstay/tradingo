#!/bin/bash


source "$(dirname ${BASH_SOURCE[0]})/profile.env"

aws ecr get-login-password --region ${AWS_REGION} | docker login --username AWS --password-stdin ${ECR_REPOSITORY}


version=$(get_version $1)

docker build -f docker/tradingo$1.dockerfile --tag tradingo/$1:$version --tag tradingo/$1:$version .

docker tag tradingo/$1:$version ${ECR_REPOSITORY}/tradingo/$1:$version

docker push ${ECR_REPOSITORY}/trading/$1:$version
