#### import the simple module from the paraview
from paraview.simple import *
from paraview import servermanager


def get_data_sets(data_object):
    if data_object.IsA("vtkDataSet"):
        yield data_object
        return
    iterator = data_object.NewIterator()
    iterator.SkipEmptyNodesOn()
    iterator.InitTraversal()
    while not iterator.IsDoneWithTraversal():
        yield iterator.GetCurrentDataObject()
        iterator.GoToNextItem()


def check_global_point_ids(data_object, output_type):
    # Points with the same global id must be at the same position, and the other way around.
    # Numbering each data set on its own would give the same ids to unrelated points.
    id_to_position = {}
    position_to_id = {}
    number_of_data_sets = 0
    for data_set in get_data_sets(data_object):
        number_of_data_sets += 1
        global_ids = data_set.GetPointData().GetGlobalIds()
        assert global_ids is not None, f"{output_type}: no global point ids"
        for point_id in range(data_set.GetNumberOfPoints()):
            global_id = int(global_ids.GetTuple1(point_id))
            position = data_set.GetPoint(point_id)
            assert id_to_position.setdefault(global_id, position) == position, (
                f"{output_type}: global point id {global_id} is at {id_to_position[global_id]} "
                f"and at {position}"
            )
            assert position_to_id.setdefault(position, global_id) == global_id, (
                f"{output_type}: position {position} has the global point ids "
                f"{position_to_id[position]} and {global_id}"
            )
    assert number_of_data_sets == 2, f"{output_type}: {number_of_data_sets} data sets"


# 2 different wavelets that share some points.
wavelet1 = Wavelet(WholeExtent=[-5, 5, -5, 5, -5, 5])
wavelet2 = Wavelet(WholeExtent=[0, 8, 0, 8, 0, 8])

for output_type in [
    "vtkMultiBlockDataSet",
    "vtkPartitionedDataSet",
    "vtkPartitionedDataSetCollection",
]:
    group = GroupDatasets(Input=[wavelet1, wavelet2], OutputType=output_type)
    globalIds = GlobalPointAndCellIds(Input=group)
    globalIds.UpdatePipeline()
    check_global_point_ids(servermanager.Fetch(globalIds), output_type)
