"""
Execute example by running the following commad:

   # The file format needs to be readable by ParaView (.vtk, .vtp, .ex2, ...)
   $ pvpython -m paraview.simple.example.demo2 --data /path/to/disk_out_ref.ex2

"""

import argparse
from paraview.simple import *


def demo2(fname="/Users/berk/Work/ParaView/ParaViewData/Data/disk_out_ref.ex2"):
    """This demo shows the use of readers, data information and display
    properties."""

    # Create the exodus reader and specify a file name
    reader = OpenDataFile(fname)

    # Get the list of point arrays (If exodus file).
    if "NodeBlockFields" in reader.ListProperties():
        avail = reader.NodeBlockFields.Available
        print(" => Available Arrays:", avail)
        # Select all arrays
        reader.NodeBlockFields = avail

    # Turn on the visibility of the reader
    representation = Show(
        reader,
        # Preconfigure the representation with property values
        Representation="Wireframe",
    )

    # Set more properties
    representation.Set(
        LineWidth=4,
        RenderLinesAsTubes=1,
    )

    # Black background is not pretty
    view = Render()
    view.Set(Background=[0.4, 0.4, 0.6])

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
    available_arrays = []
    print("Number of point arrays:", len(pdi))
    for i in range(len(pdi)):
        ai = pdi[i]
        print("----------------")
        print("Array:", i, " ", ai.Name, ":")
        numComps = ai.GetNumberOfComponents()
        print("Number of components:", numComps)
        for j in range(numComps):
            print("Range:", ai.GetRange(j))

        # Capture available array names
        available_arrays.append(ai.Name)

    # White is boring. Let's color the geometry using a variable.
    if "Pres" in available_arrays:
        ColorBy(representation, ("POINTS", "Pres"))
        AssignFieldToColorPreset("Pres", "Fast", (0.00678, 0.0288))
    elif len(available_arrays):
        array_name = available_arrays[0]
        ColorBy(representation, ("POINTS", array_name))
        # will use the full range of the array by default
        AssignFieldToColorPreset(array_name, "Fast")

    # Lock the script execution to interact with 3D view
    Interact()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="pvpython::paraview.simple.example.demo2",
        description="ParaView Python demo2 example",
    )
    parser.add_argument("--data", help="Path to a data file to load", required=True)
    args, _ = parser.parse_known_args()
    print(f"Loading: {args.data}")
    demo2(args.data)
