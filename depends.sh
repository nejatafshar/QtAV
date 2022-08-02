#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
P=$DIR/ffmpegBin && git -C $P pull || git clone --recursive -j8 https://192.168.1.17/afshar/ffmpegBin.git $P