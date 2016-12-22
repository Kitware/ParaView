from paraview.servermanager import vtkProcessModule
pvoptions = vtkProcessModule.GetProcessModule().GetOptions()
if pvoptions.GetSymmetricMPIMode() == False:
    print("ERROR: Please run ParaView in SymmetricMPI mode.")
    import sys
    sys.exit(1)

from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

if rank == 0:
    data = { 'pi': 3.142 }
else:
    data = None

data = comm.bcast(data, root=0)
assert data["pi"] == 3.142
print("All's well that ends well!")
