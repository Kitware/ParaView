#!/bin/bash

#
# The vector fields are 3D, but we need their projection
# onto the x-y plane in order to trace streamlines.
# This script creates a set of dummy fields by symlinks
# to the x and y components of the original, and a symlink
# to an array of zeros for the z component. The genzero
# utility is used to make the array of zeros.
#

#dd if=/dev/zero of=./zeros.gda bs=1278960000 count=32


DATA_PATH=$1
shift 1

if [[ -z "$DATA_PATH" ]]
then
  echo "Usage: $0 /path/to/2d/run." 1>&2
  exit
fi

cd $DATA_PATH

BX=`ls bx_[0-9]*.gda`

for f in `ls bx_[0-9]*.gda` 
do
  STEP=`echo $f | cut -d_ -f2 | cut -d. -f1`
  echo -n "processing $STEP..."

  # B
  if [ -e bx_$STEP.gda ]
  then 
    ln -s bx_$STEP.gda  bpx_$STEP.gda
    ln -s zeros.gda     bpy_$STEP.gda
    ln -s bz_$STEP.gda  bpz_$STEP.gda
  fi

  # E
  if [ -e ex_$STEP.gda ]
  then
    ln -s ex_$STEP.gda  epx_$STEP.gda
    ln -s zeros.gda     epy_$STEP.gda
    ln -s ez_$STEP.gda  epz_$STEP.gda
  fi

  # V
  if [ -e vix_$STEP.gda ]
  then
    ln -s vix_$STEP.gda vipx_$STEP.gda
    ln -s zeros.gda     vipy_$STEP.gda
    ln -s viz_$STEP.gda vipz_$STEP.gda
  fi

  # ui 
  if [ -e uix_$STEP.gda ]
  then
    ln -s uix_$STEP.gda uipx_$STEP.gda
    ln -s zeros.gda     uipy_$STEP.gda
    ln -s uiz_$STEP.gda uipz_$STEP.gda
  fi

  # ue 
  if [ -e uex_$STEP.gda ]
  then
    ln -s uex_$STEP.gda uepx_$STEP.gda
    ln -s zeros.gda     uepy_$STEP.gda
    ln -s uez_$STEP.gda uepz_$STEP.gda
  fi

  echo "OK."
done
