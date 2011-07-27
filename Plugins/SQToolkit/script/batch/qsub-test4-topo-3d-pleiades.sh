#PBS -S /bin/bash
#PBS -N t4-mt-3d-day 
#PBS -j oe
#PBS -m eab
#PBS -M burlen.loring@gmail.com

module use -a /u/bloring/modulefiles
module load PV3-3.8.1-R-IT
module load SVTK-PV3-3.8.1-R-IT

cd $PBS_O_WORKDIR

mpiexec MagnetosphereTopoBatch /u/bloring/ParaView/SciVisToolKit/development/runConfig/test4-3D-day.xml /nobackupp40/hkarimab/d40/test4/data/test4.bovm /nobackupp40/hkarimab/d40/test4/data/topo-day-3d/ day 32000 32000 

