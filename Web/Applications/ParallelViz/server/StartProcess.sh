#!/bin/bash
# Argument set:
# 1: Base ParaView Path
# 2: Number of server process
# 3..n: regular pvpython args

port=`./findPort.sh`
echo "================================================"
echo mpirun -n $2 $1/bin/pvserver --server-port=$port &
echo $1/bin/pvpython $3 $4 $5 $6 $7 $8 $9 --ds-host localhost --ds-port $port
echo "================================================"

mpirun -n $2 $1/bin/pvserver --server-port=$port &
sleep 1
$1/bin/pvpython $3 $4 $5 $6 $7 $8 $9 --ds-host localhost --ds-port $port
