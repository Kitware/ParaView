from paraview.simple import *
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkCommonTransforms import vtkTransform
from paraview.incubator import vtkPVIncubatorCAVEInteractionStyles


#
def create_interactor_style():
    """ Create and return a Python object

    ParaView depends on this method being present in your module and
    that when it is invoked, it returns an instance of the Python
    object implementing the HandleTracker, HandleButton, and
    HandleValuator methods.

    Be sure to define in your python module a method called
    "create_interactor_style()" which constructs and returns an
    instance of your custom style, otherwise the ParaView UI side will
    not know what to do with your python module.
    """
    return CustomInteractorStyle()


class CustomInteractorStyle(object):
    def __init__(self):
        self.flyAmount = 0.05

    def Initialize(self, vtkSelf):
        """Perform initialization on the C++ side proxy object.

        Intialize() is called by the C++ after it instantiates this
        object and passes it to the SetPythonObject() method.  This
        happens anytime you set the FileName property on the
        vtkSMVRPythonInteractorStyleProxy or else click the Refresh
        button in the proxy properties widget.  The Initialize()
        method is a good place to do things like add named roles,
        as in the example here, because afterward you can use the
        "Edit" button in the UI to define event bindings for those
        roles.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object.

        """
        vtkSelf.ClearAllRoles()
        vtkSelf.AddTrackerRole("Tracker")
        vtkSelf.AddButtonRole("Fly")
        vtkSelf.AddButtonRole("Reset")

    def HandleTracker(self, vtkSelf, role, sensor, matrix):
        """Handle a tracker event.

        Given the tracker event data in the method parameters, take
        some action based on that data.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object.
            role: The role you assigned to events of this type in
                your interactor style role bindings.  Named roles
                can be defined for the different event types in your
                Initialize() method.
            sensor: The numeric identifier of the sensor that produced
                this event.
            matrix: A 16-element array in row-major format containing
                the tracker matrix object.

        """
        print(f"HandleTracker() -> sensor: {sensor}, role: {role}")
        if role == "Tracker" and self.flying:
            rvProxy = GetActiveView()

            # Get the current matrix
            m_elts = rvProxy.GetProperty("ModelTransformMatrix")
            mat = vtkMatrix4x4()
            mat.DeepCopy(m_elts)

            # Perform some transformation and apply to the current matrix
            tform = vtkTransform()
            tform.Identity()
            tform.Translate(0, 0, self.flyAmount)
            mat.Multiply4x4(tform.GetMatrix(), mat, mat)

            # Store the updated matrix back on the renderview
            m_elts = [ mat.GetElement(i, j) for i in range(4) for j in range(4) ]

            rvProxy.SetPropertyWithName("ModelTransformMatrix", m_elts)
            rvProxy.UpdateVTKObjects()

    def HandleButton(self, vtkSelf, role, button, state):
        """Handle a button event.

        Given the button event data in the method parameters, take
        some action based on that data.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object.
            role: The role you assigned to events of this type in
                your interactor style role bindings.  Named roles
                can be defined for the different event types in your
                Initialize() method.
            button: The numeric identifier of the button that produced
                this event.
            state: The button state, 0 for up, 1 for down.
        """
        print(f"HandleButton() -> button: {button}, role: {role}")
        if role == "Fly":
            if state == 1:
                self.flying = True
            else:
                self.flying = False
        elif role == "Reset":
            rvProxy = GetActiveView()
            m_elts = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
            rvProxy.SetPropertyWithName("ModelTransformMatrix", m_elts)
            rvProxy.UpdateVTKObjects()

    def HandleValuator(self, vtkSelf, role, numChannels, channelData):
        """Handle an valuator event.

        Given the valuator event data in the method parameters, take
        some action based on that data.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object.
            role: The role you assigned to events of this type in
                your interactor style role bindings.  Named roles
                can be defined for the different event types in your
                Initialize() method.
            numChannels: The number of channels of valuator data present in
                channelData parameter (i.e. the length of the array)
            channelData: The array of valuator data values.

        """
        print(f"HandleValuator() -> number of channels: {numChannels}, role: {role}")
