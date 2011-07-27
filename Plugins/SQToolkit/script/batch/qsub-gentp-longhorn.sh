#!/bin/bash

# (C) 2010 SciberQuest Inc.

#$ -V                                   # Inherit the submission environment
#$ -cwd                                 # Start job in submission dir
#$ -N gentp-batch                       # Job name
#$ -j y                                 # Combine stderr and stdout into stdout
#$ -o $HOME/$JOB_NAME.out               # Name of the output file

if [ -z $1 ]
then
  echo "Error: \$1 should be set to path of the dataset."
  exit
fi
DATA_PATH=$1

IBRUN_PATH=/share/sge6.2/default/pe_scripts

$IBRUN_PATH/ibrun /home/01237/bloring/apps/SVTK-PV3-3.8-icc-R/gentp $DATA_PATH 

