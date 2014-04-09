# -----------------------------------------------------------------------------
# ParaView Python - Path setup
# -----------------------------------------------------------------------------

import sys, time, os
pv_path = '/Users/seb/work/code/ParaView/build-ninja'
output_dir = '/tmp/composite-test'

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

from paraview.simple import *

LoadPlugin(pv_path + '/lib/libRGBZView.dylib')

wavelet = Wavelet()
contour_values = [ 64.0, 90.6, 117.2, 143.8, 170.4, 197.0, 223.6, 250.2]
filters = [ wavelet ]
representations = []

for iso_value in contour_values:
    filters.append( Contour( Input=wavelet, PointMergeMethod="Uniform Binning", ContourBy = ['POINTS', 'RTData'], Isosurfaces = [iso_value], ComputeScalars = 1 ) )

view = CreateView("RGBZView")
view.CompositeDirectory = output_dir



lut = GetLookupTableForArray( "RTData", 1, RGBPoints=[63.96153259277344, 0.23, 0.299, 0.754, 157.09104919433594, 0.865, 0.865, 0.865, 250.22056579589844, 0.706, 0.016, 0.15], VectorMode='Magnitude', NanColor=[0.25, 0.0, 0.0], ColorSpace='Diverging', ScalarRangeInitialized=1.0 )

for data in filters:
    rep = Show(data, view)
    rep.LookupTable = lut
    rep.ColorArrayName = "RTData"
    rep.ColorAttributeType = "POINT_DATA"
    representations.append(rep)

Render(view)
view.ResetClippingBounds()
view.FreezeGeometryBounds()

view.ResetActiveImageStack()
view.RGBStackSize = 1 + len(filters)

for rep in representations:
    view.ActiveRepresentation = rep
    view.CaptureActiveRepresentation()

view.WriteImage()
view.ComputeZOrdering()
view.WriteComposite()