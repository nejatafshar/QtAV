#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GITSRV=github.com/nejatafshar
P=$DIR/ffmpegBin && git -C $P pull || git clone --recursive -j8 -b ios --single-branch https://$GITSRV/FFmpegBin.git $P