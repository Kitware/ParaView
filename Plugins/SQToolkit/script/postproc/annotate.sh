#!/bin/bash

INPUT=$1
shift 1

OUTPUT=$1
shift 1

TEXT=$1
shift 1

FONT_SIZE=$1
shift 1

FONT_COLOR=$1
shift 1

if [[ -z "$INPUT" || -z "$OUTPUT" || -z "$TEXT" ]]
then
  echo "Error: $0 /path/to/input /path/to/output text" 1>&2
  exit
fi

# default font size
if [[ -z "$FONT_SIZE" ]]
then
  FONT_SIZE=25
fi

# default font color
if [[ -z "$FONT_COLOR" ]]
then
  FONT_COLOR="w"
fi

gimp -i -b "(annotate \"$INPUT\" \"$OUTPUT\" \"$TEXT\" \"$FONT_SIZE\" \"$FONT_COLOR\")" -b "(gimp-quit 0)"
