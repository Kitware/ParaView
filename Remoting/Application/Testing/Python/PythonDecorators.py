from paraview.simple import *
from paraview.decorator_utils import get_decorators, should_trace_based_on_decorators


def GenericDecoratorTest():
    view = GetActiveViewOrCreate("RenderView")
    decorator = get_decorators(view, "LegendGrid")[0]

    # LegendGrid is enabled only is CameraParallelProjection is on
    view.CameraParallelProjection = 0
    assert decorator.Enable() == False
    view.CameraParallelProjection = 1
    assert decorator.Enable() == True


def ShowWidgetDecoratorTest():
    view = GetActiveViewOrCreate("RenderView")
    decorator = get_decorators(view, "StereoType")[0]

    view.StereoRender = 0
    assert decorator.CanShow(True) == False
    view.StereoRender = 1
    assert decorator.CanShow(True) == True


def EnableWidgetDecoratorTest():
    chartView = CreateView("XYChartView")

    wavelet = Wavelet()
    plotOverLine = PlotOverLine(Input=wavelet)
    plotOverLine1Display = Show(plotOverLine, chartView, "XYChartRepresentation")

    #  XArrayName is enabled only if UseIndexForXAxis is False
    decorator = get_decorators(plotOverLine1Display, "XArrayName")[0]
    plotOverLine1Display.UseIndexForXAxis = 1
    assert decorator.Enable() == False
    # Properties modified on plotOverLine1Display
    plotOverLine1Display.UseIndexForXAxis = 0
    assert decorator.Enable() == True


def CompositeDecotatorTest():
    view = GetActiveViewOrCreate("RenderView")
    wavelet = Wavelet()
    waveletDisplay = Show(wavelet, view, "UniformGridRepresentation")
    waveletDisplay.Representation = "Point Gaussian"
    waveletDisplay.ScaleByArray = 0

    # ShowWidgetDecorator
    decorator = get_decorators(waveletDisplay, "UseScaleFunction")[0]

    assert decorator.CanShow(True) == False

    # this should be available only if both ScaleByArray and UseScaleFunction are set to 1
    decorator = get_decorators(waveletDisplay, "ScaleTransferFunction")[0]

    assert decorator.CanShow(True) == False
    waveletDisplay.ScaleByArray = 1
    waveletDisplay.UseScaleFunction = 0
    assert decorator.CanShow(True) == False
    waveletDisplay.ScaleByArray = 0
    waveletDisplay.UseScaleFunction = 1
    assert decorator.CanShow(True) == False
    waveletDisplay.ScaleByArray = 1
    waveletDisplay.UseScaleFunction = 1
    assert decorator.CanShow(True) == True


def PropertyGroupDecoratorTest():
    view = GetActiveViewOrCreate("RenderView")
    wavelet = Wavelet()
    waveletDisplay = Show(wavelet, view, "GeometryRepresentation")

    # Gaussian Radius is relevant only for Gaussian representations
    assert should_trace_based_on_decorators(waveletDisplay, "GaussianRadius") == False
    waveletDisplay.SetRepresentationType("Point Gaussian")
    assert should_trace_based_on_decorators(waveletDisplay, "GaussianRadius") == True


def InputDataTypeDecoratorTest():
    wavelet = Wavelet()
    clip = Clip(Input=wavelet)
    # HyperTreeGridClipFunction is relevant only for HyperTreeGrid datasets
    assert should_trace_based_on_decorators(clip, "HyperTreeGridClipFunction") == False

    hyperGrid = HyperTreeGrid()
    clip = Clip(Input=hyperGrid)
    assert should_trace_based_on_decorators(clip, "HyperTreeGridClipFunction") == True


def MultiComponentDecoratorTest():
    view = GetActiveViewOrCreate("RenderView")
    torus = Superquadric()
    torus.UpdatePipeline()
    torusDisplay = Show(torus, view, "GeometryRepresentation")
    ColorBy(torusDisplay,['POINTS','Normals'])

    # MultiComponentsMapping appears only if the array has 2 or 4 elements
    decorator = get_decorators(torusDisplay, "MultiComponentsMapping")[-1]
    assert decorator.CanShow(True) == False

    ColorBy(torusDisplay, ['POINTS', 'TextureCoords'])
    decorator = get_decorators(torusDisplay, "MultiComponentsMapping")[-1]
    assert decorator.CanShow(True) == True


if __name__ == "__main__":
    GenericDecoratorTest()
    ShowWidgetDecoratorTest()
    EnableWidgetDecoratorTest()
    CompositeDecotatorTest()
    PropertyGroupDecoratorTest()
    InputDataTypeDecoratorTest()
    MultiComponentDecoratorTest()
