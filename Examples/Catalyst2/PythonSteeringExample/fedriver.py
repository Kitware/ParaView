"""
A simple example of a Python simulation code working with ParaView Catalyst V2.
It depends on numpy and mpi4py being available. The environment
variables need to be set up properly to find Catalyst when running directly
from Python. For Linux
and Mac machines they should be:
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:<ParaView build dir>/lib
export PYTHONPATH=<ParaView build dir>/lib:<ParaView build dir>/lib/site-packages

Alternatively, pvbatch or pvpython can be used which will automatically set up
system paths for using ParaView Catalyst.

The location of the Catalyst Python wrapped libraries still need to be specified though:
export PYTHONPATH=<Catalyst build dir>/lib64/python<version>/site-packages:$PYTHONPATH

When running, Catalyst scripts must be added in on the command line. For example:
</path/to/pvpython> fedriver.py cpscript.py
mpirun -np 4 </path/to/pvbatch> --sym fedriver.py cpscript.py
"""
import numpy
from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

import fedatastructures

grid = fedatastructures.GridClass([30, 32, 34], [2.1, 2.2, 2.3])
attributes = fedatastructures.AttributesClass(grid)
doCoprocessing = True

if doCoprocessing:
    import catalyst_adaptor
    catalyst_adaptor.initialize()


results = None
for i in range(100):
    attributes.Update(i)
    if doCoprocessing:
        catalyst_adaptor.execute(i, i, grid, attributes,results)
        results = catalyst_adaptor.results()

if doCoprocessing:
    catalyst_adaptor.finalize()
