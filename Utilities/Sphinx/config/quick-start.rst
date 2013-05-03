Quick-Start :  A Tutorial
=========================

Getting Started
---------------

To start interacting with the Server Manager, you have to load the "simple"
module. This module can be loaded from any python interpreter as long as the
necessary files are in PYTHONPATH. These files are the shared libraries located
in the paraview binary directory and python modules in the paraview directory:
paraview/simple.py, paraview/vtk.py etc. You can also use either pvpython (for
stand-alone or client/server execution), pvbatch (for non-interactive,
distributed batch processing) or the python shell invoked from Tools|Python
Shell using the ParaView client to execute Python scripts. You do not have to
set PYTHONPATH when using these.

This tutorial will be using the python integrated development environment IDLE.
PYTHONPATH is set to the following:

::

    /Users/berk/work/paraview3-build/lib:/Users/berk/work/paraview3-build/lib/site-packages

You may also need to set your path variable for searching for shared libraries
(i.e. PATH on Windows and LD_LIBRARY_PATH on Unix/Linux/Mac). The corresponding
LD_LIBRARY_PATH would be:

::

    /Users/berk/work/paraview3-build/lib (/Users/berk/work/paraview3-build/bin for versions before 3.98)

(Under WindowsXP for a debug build of paraview, set both PATH and PYTHONPATH
environment variables to include ${BUILD}/lib/Debug and
${BUILD}/lib/site-packages to make it work.)

When using a Mac to use the build tree in IDLE, start by loading the
servermanager module:

    >>> from paraview.simple import *

Importing the paraview module directly is deprecated, although still
possible for backwards compatibility. This document refers to the simple module
alone.

In this example, we will use ParaView in the stand-alone mode. Connecting to a
ParaView server running on a cluster is covered later in this document.

Tab-completion
~~~~~~~~~~~~~~

The Python shell in the ParaView Qt client provides auto-completion. One can
also use IDLE, for example to enable auto-completion. To use auto-completion in
pvpython, one can use the tips provided at TabCompletion_.

.. _TabCompletion: http://www.razorvine.net/blog/user/irmen/article/2004-11-22/17

In summary, you need to create a variable PYTHONSTARTUP as (in bash):

::

     export PYTHONSTARTUP = /home/<username>/.pythonrc

where .pythonrc is:

::

    # ~/.pythonrc
    # enable syntax completion
    try:
        import readline
    except ImportError:
        print "Module readline not available."
    else:
        import rlcompleter
        readline.parse_and_bind("tab: complete")

That is it. Tab completion works just as in any other shell.

Creating a Pipeline
-------------------

The simple module contains many functions to instantiate sources, filters, and
other related objects. You can get a list of objects this module can create from
ParaView's online help (from help menu or here:
http://paraview.org/OnlineHelpCurrent/)

Start by creating a Cone object:

    >>> cone = Cone()

You can get some documentation about the cone object using help().

    >>> help(cone)

Help on Cone in module paraview.servermanager object:

::

  class Cone(SourceProxy)
   |  The Cone source can be used to add a polygonal cone to the 3D scene. The output of the
   Cone source is polygonal data.
   |
   |  Method resolution order:
   |      Cone
   |      SourceProxy
   |      Proxy
   |      __builtin__.object
   |
   |  Methods defined here:
   |
   |  Initialize = aInitialize(self, connection=None)
   |
   |  ----------------------------------------------------------------------
   |  Data descriptors defined here:
   |
   |  Capping
   |      If this property is set to 1, the base of the cone will be capped with a filled polygon.
   Otherwise, the base of the cone will be open.
   |
   |  Center
   |      This property specifies the center of the cone.
   |
   |  Direction
   |      Set the orientation vector of the cone.  The vector does not have to be normalized.  The cone
   will point in the direction specified.
   |
   |  Height
   |      This property specifies the height of the cone.
   |
   |  Radius
   |      This property specifies the radius of the base of the cone.
   |
   |  Resolution
   |      This property indicates the number of divisions around the cone. The higher this number, the
   closer the polygonal approximation will come to representing a cone, and the more polygons it will
   contain.
   |
   | ...

This gives you a full list of properties. Check what the resolution property is set to:

    >>> cone.Resolution
    6

You can increase the resolution as shown below:

    >>> cone.Resolution = 32

Alternatively, we could have specified a value for resolution when creating the object:

    >>> cone = Cone(Resolution=32)

You can assign values to any number of properties during construction using
keyword arguments: You can also change the center.

    >>> cone.Center
    [0.0, 0.0, 0.0]
    >>> cone.Center = [1, 2, 3]

Vector properties such as this one support setting and retrieval of individual elements, as well as slices (ranges of elements):

    >>> cone.Center[0:2] = [2, 4]
    >>> cone.Center
    [2.0, 4.0, 3.0]

Next, apply a shrink filter to the cone:

    >>> shrinkFilter = Shrink(cone)
    >>> shrinkFilter.Input
    <paraview.servermanager.Cone object at 0xaf701f0>

At this point, if you are interested in getting some information about the
output of the shrink filter, you can force it to update (which will also cause
the execution of the cone source). For details about VTK's demand-driven
pipeline model used by ParaView, see one of the VTK books.

    >>> shrinkFilter.UpdatePipeline()
    >>> shrinkFilter.GetDataInformation().GetNumberOfCells()
    33L
    >>> shrinkFilter.GetDataInformation().GetNumberOfPoints()
    128L

We will cover the DataInformation class in more detail later.


Rendering
---------

Now that you've created a small pipeline, render the result. You will need two
objects to render the output of an algorithm in a scene: a representation and a
view. A representation is responsible for taking a data object and rendering it
in a view. A view is responsible for managing a render context and a collection
of representations. Simple creates a view by default. The representation object
is created automatically with Show().

    >>> Show(shrinkFilter)
    >>> Render()

In this example the value returned by Cone() and Shrink() was assigned
to Python variables and used to build the pipeline. ParaView keeps track of the
last pipeline object created by the user. This allows you to accomplish
everything you did above using the following code:

  >>> from paraview.simple import *
  # Create a cone and assign it as the active object
  >>> Cone()
  <paraview.servermanager.Cone object at 0x2910f0>
  # Set a property of the active object
  >>> SetProperties(Resolution=32)
  # Apply the shrink filter to the active object
  # Shrink is now active
  >>> Shrink()
  <paraview.servermanager.Shrink object at 0xaf64050>
  # Show shrink
  >>> Show()
  <paraview.servermanager.UnstructuredGridRepresentation object at 0xaf57f90>
  # Render the active view
  >>> Render()
  <paraview.servermanager.RenderView object at 0xaf57ff0>

This was a quick introduction to the paraview.simple module. In the following
sections, we will discuss the Python interface in more detail and introduce more
advanced concepts.
