#!/bin/bash

if [ $TRAVIS_OS_NAME = 'osx' ]; then
    brew tap AdoptOpenJDK/openjdk
    brew cask install adoptopenjdk8
    brew install zlib
elif [ $TRAVIS_OS_NAME = 'linux' ]; then
    sudo add-apt-repository -y ppa:saiarcot895/myppa
    sudo apt update
    sudo apt install openjdk-8-jdk zlib
fi