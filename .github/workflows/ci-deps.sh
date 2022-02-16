#!/bin/bash

case $1 in
    macos-*)
        brew update
        brew install ninja sfiera/gn/gn
        pip3 install --user pytest pygments
        ;;

    ubuntu-*)
        echo "deb http://apt.arescentral.org `lsb_release -sc`  contrib" \
          | sudo tee /etc/apt/sources.list.d/arescentral.list
        sudo apt-key adv --keyserver keyserver.ubuntu.com --recv \
          5A4F5210FF46CEE4B799098BAC879AADD5B51AE9
        sudo apt-get update
        sudo apt-get install -y --no-install-recommends python3 python3-pip build-essential clang make gn ninja-build
        pip3 install --user pytest pygments
        ;;

    *)
        echo >&2 "$1: unknown runner os"
        exit 1
        ;;
esac
