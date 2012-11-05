from paraview.simple import *
import sys


def assertTrue(actual, excepted, error_message):
  if actual != excepted:
    print >> sys.stderr, 'ERROR:  %s Actual: %s Expected: %s' \
                         % (error_message, actual, excepted)
    sys.exit(-1)

w = Wavelet()
w.UpdatePipeline()
image_data = w.GetClientSideObject().GetOutput()
num_points = image_data.GetNumberOfPoints()
assertTrue(num_points, 9261, 'Incorrect number of points.')

points = ()
for i in range(0, num_points):
  points += image_data.GetPoint(i)

assertTrue(hash(points), -3584845732413037660, 'Point hash has changed.')
assertTrue(image_data.GetBounds(), (-10.0, 10.0, -10.0, 10.0, -10.0, 10.0),
           'Incorrect bounds.')
