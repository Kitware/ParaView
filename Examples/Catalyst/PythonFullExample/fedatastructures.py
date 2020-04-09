import numpy

class GridClass:
    """
    We are working with a uniform grid which will be
    represented as a vtkImageData in Catalyst. It is partitioned
    in the x-direction only.
    """
    def __init__(self, pointDimensions, spacing):
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        rank = comm.Get_rank()
        size = comm.Get_size()

        self.XStartPoint = int(pointDimensions[0]*rank/size)
        self.XEndPoint = int(pointDimensions[0]*(rank+1)/size)
        if rank+1 != size:
            self.XEndPoint += 1
        else:
            self.XEndPoint = pointDimensions[0]-1
        self.NumberOfYPoints = pointDimensions[1]
        self.NumberOfZPoints = pointDimensions[2]
        self.NumberOfGlobalXPoints = pointDimensions[0]

        self.PointDimensions = pointDimensions
        self.Spacing = spacing

    def GetNumberOfPoints(self):
        return (self.XEndPoint-self.XStartPoint+1)*self.PointDimensions[1]*self.PointDimensions[2]

    def GetNumberOfCells(self):
        return (self.XEndPoint-self.XStartPoint)*(self.PointDimensions[1]-1)*(self.PointDimensions[2]-1)

class AttributesClass:
    """
    We have velocity point data and pressure cell data.
    """
    def __init__(self, grid):
        self.Grid = grid

    def Update(self, time):
        self.Velocity = numpy.zeros((self.Grid.GetNumberOfPoints(), 3))
        self.Velocity = self.Velocity + time
        x = numpy.linspace(0, numpy.pi / 2, self.Grid.GetNumberOfCells());
        self.Pressure = numpy.sin(time * x / 20)
