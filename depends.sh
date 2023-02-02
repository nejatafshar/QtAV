#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GITSRV=github.com/nejatafshar

if [[ "$OSTYPE" == "linux-gnu"* ]]; then # Linux
   BRANCH=linux
elif [[ "$OSTYPE" == "darwin"* ]]; then # Mac OSX
   BRANCH=mac
elif [[ "$OSTYPE" == "cygwin" ]]; then # POSIX compatibility layer and Linux environment emulation for Windows
   BRANCH=win
elif [[ "$OSTYPE" == "msys" ]]; then # Lightweight shell and GNU utilities compiled for Windows (part of MinGW)
   BRANCH=win
else # Unknown
   BRANCH=master
fi

P=$DIR/ffmpegBin && git -C $P pull || git clone --recursive -j8 -b $BRANCH --single-branch https://$GITSRV/FFmpegBin.git $P