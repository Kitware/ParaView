#!/bin/bash

# (C) 2010 SciberQuest Inc.

#$ -V                                   # Inherit the submission environment
#$ -cwd                                 # Start job in submission dir
#$ -j y                                 # Combine stderr and stdout into stdout
#$ -o $HOME/$JOB_NAME.out               # Name of the output file

if [ -z $1 ]
then
  echo "Error: no run config."
  exit
fi
CONFIG_FILE=$1
shift 1

if [ -z $1 ]
then
  echo "Error: no bov file."
  exit
fi
BOV_FILE=$1
shift 1

if [ -z $1 ]
then
  echo "Error: no output path."
  exit
fi
OUTPUT_PATH=$1
shift 1

if [ -z $1 ]
then
  echo "Error: no base name."
  exit
fi
BASE_NAME=$1
shift 1

if [ -z $1 ]
then
  echo "Error: no start time."
  exit
fi
START_TIME=$1
shift 1

if [ -z $1 ]
then
  echo "Error: no end time."
  exit
fi
END_TIME=$1

module use -a /home/01237/bloring/modulefiles
module load PV3-3.8-icc-R
module load SVTK-PV3-3.8-icc-R

MTB_EXE=`which MagnetosphereTopoBatch`
echo "MTB_EXE=$MTB_EXE"
echo "CONFIG_FILE=$CONFIG_FILE"
echo "BOV_FILE=$BOV_FILE"
echo "OUTPUT_PATH=$OUTPUT_PATH"
echo "BASE_NAME=$BASE_NAME"
echo "START_TIME=$START_TIME"
echo "END_TIME=$END_TIME"

IBRUN_PATH=/share/sge6.2/default/pe_scripts

$IBRUN_PATH/ibrun MagnetosphereTopoBatch $CONFIG_FILE $BOV_FILE $OUTPUT_PATH $BASE_NAME $START_TIME $END_TIME

