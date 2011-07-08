#!/bin/bash

#if [ $# != 3 ]
#then
# echo "usage: $0 input output"
# exit
#fi

INPUT=$1
OUTPUT=$2

ffmpeg -i $INPUT.mp4 -qscale 1 -vcodec msmpeg4v2 -acodec mp3 -mbd rd -flags +4mv+aic -trellis 2 -cmp 2 -subcmp 2 -g 300 -pass 1/2 $OUTPUT.avi

