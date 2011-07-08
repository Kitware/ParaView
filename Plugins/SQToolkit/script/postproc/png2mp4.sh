#!/bin/bash

if [ $# != 3 ]
then
  echo "usage: $0 pattern rate output"
  exit
fi

INPUT_PATTERN=$1
FRAME_RATE=$2
OUTPUT_NAME=$3

ffmpeg -loop_output 0 -qscale 1 -r $FRAME_RATE -b 9600 -i $INPUT_PATTERN $OUTPUT_NAME

