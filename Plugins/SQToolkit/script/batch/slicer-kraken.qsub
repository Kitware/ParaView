#!/bin/bash

#PBS -N Slicer-xyz-100
#PBS -m abe
#PBS -M bloring@lbl.gov
#PBS -j eo
#PBS -A UT-TENN0035
#PBS -l size=504,walltime=16:00:00

cd $PBS_O_WORKDIR


export PV_HOME=/lustre/scratch/proj/sw/paraview/3.10.0/cnl2.2_gnu4.4.4-so

export LD_LIBRARY_PATH=$PV_HOME/lib/paraview-3.10/:$PV_HOME/lib:/opt/xt-pe/2.2.74/lib:/opt/cray/mpt/5.0.0/xt/seastar/mpich2-gnu/lib:/opt/cray/mpt/5.0.0/xt/seastar/sma/lib64:/opt/gcc/mpfr/2.3.1/lib:/opt/gcc/4.4.4/snos/lib64:/sw/xt/python/2.6.4/sles10.1_gnu4.1.2//lib/:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/lib64/mysql:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/lib64:/opt/cray/projdb/1.0.0-1.0202.19483.52.1/lib64:/opt/cray/csa/3.0.0-1_2.0202.21426.77.7/lib64:/opt/cray/job/1.5.5-0.1_2.0202.21413.56.7/lib64:/opt/torque/2.4.8/lib

export PATH=$PV_HOME/lib/paraview-3.10:/opt/cray/xt-asyncpe/4.0/bin:/opt/cray/pmi/1.0-1.0000.8256.50.1.ss/bin:/opt/gcc/4.4.4/bin:/nics/e/sw/tools/bin:/sw/xt/python/2.6.4/sles10.1_gnu4.1.2//bin:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/sbin:/opt/cray/MySQL/5.0.64-1.0202.2899.21.1/bin:/opt/cray/projdb/1.0.0-1.0202.19483.52.1/bin:/opt/cray/account/1.0.0-2.0202.19482.49.18/bin:/opt/cray/csa/3.0.0-1_2.0202.21426.77.7/sbin:/opt/cray/csa/3.0.0-1_2.0202.21426.77.7/bin:/opt/cray/job/1.5.5-0.1_2.0202.21413.56.7/bin:/opt/xt-lustre-ss/2.2.74_1.6.5/usr/sbin:/opt/xt-lustre-ss/2.2.74_1.6.5/usr/bin:/opt/xt-boot/2.2.74/bin/snos64:/opt/xt-os/2.2.74/bin/snos64:/opt/xt-service/2.2.74/bin/snos64:/opt/xt-prgenv/2.2.74/bin:/sw/altd/bin:/sw/xt/tgusage/3.0-r2/binary/bin:/sw/xt/bin:/usr/local/gold/bin:/usr/local/hsi/bin:/usr/local/openssh/bin:/opt/moab/5.4.3.s16991/bin:/opt/torque/2.4.8/bin:/opt/modules/3.1.6.5/bin:/usr/local/bin:/usr/bin:/usr/X11R6/bin:/bin:/usr/games:/opt/gnome/bin:/usr/lib/jvm/jre/bin:/usr/lib/mit/bin:/usr/lib/mit/sbin:/opt/pathscale/bin:.:/usr/lib/qt3/bin:/nics/c/home/bloring/bin

echo
echo "Starting slicer..."
echo 

echo "PWD="`pwd`
echo "PBS_O_WORKDIR=$PBS_O_WORKDIR"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "PATH=$PATH"

aprun -n 504 $PV_HOME/lib/paraview-3.10/Slicer 1  0 1 0  0 2918 0  100  /lustre/scratch/vadim/Asymmetric-4sp/data-striped/info.bov /lustre/scratch/bloring/slice-asym y ui ue b
echo "aprun pvserver exited with code $?."

aprun -n 504 $PV_HOME/lib/paraview-3.10/Slicer 1  1 0 0  2918 0 0  100  /lustre/scratch/vadim/Asymmetric-4sp/data-striped/info.bov /lustre/scratch/bloring/slice-asym x ui ue b
echo "aprun pvserver exited with code $?."

aprun -n 504 $PV_HOME/lib/paraview-3.10/Slicer 1  0 0 1  0 0 2918  100  /lustre/scratch/vadim/Asymmetric-4sp/data-striped/info.bov /lustre/scratch/bloring/slice-asym z ui ue b
echo "aprun pvserver exited with code $?."

