from paraview.simple import *


unstructuredCellTypes1 = UnstructuredCellTypes()
plotOverLine1 = PlotOverLine(Input=unstructuredCellTypes1)
plotOverLine1.Point1 = [0, 0, 0]
plotOverLine1.Point2 = [10.0, 10.0, 10.0]
plotOverLine1.UpdatePipeline()

assert plotOverLine1.GetDataInformation().DataInformation.GetNumberOfPoints() == 1001

wavelet = Wavelet()
plotOverLine1 = PlotOverLine(Input=wavelet)
plotOverLine1.Point1 = [0, 0, 0]
plotOverLine1.Point2 = [10.0, 10.0, 10.0]
plotOverLine1.UpdatePipeline()

assert plotOverLine1.GetDataInformation().DataInformation.GetNumberOfPoints() == 1001

hyperTreeGridRandom1 = HyperTreeGridRandom()
plotOverLine1 = PlotOverLine(Input=hyperTreeGridRandom1)
plotOverLine1.Point1 = [-10.0, -10.0, -10.0]
plotOverLine1.Point2 = [10.0, 10.0, 10.0]
plotOverLine1.UpdatePipeline()

assert plotOverLine1.GetDataInformation().DataInformation.GetNumberOfPoints() == 1001
