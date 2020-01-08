# This test tests a scenario where there was potential that
# the delivery-stage in rendering would entirely miss representation (namely,
# selection representation that only had empty data) to deliver.
# This manifested as bug #17879.

from paraview.simple import *

wavelet1 = Wavelet()
renderView1 = CreateView('RenderView')
wavelet1Display = Show(wavelet1, renderView1)

renderView1.Update()

pointDatatoCellData1 = PointDatatoCellData(Input=wavelet1)
pointDatatoCellData1Display = Show(pointDatatoCellData1, renderView1)

# hide wavelet1
Hide(wavelet1, renderView1)

# update the view to ensure updated data information
renderView1.Update()

# set scalar coloring
ColorBy(pointDatatoCellData1Display, ('CELLS', 'RTData'))

# rescale color and/or opacity maps used to include current data range
pointDatatoCellData1Display.RescaleTransferFunctionToDataRange(True, True)

# change representation type
pointDatatoCellData1Display.SetRepresentationType('Volume')

Render()

# hide data in view
Hide(pointDatatoCellData1, renderView1)

# set active source
SetActiveSource(wavelet1)

# show data in view
wavelet1Display = Show(wavelet1, renderView1)

# set scalar coloring
ColorBy(wavelet1Display, ('POINTS', 'RTData'))

# rescale color and/or opacity maps used to include current data range
wavelet1Display.RescaleTransferFunctionToDataRange(True, True)

# change representation type
wavelet1Display.SetRepresentationType('Volume')

Render()
