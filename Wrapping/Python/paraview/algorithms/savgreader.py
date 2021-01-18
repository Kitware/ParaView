"""Reader for NIST SAVG files.

This reader reads in a .savg file and produces a vtkPartionedDatasSetCollection
with one vtkPartitionedDataSet for each type of primitive: points, lines,
and polygons.

"""

from paraview.util.vtkAlgorithm import VTKPythonAlgorithmBase

from .. import print_error


def get_coords_from_line(line):
    """ Given a line, split it, and parse out the coordinates.

        Return:

             A tuple containing coords (position, color, normal)
    """
    values = line.split()

    pt = None
    pt_n = None
    pt_col = None

    # The first three are always the point coords
    if len(values) >= 3:
        pt = [float(values[0]), float(values[1]), float(values[2])]

    # Then if there are only 6 total, the next three are normal coords
    if len(values) == 6:
        pt_n = [float(values[3]), float(values[4]), float(values[5])]
    else:
        if len(values) >= 7:    # Otherwise the next 4 are colors
            pt_col = [float(values[3]), float(values[4]), float(values[5]), float(values[6])]
        if len(values) >= 10:   # And if there are more, those are the normals
            pt_n = [float(values[7]), float(values[8]), float(values[9])]

    return pt, pt_col, pt_n


def not_supported(line):
    return (
        line.startswith('tri') or
        line.startswith('pixel') or
        line.startswith('text') or
        line.startswith('program') or
        line.startswith('attribute') or
        line.startswith('nooptimizations')
    )


class SAVGReader(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0, nOutputPorts=1, outputType="vtkPartitionedDataSetCollection")
        self._filename = None
        self._ndata = None
        self._timesteps = None

    def SetFileName(self, name):
        """Specify filename for the file to read."""
        if name.lower().endswith("savg"):
            if self._filename != name:
                self._filename = name
                self.Modified()

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonCore import vtkFloatArray, vtkPoints
        from vtkmodules.vtkCommonDataModel import (
            vtkCellArray,
            vtkCompositeDataSet,
            vtkPartitionedDataSet,
            vtkPartitionedDataSetCollection,
            vtkPolyData
        )

        output = vtkPartitionedDataSetCollection.GetData(outInfoVec);
        partitioned_datasets = []
        partitioned_dataset_names = []

        # Parse line file
        if not self._filename:
            print_error("SAVGReader requires a FileName")
            return 0

        # Stores lines of text from the file associated with each group of
        # geometries encountered.
        geometries = {
            "lines": [],
            "points": [],
            "poly": []
        }

        # Read the file and build up data structure to hold the primitives
        with open(self._filename, "r") as file:
            current = None
            for line in file:
                parts = line.split("#")
                line = parts[0].strip().lower()

                if len(line) < 1:
                    continue

                if not_supported(line):
                    continue

                if line.startswith("lin"):
                    geometries["lines"].append({
                        "rgba": None,
                        "values": []
                    })
                    current = geometries["lines"][-1]
                    line_parts = line.split(" ")
                    if len(line_parts) == 5:
                        current["rgba"] = [float(n) for n in line_parts[1:]]
                elif line.startswith("point"):
                    geometries["points"].append({
                        "rgba": None,
                        "values": [],
                    })
                    current = geometries["points"][-1]
                    line_parts = line.split(" ")
                    if len(line_parts) == 5:
                        current["rgba"] = [float(n) for n in line_parts[1:]]
                elif line.startswith("poly"):
                    geometries["poly"].append({
                        "rgba": None,
                        "npts": None,
                        "values": [],
                    })
                    current = geometries["poly"][-1]
                    line_parts = line.split(" ")
                    if len(line_parts) == 2:
                        current["npts"] = int(line_parts[1])
                    elif len(line_parts) == 6:
                        current["rgba"] = [float(n) for n in line_parts[1:5]]
                        current["npts"] = int(line_parts[5])
                elif line.startswith("end"):
                    current = None
                else:
                    if current is not None:
                        if "npts" in current and current["npts"] is not None:
                            # polygon, known num pts per poly
                            if len(current["values"]) == current["npts"]:
                                # Reached the number of points for the current one,
                                # start a new one.
                                geometries["poly"].append({
                                    "npts": current["npts"],
                                    "rgba": current["rgba"],
                                    "values": []
                                })
                                current = geometries["poly"][-1]
                        pt, pt_col, pt_n = get_coords_from_line(line)
                        if pt:
                            current["values"].append({
                                "pos": pt,
                            })
                            color = pt_col or current["rgba"]
                            if color:
                                current["values"][-1]["col"] = color
                            if pt_n:
                                current["values"][-1]["norm"] = pt_n

        # Build lines polydata if there were any lines
        if geometries["lines"]:
            line_points = vtkPoints()
            line_cells = vtkCellArray()
            line_point_colors = vtkFloatArray()
            line_point_colors.SetNumberOfComponents(4)
            line_point_colors.SetName("rgba_colors")
            line_point_normals = vtkFloatArray()
            line_point_normals.SetNumberOfComponents(3)
            line_point_normals.SetName("vertex_normals")

            pt_count = 0

            for batch in geometries["lines"]:
                num_in_batch = len(batch["values"])
                first_in_batch = True
                for coord in batch["values"]:
                    if "pos" in coord:
                        line_points.InsertNextPoint(coord["pos"])
                    if "norm" in coord:
                        line_point_normals.InsertNextTuple(coord["norm"])
                    if "col" in coord:
                        line_point_colors.InsertNextTuple(coord["col"])

                    if first_in_batch:
                        line_cells.InsertNextCell(num_in_batch)
                        first_in_batch = False

                    line_cells.InsertCellPoint(pt_count)

                    pt_count += 1

            output_lines = vtkPolyData()

            output_lines.SetPoints(line_points)
            output_lines.SetLines(line_cells)

            if line_point_colors.GetNumberOfTuples() > 0:
                output_lines.GetPointData().AddArray(line_point_colors)

            if line_point_normals.GetNumberOfTuples() > 0:
                output_lines.GetPointData().AddArray(line_point_normals)

            ds = vtkPartitionedDataSet()
            ds.SetNumberOfPartitions(1)
            ds.SetPartition(0, output_lines)

            partitioned_datasets.append(ds)
            partitioned_dataset_names.append("Lines")

        # Build the points polydata if we found points
        if geometries["points"]:
            p_points = vtkPoints()
            p_cells = vtkCellArray()
            p_point_colors = vtkFloatArray()
            p_point_colors.SetNumberOfComponents(4)
            p_point_colors.SetName("rgba_colors")
            p_point_normals = vtkFloatArray()
            p_point_normals.SetNumberOfComponents(3)
            p_point_normals.SetName("vertex_normals")

            p_count = 0

            for batch in geometries["points"]:
                num_in_batch = len(batch["values"])
                first_in_batch = True
                for coord in batch["values"]:
                    if "pos" in coord:
                        p_points.InsertNextPoint(coord["pos"])
                    if "norm" in coord:
                        p_point_normals.InsertNextTuple(coord["norm"])
                    if "col" in coord:
                        p_point_colors.InsertNextTuple(coord["col"])

                    if first_in_batch:
                        p_cells.InsertNextCell(num_in_batch)
                        first_in_batch = False

                    p_cells.InsertCellPoint(p_count)

                    p_count += 1

            output_points = vtkPolyData()

            output_points.SetPoints(p_points)
            output_points.SetVerts(p_cells)

            if p_point_colors.GetNumberOfTuples() > 0:
                output_points.GetPointData().AddArray(p_point_colors)

            if p_point_normals.GetNumberOfTuples() > 0:
                output_points.GetPointData().AddArray(p_point_normals)

            ds = vtkPartitionedDataSet()
            ds.SetNumberOfPartitions(1)
            ds.SetPartition(0, output_points)

            partitioned_datasets.append(ds)
            partitioned_dataset_names.append("Points")

        # Build the polygons if there were any
        if geometries["poly"]:
            poly_points = vtkPoints()
            poly_cells = vtkCellArray()
            poly_point_colors = vtkFloatArray()
            poly_point_colors.SetNumberOfComponents(4)
            poly_point_colors.SetName("rgba_colors")
            poly_point_normals = vtkFloatArray()
            poly_point_normals.SetNumberOfComponents(3)
            poly_point_normals.SetName("vertex_normals")

            pt_count = 0

            for batch in geometries["poly"]:
                num_in_batch = len(batch["values"])
                if num_in_batch < 1:
                    continue
                first_in_batch = True
                for coord in batch["values"]:
                    if "pos" in coord:
                        poly_points.InsertNextPoint(coord["pos"])
                    if "norm" in coord:
                        poly_point_normals.InsertNextTuple(coord["norm"])
                    if "col" in coord:
                        poly_point_colors.InsertNextTuple(coord["col"])

                    if first_in_batch:
                        np_in_cell = num_in_batch
                        poly_cells.InsertNextCell(np_in_cell)
                        first_in_batch = False

                    poly_cells.InsertCellPoint(pt_count)

                    pt_count += 1

            output_polys = vtkPolyData()

            output_polys.SetPoints(poly_points)
            output_polys.SetPolys(poly_cells)

            if poly_point_colors.GetNumberOfTuples() > 0:
                output_polys.GetPointData().AddArray(poly_point_colors)

            if poly_point_normals.GetNumberOfTuples() > 0:
                output_polys.GetPointData().AddArray(poly_point_normals)

            ds = vtkPartitionedDataSet()
            ds.SetNumberOfPartitions(1)
            ds.SetPartition(0, output_polys)

            partitioned_datasets.append(ds)
            partitioned_dataset_names.append("Polygons")

        # Add any partioned datasets we created
        output.SetNumberOfPartitionedDataSets(len(partitioned_datasets))

        for idx, pds in enumerate(partitioned_datasets):
            output.SetPartitionedDataSet(idx, pds)
            output.GetMetaData(idx).Set(vtkCompositeDataSet.NAME(), partitioned_dataset_names[idx])

        return 1
