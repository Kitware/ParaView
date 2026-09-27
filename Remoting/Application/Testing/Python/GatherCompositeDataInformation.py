# This test verifies that data information for composite datasets is gathered
# correctly, including the data assembly and the hierarchy. Only rank 0 serializes
# the assembly and hierarchy when information is gathered from other ranks, so this
# test ensures that
#   * in the default mode, the assembly and hierarchy reported are the ones of rank 0
#     and the (reduced) information sent by the satellite ranks can still be parsed,
#     i.e. statistics from all ranks are accounted for,
#   * information requested for a specific rank reports the assembly and hierarchy of
#     that rank,
#   * in symmetric mode, where information is never gathered across ranks, every
#     rank reports its own assembly, hierarchy, and statistics.
# It can be run on any number of ranks.

from paraview.simple import *

from vtkmodules.vtkCommonCore import VTK_PARTITIONED_DATA_SET_COLLECTION
from vtkmodules.vtkFiltersSources import vtkConeSource, vtkSphereSource

pm = servermanager.vtkProcessModule.GetProcessModule()
nranks = pm.GetNumberOfLocalPartitions()
rank = pm.GetPartitionId()
symmetric = pm.GetSymmetricMPIMode()

# The ranks that contribute to the information reported on this rank, and the rank
# whose assembly and hierarchy are reported.
contributingRanks = 1 if symmetric else nranks
assemblyRank = rank if symmetric else 0


def GetNodeNames(assembly):
    return [assembly.GetNodeName(nodeId)
            for nodeId in assembly.GetChildNodes(assembly.GetRootNode(), True)]


# Expected statistics for a single rank.
sphere = vtkSphereSource()
sphere.Update()
cone = vtkConeSource()
cone.Update()
pointsPerRank = sphere.GetOutput().GetNumberOfPoints() + cone.GetOutput().GetNumberOfPoints()
cellsPerRank = sphere.GetOutput().GetNumberOfCells() + cone.GetOutput().GetNumberOfCells()

# Every rank builds a partitioned dataset collection with two partitioned
# datasets (each with one partition on every rank). The data assembly is the same
# on every rank except for a node that is named after the rank producing it, so we
# can tell whose assembly is reported.
pdc = ProgrammableSource(registrationName="pdc")
pdc.OutputDataSetType = "vtkPartitionedDataSetCollection"
pdc.Script = """
from vtkmodules.vtkCommonDataModel import vtkDataAssembly, vtkPartitionedDataSet
from vtkmodules.vtkFiltersSources import vtkConeSource, vtkSphereSource
from vtkmodules.vtkParallelCore import vtkMultiProcessController

rank = vtkMultiProcessController.GetGlobalController().GetLocalProcessId()
output = self.GetOutput()
output.SetNumberOfPartitionedDataSets(2)
for index, (name, source) in enumerate((("sphere", vtkSphereSource()), ("cone", vtkConeSource()))):
    source.Update()
    pds = vtkPartitionedDataSet()
    pds.SetNumberOfPartitions(1)
    pds.SetPartition(0, source.GetOutput())
    output.SetPartitionedDataSet(index, pds)
    output.GetMetaData(index).Set(output.NAME(), name)

assembly = vtkDataAssembly()
assembly.SetRootNodeName("Root")
sphereNode = assembly.AddNode("Sphere")
assembly.AddDataSetIndex(sphereNode, 0)
coneNode = assembly.AddNode("Cone")
assembly.AddDataSetIndex(coneNode, 1)
assembly.AddNode("FromRank%d" % rank)
output.SetDataAssembly(assembly)
"""
pdc.UpdatePipeline()

info = pdc.GetDataInformation().DataInformation
assert info.GetCompositeDataSetType() == VTK_PARTITIONED_DATA_SET_COLLECTION, \
    "Rank %d: expected a partitioned dataset collection" % rank

# In the default mode, the statistics only add up if the information sent by every
# rank was parsed.
assert info.GetNumberOfDataSets() == 2 * contributingRanks, \
    "Rank %d: wrong number of datasets: %d != %d" % (
        rank, info.GetNumberOfDataSets(), 2 * contributingRanks)
assert info.GetNumberOfPoints() == contributingRanks * pointsPerRank, \
    "Rank %d: wrong number of points: %d != %d" % (
        rank, info.GetNumberOfPoints(), contributingRanks * pointsPerRank)
assert info.GetNumberOfCells() == contributingRanks * cellsPerRank, \
    "Rank %d: wrong number of cells: %d != %d" % (
        rank, info.GetNumberOfCells(), contributingRanks * cellsPerRank)

# The data assembly must be the expected one.
assembly = info.GetDataAssembly()
assert assembly is not None, "Rank %d: missing data assembly" % rank
assert GetNodeNames(assembly) == ["Sphere", "Cone", "FromRank%d" % assemblyRank], \
    "Rank %d: wrong data assembly nodes: %s" % (rank, GetNodeNames(assembly))
children = assembly.GetChildNodes(assembly.GetRootNode(), False)
assert [list(assembly.GetDataSetIndices(child, False)) for child in children[:2]] == \
    [[0], [1]], "Rank %d: wrong dataset indices in the data assembly" % rank

# The hierarchy must exist and describe both partitioned datasets.
hierarchy = info.GetHierarchy()
assert hierarchy is not None, "Rank %d: missing hierarchy" % rank
hierarchyNames = GetNodeNames(hierarchy)
assert "sphere" in hierarchyNames and "cone" in hierarchyNames, \
    "Rank %d: wrong hierarchy nodes: %s" % (rank, hierarchyNames)

# Information for an individual rank is only available in the default mode: every
# rank has to report valid statistics and its own assembly and hierarchy.
if not symmetric:
    for r in range(nranks):
        rankInfo = pdc.GetRankDataInformation(r)
        assert rankInfo.GetCompositeDataSetType() == VTK_PARTITIONED_DATA_SET_COLLECTION, \
            "Rank %d: expected a partitioned dataset collection" % r
        assert rankInfo.GetNumberOfDataSets() == 2, \
            "Rank %d: wrong number of datasets: %d" % (r, rankInfo.GetNumberOfDataSets())
        assert rankInfo.GetNumberOfPoints() == pointsPerRank, \
            "Rank %d: wrong number of points: %d != %d" % (
                r, rankInfo.GetNumberOfPoints(), pointsPerRank)
        assert rankInfo.GetNumberOfCells() == cellsPerRank, \
            "Rank %d: wrong number of cells: %d != %d" % (
                r, rankInfo.GetNumberOfCells(), cellsPerRank)
        assert GetNodeNames(rankInfo.GetDataAssembly()) == ["Sphere", "Cone", "FromRank%d" % r], \
            "Rank %d: wrong data assembly nodes: %s" % (r, GetNodeNames(rankInfo.GetDataAssembly()))
        rankHierarchyNames = GetNodeNames(rankInfo.GetHierarchy())
        assert "sphere" in rankHierarchyNames and "cone" in rankHierarchyNames, \
            "Rank %d: wrong hierarchy nodes: %s" % (r, rankHierarchyNames)
