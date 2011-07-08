#!/bin/bash

# (C) 2010 SciberQuest Inc.
#
# pvserver-rc-batch.sh
#
# SGE batch script to start a pvserver group using it's reverse connection option.
#
# This script requires the follwing positional command line options:
#
# 1) the login node hostname
# 2) the port number used in the reverse tunnel.
# 3) the path to a ParaView install
#

#$ -V                                   # Inherit the submission environment
#$ -cwd                                 # Start job in submission dir
#$ -N pvserver-fc-batch                 # Job name
#$ -j y                                 # Combine stderr and stdout into stdout
#$ -o $HOME/$JOB_NAME.out               # Name of the output file

if [ -z $1 ]
then
  echo "Error: \$1 should be set to login node hostname."
  exit
fi
FE_HOST=$1

if [ -z $2 ]
then
  echo "Error: \$2 should be set to the port number."
  exit
fi
FE_PORT=$2

if [ -z $3 ]
then
  echo "Error: \$3 should point to the top directory of a ParaView install."
  exit
fi
PV_PATH=$3

DISP_SCRIPT=/share/sge6.2/default/pe_scripts/tacc_xrun
IBRUN_PATH=/share/sge6.2/default/pe_scripts

echo "Starting ParaView at $PV_PATH/bin/pvserver $FE_HOST:$FE_PORT."


ssh -fNgt -R $FE_PORT:localhost:$FE_PORT $FE_HOST

export NO_HOSTSORT=1

$IBRUN_PATH/ibrun $DISP_SCRIPT $PV_PATH/bin/pvserver --server-port=$FE_PORT

