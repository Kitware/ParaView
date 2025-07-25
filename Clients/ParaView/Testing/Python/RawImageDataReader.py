from paraview.simple import *
from paraview import smtesting
import sys

smtesting.ProcessCommandLineArguments()

# setting the backwards compatibility version for this test, see below
paraview.compatibility.major = 6
paraview.compatibility.minor = 0

def loadRawImage(sourceName, dataType, numDims, dimensions, dataExtent):
    try:
        im3d_reader = ImageReader(
            FileNames=[f"{smtesting.DataDir}/Testing/Data/{sourceName}.raw"],
            DataScalarType=dataType,
            FileDimensionality=numDims,
            Dimensions=dimensions,
            registrationName=sourceName
        )
        im3d_display = Show(im3d_reader)
        im3d_display.Representation = 'Surface With Edges'
        ColorBy(im3d_display, ('POINTS', 'ImageFile'))
        Render()
    except Exception as e:
        print(e)
        sys.exit("Could not load raw image. Test failed.")

    # Check the source is loaded with the right dimensions set
    source = FindSource(sourceName)
    assert(source is not None)
    assert(source.FileDimensionality == str(numDims))
    assert(str(source.Dimensions) == str(dimensions))

    # backwards comapatiblity:
    # check that data extent attribute is readable and matches the expected value
    assert(str(source.DataExtent) == str(dataExtent))
    # check that it is also editable and that the dimensions property updates
    source.DataExtent = dataExtent[:1] + [dataExtent[1] - 1] + dataExtent[2:]
    dimensions[0] -= 1
    assert(str(source.Dimensions) == str(dimensions))

# open a 3D raw image
loadRawImage(
    sourceName="raw3DImage",
    dataType="unsigned char",
    numDims=3,
    dimensions=[4, 3, 5],
    dataExtent=[0, 3, 0, 2, 0, 4] # for backwards compatiblity, 'DataExtent' was replaced by 'Dimensions' in 6.1.0
)

# open a 2D raw image
loadRawImage(
    sourceName="raw2DImage",
    dataType="unsigned char",
    numDims=2,
    dimensions=[5, 3, 0],
    dataExtent=[0, 4, 0, 2, 0, 0],
)
