"""A Python plugin, loadable using Tools > Manage Plugins > Load New, that adds a
filter implemented in Python and Python functions to the "Analysis Tools" menu
and toolbar. These are the same menu and toolbar as the ones added by
AnalysisTools.xml, if also loaded."""

from paraview.util.vtkAlgorithm import (
    VTKPythonAlgorithmBase, smdomain, smmenu, smproperty, smproxy)


# ------------------------------------------------------------------------------
# A filter, added to the menu and the toolbar.
# ------------------------------------------------------------------------------
@smmenu.proxy(menu="Analysis Tools", icon=":/pqWidgets/Icons/pqCalculator.svg", toolbar=True)
@smproxy.filter(label="Normalized Height")
@smproperty.input(name="Input")
@smdomain.datatype(dataTypes=["vtkDataSet"], composite_data_supported=False)
class NormalizedHeight(VTKPythonAlgorithmBase):
    """Adds a "NormalizedHeight" point array: the Z coordinate of each point,
    scaled to [0, 1] over the bounds of the input."""

    def __init__(self):
        super().__init__(nInputPorts=1, nOutputPorts=1, outputType="vtkDataSet")

    def RequestDataObject(self, request, inInfo, outInfo):
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)
        if outData is None or (not outData.IsA(inData.GetClassName())):
            outData = inData.NewInstance()
            outInfo.GetInformationObject(0).Set(outData.DATA_OBJECT(), outData)
        return super().RequestDataObject(request, inInfo, outInfo)

    def RequestData(self, request, inInfo, outInfo):
        from vtkmodules.vtkFiltersCore import vtkElevationFilter

        inData = self.GetInputData(inInfo, 0, 0)
        bounds = inData.GetBounds()
        elevation = vtkElevationFilter()
        elevation.SetInputData(inData)
        elevation.SetLowPoint(0, 0, bounds[4])
        elevation.SetHighPoint(0, 0, bounds[5])
        elevation.SetScalarRange(0, 1)
        elevation.Update()

        outData = self.GetOutputData(outInfo, 0)
        outData.ShallowCopy(elevation.GetOutput())
        outData.GetPointData().GetArray("Elevation").SetName("NormalizedHeight")
        return 1


# ------------------------------------------------------------------------------
# Python functions, called when the menu (or toolbar) items are triggered.
# ------------------------------------------------------------------------------
@smmenu.action(menu="Analysis Tools", label="Color By Normalized Height",
               icon=":/pqWidgets/Icons/pqPalette.svg", toolbar=True)
def color_by_normalized_height():
    """Colors the active source by its "NormalizedHeight" point array."""
    from paraview.simple import ColorBy, GetActiveSource, GetActiveViewOrCreate, Show

    source = GetActiveSource()
    if source is None or "NormalizedHeight" not in source.PointData.keys():
        print("The active source has no 'NormalizedHeight' point array.")
        return
    view = GetActiveViewOrCreate("RenderView")
    display = Show(source, view)
    ColorBy(display, ("POINTS", "NormalizedHeight"))
    display.RescaleTransferFunctionToDataRange(True, False)
    display.SetScalarBarVisibility(view, True)


@smmenu.action(menu="Analysis Tools/Reports", label="Print Array Ranges")
def print_array_ranges():
    """Prints the ranges of the point data arrays of the active source."""
    from paraview.simple import GetActiveSource

    source = GetActiveSource()
    if source is None:
        print("No active source.")
        return
    for name in source.PointData.keys():
        print("%s: %s" % (name, source.PointData[name].GetRange()))
