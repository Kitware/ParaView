from paraview.simple import *
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkCommonTransforms import vtkTransform
# The import below is needed so that references to vtkSelf in this
# module are recognized as instances of vtkSMVRInteratorStyleProxy.
from paraview.incubator import vtkPVIncubatorCAVEInteractionStyles
from paraview.incubator.pythoninteractorbase import PythonInteractorBase


def create_interactor_style():
    return SimpleScalingInteractor()


class SimpleScalingInteractor(PythonInteractorBase):
    """ This example custom Python interactor style allows updating the
    uniform scaling factor in the ModelTransformMatrix (aka the
    NavigationMatrix). It works best when your controller has a
    trackpad that has both valuator and button functionality, e.g.
    the HTC Vive controller. If you associate the "DoScale" button
    role with clicking the trackpad, and the "ScaleValue" valuator
    role with its numeric value, you can click above the midpoint
    to scale up by a factor of 2, or click below the midpoint to
    scale down by a factor of 2. """

    MIN_SCALE = 0.00001
    MAX_SCALE = 10000

    def __init__(self):
        self.restingValuator = 0.0
        self.value = 0.0
        self.scaleUpdated = False
        self.savedNavMatrix = vtkMatrix4x4()

    def Initialize(self, vtkSelf):
        vtkSelf.ClearAllRoles()
        vtkSelf.AddButtonRole("DoScale")
        vtkSelf.AddValuatorRole("ScaleValue")
        vtkSelf.AddButtonRole("Reset")

    def Update(self, vtkSelf):
        if self.scaleUpdated:
            self.savedNavMatrix.DeepCopy(vtkSelf.GetNavigationMatrix())

            # Get the current scale of the navigation matrix
            currentScale = vtkSelf.GetNavigationScale()

            # Uniform scaling only, pick any axis
            prevScale = currentScale[0]
            newScale = 1.0

            # Compute new scale from the existing one
            if self.value < 0:
                if prevScale > SimpleScalingInteractor.MIN_SCALE:
                    newScale = prevScale / 2
            else:
                if prevScale < SimpleScalingInteractor.MAX_SCALE:
                    newScale = prevScale * 2

            vtkSelf.SetNavigationScale([newScale, newScale, newScale])
            self.scaleUpdated = False

    def HandleButton(self, vtkSelf, role, button, state):
        if role == "Reset" and state == 1:
            self.currentScale = 1.0
            vtkSelf.SetNavigationMatrix(vtkMatrix4x4())
        if role == "DoScale" and state == 1 and self.value != self.restingValuator:
            self.scaleUpdated = True

    def HandleValuator(self, vtkSelf, numChannels, channelData):
        idx = vtkSelf.GetChannelIndexForValuatorRole("ScaleValue")
        self.value = channelData[idx]
