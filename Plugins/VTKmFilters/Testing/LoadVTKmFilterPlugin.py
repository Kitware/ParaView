from paraview.simple import *

# Load the distributed plugin.
LoadDistributedPlugin("VTKmFilters" , ns=globals())

# Now use a filter from that plugin.
# We don't really care if the filter works correctly in this test,
# we're just verifying that the plugin can be loaded correctly.
Wavelet()
UpdatePipeline()
VTKmContour()
UpdatePipeline()
