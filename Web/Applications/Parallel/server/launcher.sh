#!/bin/bash

# Configuration paths
PV_WEB_SERVER_PATH=/paraview-build-tree/lib/site-packages/paraview/web
PV_BUILD_PATH=/.../FIXME/paraview-build-tree
DATA_DIR=/.../FIXME/ParaViewData/Data

PVPYTHON=$PV_BUILD_PATH/bin/pvpython
PVSERVER=$PV_BUILD_PATH/bin/pvserver

# expect PORT CLIENT RESOURCE FILE
usage(){
    echo "Usage: $0 portNumber client-app-name resources file-to-load"
    echo "   portNumber: The port on which the ParaViewWeb app should listen"
    echo "   client-app-name: [loader, visualizer]"
    echo "   resource: Number of nodes to run on"
    echo "   file-to-load: File that should be pre-loaded is any"
    echo "   secret: Password used to authenticate user"
    exit 1
}

# Check if we got all the command line arguments we need
if [[ $# -ne 5 ]]
then
    usage
fi

# Grab the command line arguments into variables
HTTPport=$1
client=$2
resource=$3
file=$4
secret=$5
server=$PV_WEB_SERVER_PATH/pv_web_file_loader.py

# Find the server script to use
if [ $client = 'FileViewer' ]
then server=$PV_WEB_SERVER_PATH/pv_web_file_loader.py
fi
if [ $client = 'Visualizer' ]
then server=$PV_WEB_SERVER_PATH/pv_web_visualizer.py
fi

# Find available MPI port
MPIport=11111
for MPIportSearch in {11111..11211}
do
  # Linux : result=`netstat -vatn | grep ":$port " | wc -l`
  # OSX   : result=`netstat -vatn | grep ".$port " | wc -l`
  result=`netstat -vatn | grep ".$MPIportSearch " | wc -l`
  if [ $result == 0 ]
  then
      MPIport=$MPIportSearch
      break
  fi
done

# Run the MPI server
echo mpirun -n $resource $PVSERVER --server-port=$MPIport &
mpirun -n $resource $PVSERVER --server-port=$MPIport &

# Wait for the server to start and run the client
sleep 1
echo $PVPYTHON $server -f --ds-host localhost --ds-port $MPIport --port $HTTPport --data-dir $DATA_DIR --authKey $secret
$PVPYTHON $server -f --ds-host localhost --ds-port $MPIport --port $HTTPport --data-dir $DATA_DIR --authKey $secret
