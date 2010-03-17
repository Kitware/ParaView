"""

The blotish module provides a set of commands that control batch ParaView
visualization using semantics similar to the blot program.

Instead of calling the public methods of blotish directly this module is
driven by pvblot.  The pvblot module creates an interpreter that translates
blot syntax to botish method calls.  See pvblot.

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

import paraview.simple
import async_io_helper
import timestep_selection
import number_list_parser
import blot_common
import tplot as tplot_mod
import math
import re

# Global variable state is used by methods in this module
# After _init_blotish is called state is an instance of
# the class _State.
state = None

# The exception class that is raised by this module
BlotishError = blot_common.Error

# Some constants
NODE_VARIABLE = blot_common.NODE_VARIABLE
ELEMENT_VARIABLE = blot_common.ELEMENT_VARIABLE
GLOBAL_VARIABLE = blot_common.GLOBAL_VARIABLE
EXODUS = blot_common.EXODUS
SPYPLOT = blot_common.SPYPLOT

STANDARD_COLORS = blot_common.STANDARD_COLORS


def _subprogram(names=[]):
    """A decorator factor that can be applied to methods to signal that the
    method requires a specifc subprogram or subprograms.  The argument to this
    factor method can be a single string to require a specific subpgram,
    a list of strings to require one of a set of subprograms, or if no argument
    is provided than the method is valid if any subprogram is active."""
    def decorator_func(func):
        if isinstance(names, str): programs = [names]
        else: programs = list(names)
        def wrapper(*args, **kwargs):
            if not state.subProgram \
                or (state.subProgram and programs
                    and state.subProgram not in programs):
                if programs:
                    if len(programs) > 1: plural = "s"
                    else: plural = ""
                    msg = "This command is only available in the subprogram%s: %s" % (plural,
                                                                         ", ".join(programs))
                else:
                    msg = "Expected a subprogram name"
                raise BlotishError(msg)
            func(*args, **kwargs)
        wrapper.__name__ = func.__name__
        wrapper.__doc__ = func.__doc__
        return wrapper
    return decorator_func


class _State(object):
    "This class holds state variables for blotish including PV objects."

    def __init__(self):
        self.filename = None
        self.numBlocks = 1
        self.numBlockColors = len(STANDARD_COLORS)
        self.numSpectrumColors = 5
        self.filename_counter = 0
        self.mlines = 1
        self.msurface = 1
        self.tplot = None
        self.autoplot = True
        self.subProgram = None
        self.ioHelper = async_io_helper.new_helper()
        self.interactive = True
        self.diskwrite = False
        self.time_selection = None # will be set to an instance of timestep_selection

        # set some default values acceptable for the ExodusIIReader
        # these may be adjusted for the SpyPlotReader
        self._block_id_variable = "ObjectId"
        self._block_id_field = "CELL_DATA"
        self._block_id_offset = 1

    def __del__(self):

        # Remove tplot first
        del self.tplot

        # Sometimes paraview is already unloaded at this point.
        if not paraview: return

        # Remove pipeline objects
        for name in self._pipelineObjects.keys():
            source = self._pipelineObjects.pop(name)
            paraview.simple.Delete(source)

        # Finally remove the reader
        paraview.simple.Delete(self.reader)

    def _getPrompt(self):
        if self.subProgram == "detour": return "DETOUR> "
        if self.subProgram == "tplot": return "TPLOT> "
        if self.subProgram == "splot": return "SPLOT> "
        else: return "BLOT> "
    prompt = property(_getPrompt, doc="Return the prompt for the active subprogram.")

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
            raise BlotishError("No filename specified.")
        if not self._reader:
            # matches patterns that end with "spcth" or spcth.<num>
            if paraview.servermanager.vtkSMReaderFactory.CanReadFile(\
              self.filename, "sources", "spcthreader",\
              paraview.servermanager.ActiveConnection.ID):
                # For the spy plot reader we will turn on all variables
                # and then apply the Extract CTH Parts filter to give us
                # something to work with
                r = paraview.simple.SpyPlotReader(FileName=self.filename)
                r.CellArrays = r.CellArrays.Available
                extract = paraview.simple.ExtractCTHParts()
                material_vars = []
                for var_name in r.CellArrays:
                    if var_name.startswith("Material volume fraction"):
                        material_vars.append(var_name)

                extract.UnsignedCharacterVolumeArrays = material_vars

                extract.add_attribute("DatabaseType", SPYPLOT)
                extract.add_attribute("FileName", r.FileName)
                extract.add_attribute("TimestepValues", r.TimestepValues)

                self._reader = extract
                self._block_id_variable = "Part Index"
                self._block_id_field = "POINT_DATA"
                self._block_id_offset = 0
                
            else:
                self._reader = paraview.simple.ExodusIIReader(FileName=self.filename)
                self._reader.add_attribute("DatabaseType", EXODUS)

        return self._reader

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
    # The type of the last variable selected (NODE_VARIABLE, ELEMENT_VARIABLE, GLOBAL_VARIABLE)
    currentVariableType = None

    def loadVariable(self, name):
        """
        Finds the variable with name (case insensitive) and makes sure that
        the variable is loaded in the reader.  If that variable exists (and
        is loaded), then the currentVariable and currentVariableType fields
        of this object are set.  If the variable is not found, an exception is
        raised.
        """
        var = _find_variable_or_raise_exception(self.reader, name)
        self.currentVariable = var.name
        self.currentVariableType = var.type
        self.currentVariableComponent = var.component

    def get_next_screenshot_filename(self, label=""):
        self.filename_counter += 1
        return "pvblot_%05d_%s.png" % (self.filename_counter, label)

    def _getCurrentVariableInfo(self):
        if not self.currentVariable:
            raise BlotishError("No variable selected.")
        r = self.reader
        itr = zip([NODE_VARIABLE, ELEMENT_VARIABLE, GLOBAL_VARIABLE],
                  [r.PointData, r.CellData, r.FieldData])
        for var_type, var_data in itr:
            if self.currentVariableType == var_type:
                return var_data[self.currentVariable]
        return None
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
            rgbpoints.append(i + self._block_id_offset)
            rgbpoints.extend(STANDARD_COLORS[i % self.numBlockColors])
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
        range = self.currentVariableInfo.GetRange(self.currentVariableComponent)
        
        if self.currentVariableComponent == -1:
            lt.VectorMode = "Magnitude"
        else:
            lt.VectorMode = "Component"
            lt.VectorComponent = self.currentVariableComponent

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

    def getPlotTimes(self):
        return self.time_selection.get_selected_times()


def _cleanup():
    global state
    del state
    state = None

def _get_prompt():
    if state.ioHelper.get_output():
        return str(state.ioHelper.get_output())
    return state.prompt

def _handle_input(line):
    return state.ioHelper.handle_input(line)

def _get_io_helper():
    return state.ioHelper

def _set_interactive(value):
    """Sets interactive mode on/off.  Sets diskwrite mode to the opposite value."""
    state.interactive = value
    state.diskwrite = not value

def _find_variable_or_raise_exception(reader, name):
    """Returns a valid blot_common.Variable instance or raises a BlotishError."""
    var = blot_common.find_variable(reader, name)
    if not var:
        raise BlotishError("'%s' is an invalid variable name" % name)
    return var

def _init_blotish(filename):
    """
    This command initializes the blotish commands.  Think of this as the
    super-command that must be run before all other sub-commands.

    This function takes a single string argument that is the path to the
    exodus file to load.
    """
    global state
    if state: return
    elif not filename: raise BlotishError("You need to specify a filename.")

    state = _State()
    state.filename = filename

    # Make sure a render window has been created
    state.renderview = paraview.simple.GetRenderView()
    state.renderview.UseOffscreenRenderingForScreenshots = 0

    # This will automatically load the reader, filename must be set first.
    reader = state.reader

    if not reader:
        state.filename = None
        raise BlotishError("Failed to load file %s" % filename)

    reader.UpdatePipeline()
    datainfo = reader.GetDataInformation()
    state.numBlocks = datainfo.GetNumberOfDataSets()

    print "Database:", reader.FileName
    print
    print "Number of nodes          =", datainfo.GetNumberOfPoints()
    print "Number of elements       =", datainfo.GetNumberOfCells()
    print "Number of element blocks =", state.numBlocks
    print

    if reader.DatabaseType == EXODUS:
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

    elif reader.DatabaseType == SPYPLOT:

        print "Variable Names"
        print "Element:",
        for name in reader.Input.CellArrays.Available:
            print " ", name,

    print
    print "Number of time steps     =", len(reader.TimestepValues)
    if len(reader.TimestepValues) > 0:
        print "   Minimum time          =", min(reader.TimestepValues)
        print "   Maximum time          =", max(reader.TimestepValues)
        all_times = reader.TimestepValues
    else:
        all_times = [0]
    print
    state.time_selection = timestep_selection.TimestepSelection(all_times)

def _finish_plot_change():
    """Maybe re-render depending on the value of state.autoplot"""
    if state.autoplot:
        paraview.simple.Render()

def _find_variable_command(variable_name):
    """When a command is entered that doesn't match any existing command it
    might be a variable name in which case we should call typlot with that
    variable as the first argument."""
    def call_typlot(*args, **kwargs):
        args = list(args)
        args.insert(0, variable_name)
        typlot(*args, **kwargs)
    call_typlot.__name__ = typlot.__name__
    call_typlot.__doc__ = typlot.__doc__

    var = blot_common.find_variable(state.reader, variable_name)
    if var: return call_typlot
    return None

@_subprogram("tplot")
@async_io_helper.wrap(_get_io_helper)
def typlot(*args, **kwargs):
    """Create a time plot of the given variable"""

    _require_plot_mode(tplot_mod.TYCURVE)

    # If args not specified then get args using asynchronous input
    if not args:
        yield "Y VARIABLE "
        args = kwargs["io_helper"].get_input().split()
        if not args: args = [" "]

    args = list(args)
    var = _find_variable_or_raise_exception(state.reader, args.pop(0))

    if var.type == GLOBAL_VARIABLE:
        if args:
            blot_common.print_blot_warning("Extra tokens after global "
                                           "variable name ignored.")
        c = tplot_mod.Curve(var)
        state.tplot.add_curve(c)
        state.tplot.print_show()
        return

    if not args:
        raise BlotishError("Expected node/element number")

    s = " ".join(args)
    try: selected_ids = number_list_parser.parse_number_list(s, int)
    except number_list_parser.Error, err: raise BlotishError(err)

    uniq = dict()
    selected_ids = [uniq.setdefault(i,i) for i in selected_ids if i not in uniq]

    for id in selected_ids:
        c = tplot_mod.Curve(var, id)
        state.tplot.add_curve(c)
    state.tplot.print_show()


def _require_plot_mode(mode):
    """Check if the current tplot mode is the given mode.  If there is a mismatch
    raise a BlotishError."""
    current_mode = state.tplot.get_current_curve_mode()
    if current_mode is not None:
        if mode != current_mode:
            raise BlotishError("Time curves and X-Y curves must be defined separately")


@_subprogram("tplot")
@async_io_helper.wrap(_get_io_helper)
def xyplot(io_helper=None):
    """Create a variable versus variable plot with data points at each timestep"""

    _require_plot_mode(tplot_mod.XYCURVE)

    def extract_variable_and_ids(args):
        # convert to list of at least one token
        if not args: args = [" "]
        args = list(args)

        # treat the first token as a variable name to lookup a variable
        var = _find_variable_or_raise_exception(state.reader, args.pop(0))

        # variable must not be a global
        if var.type == GLOBAL_VARIABLE:
            raise BlotishError("XYPlot does not support global variables")
            return

        # Now parse the remaining tokens as a list of numbers and number ranges
        if not args: raise BlotishError("Expected node/element number")
        s = " ".join(args)
        try: selected_ids = number_list_parser.parse_number_list(s, int)
        except number_list_parser.Error, err: raise BlotishError(err)

        # Convert to list of unique values and return
        uniq = dict()
        selected_ids = [uniq.setdefault(i,i) for i in selected_ids if i not in uniq]
        return var, selected_ids

    yield "X VARIABLE "
    args = io_helper.get_input().split()
    x_var, x_ids = extract_variable_and_ids(args)

    yield "Y VARIABLE "
    args = io_helper.get_input().split()
    y_var, y_ids = extract_variable_and_ids(args)

    if len(x_ids) != len(y_ids):
        raise BlotishError("The number of selected node or element ids must be "
                           "the same for the X variable and Y variable.")

    if x_var.type != y_var.type:
        raise BlotishError("X and Y variable must be the same type (node or element)")

    for xid, yid in zip(x_ids, y_ids):
        if xid != yid:
            raise BlotishError("XYPlot does not support plotting variables at "
                               "different node or element ids against each other.")


    for id in x_ids:
        c = tplot_mod.Curve(y_var, id)
        c.set_x_variable(x_var)
        state.tplot.add_curve(c)
    state.tplot.print_show()



def detour():
    """
    Start a subprogram to plot deformed meshes.
    """
    print " DETOUR - a deformed mesh and contour plot program"
    print
    state.subProgram = "detour"
    paraview.simple.SetActiveView(state.renderview)
    wireframe()

def tplot():
    """Start TPLOT subprogram to plot curves of variables over times.  Variables
    can be nodal, elemental, or global."""
    print " TPLOT - a time history or X-Y plot program"
    print
    state.subProgram = "tplot"
    state.tplot = tplot_mod.TPlot()
    paraview.simple.SetActiveView(state.tplot.view)

@_subprogram("tplot")
def overlay(*args):
    """Turn overlay on or off."""
    value = True
    if args: value = _token_to_bool(args[0])
    state.tplot.overlay = value
    print " Overlay curves for all variables",
    if value: print "on"
    else: print "off"
    print

@_subprogram("detour")
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

@_subprogram("detour")
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

@_subprogram("detour")
def mlines(*args):
    """
    Turns on/off the grid lines for block elements.  The default value
    is on.  If it is on, then the wireframe grid is drawn.  If it is off,
    the wireframe grid is not drawn.
    """
    value = True
    if args: value = _token_to_bool(args[0])
    state.mlines = value
    print " Display element mesh lines",
    if value: print "on"
    else: print "off"
    print
    _updateMeshRender()
    _finish_plot_change()


def diskwrite(*args):
    """
    Turns on/off writing to disk.  During non-interactive mode
    disk write default to on, else it is off.
    """
    value = True
    if args: value = _token_to_bool(args[0])
    state.diskwrite = value
    print " Write to disk",
    if value: print "on"
    else: print "off"
    print

@_subprogram("detour")
def wireframe(*args):
    "Sets the view to wireframe mesh mode, which displays the mesh lines."
    state.resetPipeline()
    state.mlines = 1
    state.msurface = 0
    todraw = _updateMeshRender()
    paraview.simple.SetDisplayProperties(todraw,
                                         ColorAttributeType=state._block_id_field,
                                         ColorArrayName=state._block_id_variable,
                                         LookupTable=state.blockLookupTable)
    _finish_plot_change()

@_subprogram("detour")
def solid():
    """
    Sets the view to solid mesh mode, which paints each element using a
    different color for each element block.
    """
    print " Solid mesh plot"
    print
    state.resetPipeline()
    state.msurface = 1
    todraw = _updateMeshRender()
    paraview.simple.SetDisplayProperties(todraw,
                                         ColorAttributeType=state._block_id_field,
                                         ColorArrayName=state._block_id_variable,
                                         LookupTable=state.blockLookupTable)
    _finish_plot_change()

@_subprogram("detour")
def paint(variable=None):
    """Sets the view to paint contour mode, which paints contours of the
    given variable on the mesh.  The variable may be either nodal or element.
    If no variable is specified, the last selected variable is used."""
    if variable:
        state.loadVariable(variable)
    if not state.currentVariable:
        raise BlotishError("No variable selected.")
    if state.currentVariableType not in [NODE_VARIABLE, ELEMENT_VARIABLE]:
        raise BlotishError("Contour variable must be nodal or element variable")

    state.resetPipeline()
    state.mlines = 0
    state.msurface = 1
    todraw = _updateMeshRender()
    paraview.simple.SetDisplayProperties(todraw,
                                         ColorAttributeType=state.currentVariableType,
                                         ColorArrayName=state.currentVariable,
                                         LookupTable=state.spectrumLookupTable)
    _finish_plot_change()

@_subprogram("detour")
def contour(variable=None):
    """Sets the view to contour mode, which plots the line or surface contours
    where the given variable is equal to one of the contour values (set with
    the spectrum command)."""
    if variable:
        state.loadVariable(variable)
    if state.currentVariableType != NODE_VARIABLE:
        raise BlotishError("Only nodal variables supported by contour right now.")
    info = state.currentVariableInfo
    if (info.GetNumberOfComponents() > 1):
        raise BlotishError(state.currentVariable + " is not a scalar.")

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

@_subprogram("detour")
def vector(variable=None):
    """
    Sets the view to vector mode.  Pass the name of the variable as the
    argument, or pass nothing to use the last used variable.
    """
    if variable:
        state.loadVariable(variable)
    if state.currentVariableType != 'POINT_DATA':
        raise BlotishError("Only nodal variables supported by vector right now.")
    info = state.currentVariableInfo
    if (info.GetNumberOfComponents() < 2):
        raise BlotishError(state.currentVariable + " is not a vector.")
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
                                         ColorAttributeType=state._block_id_field,
                                         ColorArrayName=state._block_id_variable,
                                         LookupTable=state.blockLookupTable,
                                         Opacity=0.5)
    _finish_plot_change()




@async_io_helper.wrap(_get_io_helper)
def _tplot_plot(io_helper=None):
    if state.tplot.overlay:
        state.tplot.plot()

        if state.diskwrite:
            fname = state.get_next_screenshot_filename("tplot_overlay")
            paraview.simple.WriteImage(fname, state.tplot.view)

        return

    interactive = state.interactive
    nCurves = state.tplot.get_number_of_curves()
    for i in xrange(nCurves):
        state.tplot.plot(i)

        if state.diskwrite:
            fname = state.get_next_screenshot_filename("tplot_curve%02d"%i)
            paraview.simple.WriteImage(fname, state.tplot.view)

        if interactive and i < nCurves-1:
            yield "Enter 'Q' to quit, '' to continue. "
            result = io_helper.get_input().lower()
            if result.startswith('q'):
                break


def _tplot_reset():
    state.tplot.reset()
    
def _detour_reset():
    wireframe()

@_subprogram(["detour", "tplot"])
def plot(*args):
    "Generates the current plot."
    if state.subProgram == "tplot": return _tplot_plot()
    if state.subProgram == "detour": return _detour_plot()

@_subprogram(["detour", "tplot"])
def reset(*args):
    "Reset the plot settings"
    if state.subProgram == "tplot": return _tplot_reset()
    if state.subProgram == "detour": return _detour_reset()

@_subprogram(["detour", "tplot"])
def times(*args):
    """Select time steps"""
    try: state.time_selection.parse_times(" ".join(args))
    except timestep_selection.Error, err: raise BlotishError(err)
    print " Select specified whole times"
    print "    Number of selected times = %d" % len(state.time_selection.selected_indices)
    print

@_subprogram(["detour", "tplot"])
def steps(*args):
    """Select time steps"""
    try: state.time_selection.parse_steps(" ".join(args))
    except timestep_selection.Error, err: raise BlotishError(err)
    print " Select specified whole times"
    print "    Number of selected times = %d" % len(state.time_selection.selected_indices)
    print


@_subprogram("tplot")
def xscale(min_value=None, max_value=None):
    """Set the minimum and maximum values of the X axis"""
    state.tplot.set_xscale(_maybe_convert(min_value, float),
                           _maybe_convert(max_value, float))

@_subprogram("tplot")
def yscale(min_value=None, max_value=None):
    """Set the minimum and maximum values of the X axis"""
    state.tplot.set_yscale(_maybe_convert(min_value, float),
                           _maybe_convert(max_value, float))

@_subprogram("tplot")
def xlabel(*args):
    """Set the X axis label"""
    label = " ".join(args)
    state.tplot.set_xlabel(label)

@_subprogram("tplot")
def ylabel(*args):
    """Set the Y axis label"""
    label = " ".join(args)
    state.tplot.set_ylabel(label)

@_subprogram()
def nintv(value=None):
    """Set nintv"""
    state.time_selection.set_nintv(_maybe_convert(value, int))
    state.time_selection.print_show()

@_subprogram()
def zintv(value=None):
    """Set zintv"""
    state.time_selection.set_zintv(_maybe_convert(value, int))
    state.time_selection.print_show()

@_subprogram()
def deltime(value=None):
    """Set deltime"""
    state.time_selection.set_deltime(_maybe_convert(value, float))
    state.time_selection.print_show()

@_subprogram()
def tmin(value=None):
    """Set deltime"""
    state.time_selection.set_tmin(_maybe_convert(value, float))
    state.time_selection.print_show()

@_subprogram()
def tmax(value=None):
    """Set deltime"""
    state.time_selection.set_tmax(_maybe_convert(value, float))
    state.time_selection.print_show()

@_subprogram()
def alltimes():
    """Set time selection mode to ALLTIMES"""
    state.time_selection.set_alltimes()
    state.time_selection.print_show()

def show(name=""):
    """Show the given parameter name"""
    
    name = name.lower()
    if name in ["tmin", "tmax", "nintv", "zintv", "times", "steps"]:
        sel = state.time_selection
        count = 0
        state.time_selection.print_show()
        print " %d selected time steps" % len(sel.selected_times)
        for idx, time in zip(sel.selected_indices, sel.selected_times):
            print "     %d)  (step %3d)  %f" % (count+1, idx+1, time)
            count += 1
        print


@async_io_helper.wrap(_get_io_helper)
def _detour_plot(io_helper=None):

    interactive = state.interactive
    plotTimes = state.time_selection.selected_times
    nSteps = len(plotTimes)
    view = state.renderview
    for stepIdx in xrange(nSteps):
        t = plotTimes[stepIdx]
        view.ViewTime = t
        paraview.simple.Render(view)

        if state.diskwrite:
            paraview.simple.WriteImage(state.get_next_screenshot_filename("detour"), view)

        if interactive and stepIdx != nSteps-1:
            yield ("Time %f: Enter 'C' to complete, 'Q' to quit, '' to continue. " % t)
            result = io_helper.get_input().lower()
            if result.startswith('q'):
                break
            elif result.startswith('c'):
                interactive = False

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

@_subprogram("detour")
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


@_subprogram("detour")
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
        if factor in ["each", "mesh", "reset"]:
            paraview.simple.ResetCamera()
            _finish_plot_change()
            return
        factor = float(factor)
    camera.Dolly(factor)
    _finish_plot_change()

@_subprogram("detour")
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

def _token_to_bool(token):
    token = str(token).lower()
    if token in ["1", "on", "true", "yes"]:
        return True
    if token in ["0", "off", "false", "no"]:
        return False
    raise BlotishError("Expected \"ON\" or \"OFF\"")


def _maybe_convert(token, number_class):
    """Attempts to convert the argument to the given number class if the
    argument is not None.  Raises an exception on a conversion error.  If the
    argument is None, returns None without raising any error."""
    if token is None: return None
    try: num = number_list_parser.convert_to_number_class(token, number_class)
    except number_list_parser.Error, err: raise BlotishError(err)
    return num


