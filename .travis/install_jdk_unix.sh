#!/bin/bash

if [ $TRAVIS_OS_NAME = 'osx' ]; then
    brew cask install adoptopenjdk8
elif [ $TRAVIS_OS_NAME = 'linux' ]; then
    sudo add-apt-repository ppa:saiarcot895/myppa
    sudo apt update
    sudo apt install openjdk-8-jdk
fi