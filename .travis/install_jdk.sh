#!/bin/bash

if [ $TRAVIS_OS_NAME = 'osx' ]; then
    brew tap AdoptOpenJDK/openjdk
    brew cask install adoptopenjdk8
elif [ $TRAVIS_OS_NAME = 'linux' ]; then
    sudo add-apt-repository -y ppa:saiarcot895/myppa
    sudo apt update
    sudo apt install openjdk-8-jdk
elif [ $TRAVIS_OS_NAME = 'windows' ]; then
  git clone https://github.com/portapps/portapps ${TRAVIS_BUILD_DIR}/../portapps
  source ${TRAVIS_BUILD_DIR}/../portapps/.travis/prepare.sh
  bash ${TRAVIS_BUILD_DIR}/../portapps/.travis/java.sh
fi