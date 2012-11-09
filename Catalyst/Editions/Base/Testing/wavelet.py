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

md5 = hashlib.md5()
md5.update(str(points))
assertTrue(md5.hexdigest(), "c5faaca1a1510d746c41588d4a502e66",
           'Point hash has changed.')
assertTrue(image_data.GetBounds(), (-10.0, 10.0, -10.0, 10.0, -10.0, 10.0),
           'Incorrect bounds.')
