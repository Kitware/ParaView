#!/bin/bash
#
# axes INFILE OUTFILE x0 x1 y0 y1 nx ny xLabel yLabel title xOffset yOffset fontSize fontType"
#


echo "$0"

if [ $# != 13 ] ; then
  echo "Usage: $0 inFile outFile x0 x1 y0 y1 nx ny xLabel yLabel title xOffset yOffset fontSize fontType"
  exit 1 
fi

gimp  -i -b "(sciber-axes  \"$1\" \"$2\" \"$3\" \"$4\" \"$5\" \"$6\" $7 $8  \"$9\" \"${10}\" \"${11}\" ${12} \"${13}\"  )"  -b '(gimp-quit 0)'
