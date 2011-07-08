#!/bin/bash


# (C) 2010 SciberQuest Inc.

# ParaView client side sever startup script.
#
# The script will be exec'd by the PV client to start a server using a 
# reverse connection on a reverse ssh tunnel to the cluster front end.
# 
# There are 5 positional command linie arguments:
# 
# 1) cluster front end url
# 2) cluster front end port number
# 3) localhost port number
# 4) a qsub command that specifies resources etc...
# 5) a path (on the front end) to the batch file that start pvserver
#

USAGE="Positional command line arguments:

  \$1 = USER@CLUSTER_LOGIN_HOST
          eg: frank@longhorn.tacc.utexas.edu

  \$2 = CLUSTER_LOGIN_HOST_PORT_NUMBER
          eg: 50002

  \$3 = LOCALHOST_PORT_NUMBER
          eg: 55555

  \$4 = QSUB_COMMAND
          eg: \"qsub -q normal -P vis -pe 4way 16 -l h_rt=01:00:00\"

  \$5 = /path/to/batch/job/script/that/starts/pvserver
          eg: \"/home/frank/pvserver-rc-batch.sh \`hostname\` 50002\"

NOTE: use quotes on compound arguments!
"

if [ -z "$1" ]
then
  printf "Error: Cluster front end URL is not set on \$1.\n"
  printf "$USAGE"
  exit
fi
FE_HOST=$1

if [ -z "$2" ]
then
  prrintf "Error: Cluster port number is not set on \$2."
  printf "$USAGE"
  exit
fi
FE_PORT=$2

if [ -z "$3" ]
then
  prrintf "Error:  Localhost port number is not set on \$3."
  printf "$USAGE"
  exit
fi
LOCAL_PORT=$3

if [ -z "$4" ]
then
  prrintf "Error: Qsub command not set on \$5."
  printf "$USAGE"
  exit
fi
QSUB_COMMAND="$4"

if [ -z "$5" ]
then
  prrintf "Error:  Path to batch script that start paraview in reverse-connecrtion mode is not set on \$4."
  printf "$USAGE"
  exit
fi
BATCH_SCRIPT=$5

# cmd="ssh -t -R $FE_PORT:localhost:$LOCAL_PORT $FE_HOST \"$QSUB_COMMAND $BATCH_SCRIPT && sleep 1d\""
# echo $cmd

xterm\
  -geometry 80x20+100+100 \
  -fg white \
  -bg black \
  -T ParaView_$LOCAL_PORT:$FE_HOST:$FE_PORT \
  -e ssh -t -R $FE_PORT:localhost:$LOCAL_PORT $FE_HOST "$QSUB_COMMAND $BATCH_SCRIPT && sleep 1d"

