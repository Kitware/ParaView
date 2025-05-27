from paraview.simple import *
from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkCommonTransforms import vtkTransform
# The import below is needed so that references to vtkSelf in this
# module are recognized as instances of vtkSMVRInteratorStyleProxy.
from paraview.incubator import vtkPVIncubatorCAVEInteractionStyles
from paraview.incubator.pythoninteractorbase import PythonInteractorBase

# This module contains an example customizable python interactor style
# for use in CAVEs. The SimplyFlyer class requires a tracker device
# with at least one button and one valuator. It allows you to fly in
# the direction you are pointing your controller, and to control your
# speed using the valuator. You can also return to the origin, or
# "reset navigation" using the button. There is a wide variety of
# devices out there, so to allow you to hold your particular device
# in the most ergonomic orientation relative to your hand, you can
# choose which column of the tracker matrix corresponds to your
# desired flight direction by setting the "flyDirectionColumn" class
# member.

def create_interactor_style():
    """ Create and return a Python object

    ParaView depends on this method being present in your module and
    that when it is invoked, it returns an instance of the Python
    object implementing the HandleTracker, HandleButton, and/or
    HandleValuator methods.

    Be sure to define in your python module a method called
    "create_interactor_style()" which constructs and returns an
    instance of your custom style, otherwise the ParaView UI side will
    not know what to do with your python module.
    """
    return SimpleFlyer()


class SimpleFlyer(PythonInteractorBase):
    def __init__(self):
        """Initialize the python object.

        This constructor is optional, but useful for initializing internal
        variables.
        """
        # Which column from tracker matrix determines flight direction
        self.flyDirectionColumn = 2    # Hint: 0, 1, or 2

        # Initialize some internal state variables
        self.currentSpeed = 0.0
        self.speedUpdated = False
        self.savedMatrix = vtkMatrix4x4()
        self.transform = vtkTransform()

    def Initialize(self, vtkSelf):
        """Perform initialization on the C++ side proxy object.

        Intialize() is called by the C++ after it instantiates this
        object and passes it to the SetPythonObject() method. This
        happens anytime you set the FileName property on the
        vtkSMVRPythonInteractorStyleProxy or else click the Refresh
        button in the proxy properties widget. The Initialize()
        method is where you should do anything you would do in the
        constructor of a C++ interactor style, like adding named
        roles.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object. This is an instance of
                vtkSMVRPythonInteractorStyle, and as such, can be used
                to call any methods common to all interactor style
                proxies (instances of vtkSMVRInteractorStyleProxy).

        """
        # Removes any previous role bindings so the state is clean.
        vtkSelf.ClearAllRoles()

        # For this custom interactor style, we need the following roles:

        # a tracker for orientation/position matrix events
        vtkSelf.AddTrackerRole("Controller")

        # a valuator so we can control our speed
        vtkSelf.AddValuatorRole("FlySpeed")

        # a button we can hit to undo any navigation (return to origin)
        vtkSelf.AddButtonRole("Reset")

    def HandleTracker(self, vtkSelf, role, sensor, matrix):
        """Handle a tracker event.

        Given the tracker event data in the method parameters, take
        some action based on that data.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object. This is an instance of
                vtkSMVRPythonInteractorStyle, and as such, can be used
                to call any methods common to all interactor style
                proxies (instances of vtkSMVRInteractorStyleProxy).
            role: The role you assigned to events of this type in
                your interactor style role bindings. Named roles
                can be defined for the different event types in your
                Initialize() method. The UI then prompts you to
                associate specific events for each named role.
            sensor: The numeric identifier of the sensor that produced
                this event.
            matrix: A 16-element array in row-major format containing
                the tracker matrix object.

        """
        if role == "Controller" and self.speedUpdated:
            # Pull out the column we use for flight direction
            direction = [
                matrix[self.flyDirectionColumn],
                matrix[self.flyDirectionColumn + 4],
                matrix[self.flyDirectionColumn + 8],
                matrix[self.flyDirectionColumn + 12]
            ]

            # Translate in the direction the controller is pointed, scaled by the speed
            self.transform.Identity()
            self.transform.Translate(
                self.currentSpeed * direction[0],
                self.currentSpeed * direction[1],
                self.currentSpeed * direction[2]
            )

            # Get the current navigation matrix, transform it with the translation
            # matrix computed above, and then update the navigation matrix.
            self.savedMatrix.DeepCopy(vtkSelf.GetNavigationMatrix())
            vtkMatrix4x4.Multiply4x4(
                self.transform.GetMatrix(), self.savedMatrix, self.savedMatrix)
            vtkSelf.SetNavigationMatrix(self.savedMatrix)

            # This is required to apply the change to the NavigationMatrix
            rvProxy = GetActiveView()
            rvProxy.UpdateVTKObjects()

            # Avoid unnecessary updates when flying by valuator
            self.speedUpdated = False

    def HandleButton(self, vtkSelf, role, button, state):
        """Handle a button event.

        Given the button event data in the method parameters, take
        some action based on that data.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object. This is an instance of
                vtkSMVRPythonInteractorStyle, and as such, can be used
                to call any methods common to all interactor style
                proxies (instances of vtkSMVRInteractorStyleProxy).
            role: The role you assigned to events of this type in
                your interactor style role bindings.  Named roles
                can be defined for the different event types in your
                Initialize() method. The UI then prompts you to
                associate specific events for each named role.
            button: The numeric identifier of the button that produced
                this event.
            state: The button state, 0 for up, 1 for down.
        """
        if role == "Reset" and state == 1:
            # When we press the button we associated with the "Reset" role, we
            # set the navigation matrix back to identity, returning us to the
            # origin.
            rvProxy = GetActiveView()
            navMat = vtkMatrix4x4()
            navMat.Identity()
            vtkSelf.SetNavigationMatrix(navMat)
            rvProxy.UpdateVTKObjects()

    def HandleValuator(self, vtkSelf, numChannels, channelData):
        """Handle an valuator event.

        Given the valuator event data in the method parameters, take
        some action based on that data.

        Args:

            vtkSelf: A reference to the C++ proxy object holding
                this Python object. This is an instance of
                vtkSMVRPythonInteractorStyle, and as such, can be used
                to call any methods common to all interactor style
                proxies (instances of vtkSMVRInteractorStyleProxy).
            numChannels: The number of channels of valuator data present in
                channelData parameter (i.e. the length of the array)
            channelData: The array of valuator data values.

        """
        fsIdx = vtkSelf.GetChannelIndexForValuatorRole("FlySpeed")
        self.currentSpeed = channelData[fsIdx]
        self.speedUpdated = True
