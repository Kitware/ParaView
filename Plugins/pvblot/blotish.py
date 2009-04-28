r"""

The blotish module provides a set of commands that control batch ParaView
visualization using semantics similar to the blot program.  Those already
familiar with blot should have no trouble understanding scripts written
using this module.  However, when writing scripts with the blotish module
should understand some very important differences.

The major difference between using blotish and blot is that blotish is
using the Python interpreter, which has a significantly different syntax
than the blot interpreter.  For starters, Python commands wrap their
arguments in parenthesis and separate the arguments with commas.  Thus, the
detour command must be called as "detour()" and a 20 degree rotation around
the x axis must be called as "rotate(x, 20)".  Another difference is that
Python is case sensitive.  The commands are implemented in lower case
because that seems to be the most common.  Thus, "detour()" works but
"DETOUR()" does not.  Python also does not support partial commands; you
have to type in the entire command.  Thus, "det()" will not work in place
of "detour()" Some command have aliases for common abbreviations.  For
example, a "pl()" command exists to use in replacement for "plot()", but
"plo()" will still not work.  A final difference is that Python is more
picky about defining identifiers before using them.  The upshot is that
identifiers that refer to dynamic things like variable names must be passed
as strings.  For example, to paint the variable "Temp", the command is
executed like "paint('Temp')".

On the flip side, the Python interpreter provides many helpful features
such as flow control structures and mathematical operations.  You also have
the "help" command that can provide documentation on any module or command.

To start using the blotish commands, first load the blotish module and then
start running commands.  Note that at some point you will have to specify a
filename.  This can be done as an argument to detour.

  from blotish import *

  detour('/path/to/exodus/file')
  plot()

"""

#==============================================================================
#
#  Program:   ParaView
#  Module:    blotish.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================

#-------------------------------------------------------------------------
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
#-------------------------------------------------------------------------


# Import myself so that if the user called "from blotish import *",
# help(blotish) still works.
import blotish

import paraview.simple
import math

# An class that can be caught the signifies a blotish parameter error.
class BlotishError(ValueError): pass

_standard_colors = [
    [ 0.933, 0.000, 0.000],     # Red
    [ 0.000, 0.804, 0.804 ],    # Cyan
    [ 0.000, 1.000, 0.000 ],    # Green
    [ 0.804, 0.804, 0.000 ],    # Yellow
    [ 0.647, 0.165, 0.165 ],    # Brown
    [ 0.804, 0.569, 0.620 ],    # Pink
    [ 0.576, 0.439, 0.859 ],    # Purple
    [ 0.804, 0.522, 0.000 ],    # Orange
    ]

def _find_case_insensitive(str, lst):
    """
    Finds string str in list of strings lst ignoring case.  If the string is
    found, returns the actual value of the string.  Otherwise, returns None.
    """
    strUpper = str.upper()
    for entry in lst:
        if entry.upper() == strUpper:
            return entry
    return None

class _State(object):
    "This class holds state variables for blotish including PV objects."

    filename = None
    numBlocks = 1
    numBlockColors = len(_standard_colors)
    numSpectrumColors = 5
    availableTimes = []
    plotTimes = []
    mlines = 1
    msurface = 1
    autoplot = True

    _pipelineObjects = {}
    def _getPipelineObject(self, name, createFunc, **newObjectOptions):
        if not self._pipelineObjects.has_key(name):
            self._pipelineObjects[name] = createFunc(**newObjectOptions)
            paraview.simple.Hide(self._pipelineObjects[name])
        return self._pipelineObjects[name]

    def hasPipelineObject(self, name):
        return self._pipelineObjects.has_key(name)

    _reader = None
    def _getReader(self):
        if not self.filename:
            raise BlotishError, "No filename specified."
        return self._getPipelineObject('reader', paraview.simple.ExodusIIReader,
                                       FileName=self.filename)
    reader = property(_getReader, doc="ParaView Exodus reader pipeline object.")

    _surface = None
    def _getSurface(self):
        return self._getPipelineObject('surface',
                                       paraview.simple.ExtractSurface,
                                       Input=self.reader)
    surface = property(_getSurface,
                       doc="ParaView surface extract pipeline object.")

    _glyph = None
    def _getGlyph(self):
        return self._getPipelineObject('glyph', paraview.simple.Glyph,
                                       Input=self.surface)
    glyph = property(_getGlyph, doc="ParaView glyph pipeline object.")

    _contour = None
    def _getContour(self):
        return self._getPipelineObject('contour', paraview.simple.Contour,
                                       Input=self.reader)
    contour = property(_getContour, doc="ParaView contour pipeline object.")

    def resetPipeline(self):
        for obj in self._pipelineObjects.values():
            if obj:
                paraview.simple.Hide(obj)
                paraview.simple.SetDisplayProperties(obj, Opacity=1)

    # The last variable selected.
    currentVariable = None
    # The type of the last variable selected ('POINT_DATA' or 'CELL_DATA')
    currentVariableType = None

    def loadVariable(self, name):
        """
        Finds the variable with name (case insensitive) and makes sure that
        the variable is loaded in the reader.  If that variable exists (and
        is loaded), then the currentVariable and currentVariableType fields
        of this object are set.  If the variable is not found, an exception is
        raised.
        """
        reader = self.reader
        realname = _find_case_insensitive(name, reader.PointVariables.Available)
        if realname:
            vars = reader.PointVariables[:]
            if vars.count(realname) == 0:
                vars.append(realname)
                reader.PointVariables = vars
            self.currentVariable = realname
            self.currentVariableType = 'POINT_DATA'
            return
        realname = _find_case_insensitive(name, reader.ElementVariables.Available)

        if realname:
            vars = reader.ElementVariables[:]
            if vars.count(realname) == 0:
                vars.append(realname)
                reader.ElementVariables = vars
            self.currentVariable = realname
            self.currentVariableType = 'CELL_DATA'
            return

        raise BlotishError, "No such variable " + name

    def _getCurrentVariableInfo(self):
        if not self.currentVariable:
            raise BlotishError, "No variable selected."
        if self.currentVariableType == 'POINT_DATA':
            return self.reader[0].PointData[self.currentVariable]
        else:   # Must be CELL_DATA
            return self.reader[0].CellData[self.currentVariable]
    currentVariableInfo = property(_getCurrentVariableInfo)

    # The handling of the lookup table may change dramatically as
    # paraview.simple is improved.
    _blockLookupTable = None
    def _getBlockLookupTable(self):
        if not self._blockLookupTable:
            self._blockLookupTable = paraview.servermanager.rendering.PVLookupTable()
            paraview.servermanager.Register(self._blockLookupTable)
            self._blockLookupTable.ColorSpace = "RGB"
        lt = self._blockLookupTable
        if len(lt.RGBPoints)/4 != self.numBlocks:
            self._rebuildBlockLookupTable(lt)
        return lt
    blockLookupTable = property(_getBlockLookupTable,
                                doc="Lookup table used for coloring blocks.")

    def rebuildBlockLookupTable(self):
        self._rebuildBlockLookupTable(self.blockLookupTable)
    def _rebuildBlockLookupTable(self, lt):
        # Add RGB points to lookup table.  These are tuples of 4 values.  First
        # one is the scalar values, the other 3 the RGB values.
        rgbpoints = []
        for i in xrange(self.numBlocks):
            rgbpoints.append(i + 1)
            rgbpoints.extend(_standard_colors[(i)%self.numBlockColors])
        lt.RGBPoints = rgbpoints

    _spectrumLookupTable = None
    _spectrumLookupTableVariable = ''
    def _getSpectrumLookupTable(self):
        if not self._spectrumLookupTable:
            self._spectrumLookupTable = paraview.servermanager.rendering.PVLookupTable()
            paraview.servermanager.Register(self._spectrumLookupTable)
            self._spectrumLookupTable.ColorSpace = "HSV"
        lt = self._spectrumLookupTable
        if len(lt.RGBPoints)/8 != self.numSpectrumColors+1 or \
           self._spectrumLookupTableVariable != self.currentVariable:
            self._rebuildSpectrumLookupTable(lt)
        return lt
    spectrumLookupTable = property(_getSpectrumLookupTable,
                                   doc="Lookup table used for contours and painting.")

    def rebuildSpectrumLookupTable(self):
        self._rebuildSpectrumLookupTable(self.spectrumLookupTable)
    def _rebuildSpectrumLookupTable(self, lt):
        import paraview.vtk
        # Find a good range for the data.
        range = self.currentVariableInfo.GetRange()
        if self.currentVariableInfo.GetNumberOfComponents() > 1:
            diag = 0
            for i in xrange(self.currentVariableInfo.GetNumberOfComponents()):
                range = self.currentVariableInfo.GetRange(i)
                largest = max(abs(range[0]), range[1])
                diag = diag + largest*largest
            range = [0, math.sqrt(diag)]
        hueStep = (2.0/3.0)/(self.numSpectrumColors)
        rangeStep = (range[1]-range[0])/(self.numSpectrumColors+1)
        # Add RGB points to lookup table.  These are tuples of 4 values.  First
        # one is the scalar values, the other 3 the RGB values.
        rgbpoints = []
        for i in xrange(self.numSpectrumColors+1):
            # Add two colors for a constant range.
            color = paraview.vtk.vtkMath.HSVToRGB(2.0/3.0 - i*hueStep, 1, 1)
            rgbpoints.append(range[0]+i*rangeStep)
            rgbpoints.extend(color)
            rgbpoints.append(range[0]+i*rangeStep+0.9999*rangeStep)
            rgbpoints.extend(color)
        lt.RGBPoints = rgbpoints

    def _getSpectrumScalarValues(self):
        return self.spectrumLookupTable.RGBPoints[8::8]
    spectrumScalarValues = property(_getSpectrumScalarValues,
                                    doc="The scalar values associated with each spectrum contour.")

global state
state = _State()

on = "on"
off = "off"

def _init_blotish(filename):
    """
    This command initializes the blotish commands.  Think of this as the
    super-command that must be run before all other sub-commands.

    This function takes a single string argument that is the path to the
    exodus file to load.
    """

    if state.filename:
        return
    elif not filename:
        raise BlotishError, "You need to specify a filename."

    state.filename = filename

    # This will automatically load the reader
    reader = state.reader

    if not reader:
        state.filename = None
        print "Failed to load file", filename

    #paraview.simple.UpdatePipeline()
    datainfo = reader[0].GetDataInformation()
    state.numBlocks = datainfo.GetNumberOfDataSets()

    print "Database:", reader.FileName
    print
    print "Number of nodes          =", datainfo.GetNumberOfPoints()
    print "Number of elements       =", datainfo.GetNumberOfCells()
    print "Number of element blocks =", state.numBlocks
    print
    print "Number of node sets      =", len(reader.NodeSetArrayStatus.Available)
    print "Number of side sets      =", len(reader.SideSetArrayStatus.Available)
    print
    print "Variable Names"
    print "Global: ",
    for name in reader.GlobalVariables.Available:
        print " ", name,
    print
    print "Nodal:  ",
    for name in reader.PointVariables.Available:
        print " ", name,
    print
    print "Element:",
    for name in reader.ElementVariables.Available:
        print " ", name,
    print
    print "Number of time steps     =", len(reader.TimestepValues)
    if len(reader.TimestepValues) > 0:
        print "   Minimum time          =", min(reader.TimestepValues)
        print "   Maximum time          =", max(reader.TimestepValues)
        state.availableTimes = reader.TimestepValues
    else:
        state.availableTimes = [0]
    state.plotTimes = state.availableTimes

def _finish_plot_change():
    if state.autoplot:
        paraview.simple.Render()

def detour(filename=None):
    """
    Start a subprogram to plot deformed meshes.  If a filename has not been
    specified earlier, you can specify one here.
    """
    _init_blotish(filename)
    wirefram()

def color(ncol=None):
    """
    Set the maximum number of standard color scale colors to use on a color
    plot to ncol.
    """
    maxColors = len(_standard_colors)
    if not ncol:
        print "Set to", state.numBlockColors, "of", maxColors, "colors."
        return

    ncol = int(ncol)
    if ncol > maxColors:
        print "Setting to maximum of", maxColors, "instead of", ncol
        ncol = maxColors

    state.numBlockColors = ncol
    state.rebuildBlockLookupTable()
    _finish_plot_change()

def spectrum(ncol=5):
    """Set the number of contours to include in a paint or contour plot."""
    state.numSpectrumColors = int(ncol)
    state.rebuildSpectrumLookupTable()

    # Update the isosurfaces in the contour filter.
    if (state.hasPipelineObject('contour')):
        contour = state.contour
        contour.Isosurfaces = state.spectrumScalarValues

    _finish_plot_change()

    print "Contour locations", state.spectrumScalarValues

def _updateMeshRender():
    """Sets up the rendering of mesh based on the mlines and msurface state
    flags.  Returns the pipeline object that is being drawn so that further
    parameters (such as coloring) may be set up."""
    todraw = state.reader
    if state.mlines:
        paraview.simple.Show(todraw)
        if state.msurface:
            paraview.simple.SetDisplayProperties(
                                     todraw, Representation="Surface With Edges")
        else:
            paraview.simple.SetDisplayProperties(todraw,
                                                 Representation="Wireframe")
    else:
        if state.msurface:
            paraview.simple.Show(todraw)
            paraview.simple.SetDisplayProperties(todraw,
                                                 Representation="Surface")
        else:
            paraview.simple.Hide(todraw)
    return todraw

def mlines(mlines_flag):
    """
    Turns on/off the grid lines for block elements.  This method takes
    exactly one argument.  If it is on, then the wireframe grid is drawn.
    If it is off, the wireframe grid is not drawn.
    """
    if isinstance(mlines_flag, str):
        mlines_flag = mlines_flag.lower()
        if mlines_flag == on:
            mlines_flag = 1
        elif mlines_flag == off:
            mlines_flag = 0
        else:
            raise BlotishError, ("Unknown mlines flag: " + mlines_flag)
    state.mlines = mlines_flag
    _updateMeshRender()
    _finish_plot_change()

def wire():
    "Alias for wireframe command."
    wireframe()
def wirefram():
    "Alias for wireframe command."
    wireframe()
def wireframe():
    "Sets the view to wireframe mesh mode, which displays the mesh lines."
    state.resetPipeline()
    state.mlines = 1
    state.msurface = 0
    todraw = _updateMeshRender()
    paraview.simple.SetDisplayProperties(todraw,
                                         ColorAttributeType="CELL_DATA",
                                         ColorArrayName="ObjectId",
                                         LookupTable=state.blockLookupTable)
    _finish_plot_change()

def solid(mlines_flag=on):
    """
    Sets the view to solid mesh mode, which paints each element using a
    different color for each element block.

    The optional argument specifies weather you want to show mesh lines
    (by default they are shown).
    """
    state.resetPipeline()
    state.msurface = 1
    mlines(mlines_flag)
    todraw = _updateMeshRender()
    paraview.simple.SetDisplayProperties(todraw,
                                         ColorAttributeType="CELL_DATA",
                                         ColorArrayName="ObjectId",
                                         LookupTable=state.blockLookupTable)
    _finish_plot_change()

def paint(variable=None):
    """Sets the view to paint contour mode, which paints contours of the
    given variable on the mesh.  The variable may be either nodal or element.
    If no variable is specified, the last selected variable is used."""
    if variable:
        state.loadVariable(variable)
    if not state.currentVariable:
        raise BlotishError, "No variable selected."

    state.resetPipeline()
    state.mlines = 0
    state.msurface = 1
    todraw = _updateMeshRender()
    paraview.simple.SetDisplayProperties(todraw,
                                         ColorAttributeType=state.currentVariableType,
                                         ColorArrayName=state.currentVariable,
                                         LookupTable=state.spectrumLookupTable)
    _finish_plot_change()

def contour(variable=None):
    """Sets the view to contour mode, which plots the line or surface contours
    where the given variable is equal to one of the contour values (set with
    the spectrum command)."""
    if variable:
        state.loadVariable(variable)
    if state.currentVariableType != 'POINT_DATA':
        raise BlotishError, "Only nodal variables supported by contour right now."
    info = state.currentVariableInfo
    if (info.GetNumberOfComponents() > 1):
        raise BlotishError, state.currentVariable + " is not a scalar."

    state.resetPipeline()
    state.mlines = 0
    state.msurface = 0
    _updateMeshRender()
    contour = state.contour
    contour.ContourBy = state.currentVariable
    contour.Isosurfaces = state.spectrumScalarValues
    paraview.simple.Show(contour)
    paraview.simple.SetDisplayProperties(contour,
                                         Representation="Surface",
                                         ColorAttributeType=state.currentVariableType,
                                         ColorArrayName=state.currentVariable,
                                         LookupTable=state.spectrumLookupTable)
    _finish_plot_change()

def vector(variable=None):
    """
    Sets the view to vector mode.  Pass the name of the variable as the
    argument, or pass nothing to use the last used variable.
    """
    if variable:
        state.loadVariable(variable)
    if state.currentVariableType != 'POINT_DATA':
        raise BlotishError, "Only nodal variables supported by vector right now."
    info = state.currentVariableInfo
    if (info.GetNumberOfComponents() < 2):
        raise BlotishError, state.currentVariable + " is not a vector."
    glyph = state.glyph
    glyph.Vectors = state.currentVariable
    glyph.Orient = 1
    glyph.ScaleMode = 'vector'

    #Determine scaling factor for vector.
    bounds = glyph[0].GetDataInformation().GetBounds()
    scalefactor = max(bounds[1]-bounds[0], bounds[3]-bounds[2],
                      bounds[5]-bounds[4])
    scalefactor = scalefactor/20
    divisor = 0
    for i in xrange(info.GetNumberOfComponents()):
        range = info.GetRange(i)
        divisor = max(divisor, abs(range[0]), abs(range[1]))
    if divisor > 1:
        scalefactor = scalefactor/divisor
    glyph.SetScaleFactor = scalefactor

    state.resetPipeline()
    # Show the glyphs.
    paraview.simple.Show(glyph)
    # Show a transparent surface.
    state.mlines = 0
    state.msurface = 1
    surface = _updateMeshRender()
    paraview.simple.SetDisplayProperties(surface,
                                         ColorAttributeType="CELL_DATA",
                                         ColorArrayName="ObjectId",
                                         LookupTable=state.blockLookupTable,
                                         Opacity=0.5)
    _finish_plot_change()

def pl():
    "Alias for plot command."
    plot()
def plot():
    "Generates the current plot."
    import sys
    prompt = sys.stdin.isatty()
    for t in state.plotTimes:
        paraview.simple.GetActiveView().ViewTime = t
        paraview.simple.Render()
        if prompt and (not t == state.plotTimes[len(state.plotTimes)-1]):
            print "Time", t,
            print "  Enter 'C' to complete, 'Q' to quit. ",
            result = sys.stdin.readline()
            result = result.lower()
            if result.startswith('q'):
                break
            elif result.startswith('c'):
                prompt = False
        else:
            print "Time", t

def autoplot(flag):
    """    
    Turn autoplot on or off.  The flag should be 0 (off) or 1 (on).  When
    autoplot is on, then a command that changes the state of the plot
    automatically renders the last time step with the new features.
    """
    if isinstance(flag, str):
        flag = int(flag)
    if (flag):
        state.autoplot = True
    else:
        state.autoplot = False

x = 'x'
y = 'y'
z = 'z'
elevation = 'elevation'
azimuth = 'azimuth'
roll = 'roll'

def rot(*rotations):
    "Alias for rotate command."
    rotate(*rotations)
def rotate(*rotations):
    """
    Rotates the 3D mesh.  Each (axis, ndeg) parameter pair specifies an
    axis of rotation (x, y, or z) or (elevation, azimuth, or roll) and the
    number of degrees to rotate.  The axes have the following meanings:

      * x or elevation: rotate the camera around the focal point in the
        horizontal direction.

      * y or azimuth: rotate the camera around the focal point in the vertical
        direction.

      * z or roll: roll the camera about the view direction.

    The identifiers elevation, azimuth, and roll can be abbreviated with any
    unique prefix.
    """
    camera = paraview.simple.GetActiveCamera()
    # Some manging to rotation into list of pairs.
    rpairs = zip(rotations[::2], rotations[1::2])
    for axis, degrees in rpairs:
        axis = axis.lower()
        degrees = float(degrees)
        if axis == x or elevation.startswith(axis):
            sign = degrees/abs(degrees)
            degrees = abs(degrees)
            while degrees > 0:
                d = min(45, degrees)
                degrees = degrees - d
                camera.Elevation(sign*d)
                camera.OrthogonalizeViewUp()
        elif axis == y or azimuth.startswith(axis):
            camera.Azimuth(-degrees)
        elif axis == z or roll.startswith(axis):
            camera.Roll(degrees)
        else:
            print "Unknown axis: ", axis
    _finish_plot_change()

each = 'each'
mesh = 'mesh'
reset = 'reset'
def zoom(factor):
    """
    Zooms the view by the given factor.  Factors bigger than 1 make the
    objects larger (e.g. 2 makes it look twice as big) and factors smaller
    than 1 make the objects look smaller (e.g. 0.5 makes it look twice as
    small).

    If factor is one of the special identifiers reset, each, or mesh then
    the view is reset to fit the geometry in the view.

    Note that this zoom is specified differently than the original blot
    program.  Also, the translat parameter is not supported.  Use the
    translate command instead.
    """
    camera = paraview.simple.GetActiveCamera()
    if isinstance(factor, str):
        factor = factor.lower()
        if (factor == each) or (factor == mesh) or (factor == reset):
            paraview.simple.ResetCamera()
            _finish_plot_change()
            return
        factor = float(factor)
    camera.Dolly(factor)
    _finish_plot_change()

def trans(x, y):
    "Alias for translate."
    translate(x, y)
def translat(x, y):
    "Alias for translate."
    translate(x, y)
def translate(x, y):
    """
    Translate the view in the x and y directions.  The x argument pans
    horizontally, the y vertically.  The values are normalized so that 1.0
    corresponds to (roughly) one screen width.
    """
    x = float(x)
    y = float(y)
    from paraview.vtk import vtkMath
    camera = paraview.simple.GetActiveCamera()
    # Get basis vectors for the x and y translation
    xbasis = [0, 0, 0]
    ybasis = [0, 0, 0]
    vtkMath.Cross(camera.GetViewUp(), camera.GetDirectionOfProjection(), xbasis)
    vtkMath.Cross(xbasis, camera.GetDirectionOfProjection(), ybasis)
    vtkMath.Normalize(xbasis)
    vtkMath.Normalize(ybasis)
    # Scale the basis functions by the size of the viewable region.
    cameradist = 0
    for fpv, posv in zip(camera.GetFocalPoint(), camera.GetPosition()):
        cameradist = cameradist + (fpv-posv)*(fpv-posv)
    cameradist = math.sqrt(cameradist)
    height = cameradist*math.tan(math.radians(camera.GetViewAngle()))
    xbasis = map(lambda a: a*height, xbasis)
    ybasis = map(lambda a: a*height, ybasis)
    # Compute the translation vector
    translation = map(lambda xb, yb: xb*x + yb*y, xbasis, ybasis)
    # Translate focal point and position
    fp = camera.GetFocalPoint()
    fp = map(lambda a, b: a + b, fp, translation)
    camera.SetFocalPoint(fp)
    pos = camera.GetFocalPoint()
    pos = map(lambda a, b: a + b, pos, translation)
    camera.SetFocalPoint(pos)

    _finish_plot_change()
