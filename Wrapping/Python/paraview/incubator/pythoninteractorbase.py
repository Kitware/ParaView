from paraview.simple import *
from vtkmodules.vtkCommonMath import vtkMatrix4x4


def SetModelTransformMatrix(matrix):
    rvProxy = GetActiveView()
    invmat = vtkMatrix4x4()
    invmat.DeepCopy(matrix)
    invmat.Invert()

    mElts = [matrix.GetElement(i, j) for i in range(4) for j in range(4)]
    rvProxy.SetPropertyWithName("ModelTransformMatrix", mElts)

    mElts = [invmat.GetElement(i, j) for i in range(4) for j in range(4)]
    rvProxy.SetPropertyWithName("PhysicalToWorldMatrix", mElts)

    rvProxy.UpdateVTKObjects()


class PythonInteractorBase():
    def Initialize(self, vtkSelf):
        pass

    def HandleTracker(self, vtkSelf, role, sensor, matrix):
        pass

    def HandleButton(self, vtkSelf, role, button, state):
        pass

    def HandleValuator(self, vtkSelf, role, numChannels, channelData):
        pass
