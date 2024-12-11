from paraview.simple import *


def demo2(fname="/Users/berk/Work/ParaView/ParaViewData/Data/disk_out_ref.ex2"):
    """This demo shows the use of readers, data information and display
    properties."""

    # Create the exodus reader and specify a file name
    reader = ExodusIIReader(FileName=fname)
    # Get the list of point arrays.
    avail = reader.PointVariables.Available
    print(avail)
    # Select all arrays
    reader.PointVariables = avail

    # Turn on the visibility of the reader
    Show(reader)
    # Set representation to wireframe
    SetDisplayProperties(Representation="Wireframe")
    # Black background is not pretty
    SetViewProperties(Background=[0.4, 0.4, 0.6])
    Render()
    # Change the elevation of the camera. See VTK documentation of vtkCamera
    # for camera parameters.
    # NOTE: THIS WILL BE SIMPLER
    GetActiveCamera().Elevation(45)
    Render()
    # Now that the reader executed, let's get some information about it's
    # output.
    pdi = reader[0].PointData
    # This prints a list of all read point data arrays as well as their
    # value ranges.
    print("Number of point arrays:", len(pdi))
    for i in range(len(pdi)):
        ai = pdi[i]
        print("----------------")
        print("Array:", i, " ", ai.Name, ":")
        numComps = ai.GetNumberOfComponents()
        print("Number of components:", numComps)
        for j in range(numComps):
            print("Range:", ai.GetRange(j))
    # White is boring. Let's color the geometry using a variable.
    # First create a lookup table. This object controls how scalar
    # values are mapped to colors. See VTK documentation for
    # details.
    # Map min (0.00678) to blue, max (0.0288) to red
    SetDisplayProperties(LookupTable=MakeBlueToRedLT(0.00678, 0.0288))
    # Color by point array called Pres
    SetDisplayProperties(ColorArrayName=("POINTS", "Pres"))
    Render()


if __name__ == "__main__":
    demo2()
