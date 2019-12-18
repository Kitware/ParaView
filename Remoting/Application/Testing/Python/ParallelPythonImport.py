# test to make sure that we can import paraview when running in parallel
# from a regular Python shell. Note that PYTHONPATH needs to be set
# to know where to find ParaView.

import sys
print('sys.path = %s' % sys.path)
from mpi4py import MPI


print('proc %d' % MPI.COMM_WORLD.Get_rank())

import paraview

paraview.options.batch = True
paraview.options.symmetric = True

print('about to import paraview.simple %d' % MPI.COMM_WORLD.Get_rank())
import paraview.simple

print('finished %d' %  MPI.COMM_WORLD.Get_rank())
