# This test verifies that data information for composite datasets is gathered
# correctly from a server with multiple ranks and delivered to the client.
# Only rank 0 serializes the data assembly and hierarchy, so this test ensures
# that
#   * the assembly and hierarchy the client sees are the ones of rank 0,
#   * the (reduced) information sent by the satellite ranks can still be parsed,
#     i.e. statistics from all ranks are accounted for,
#   * information requested for a specific rank reports the assembly and hierarchy of
#     that rank.
# It must be run against a server with 2 or more ranks.

from paraview import servermanager
import paraview.simple as smp
from vtkmodules.vtkCommonCore import VTK_PARTITIONED_DATA_SET_COLLECTION
from vtkmodules.vtkFiltersSources import vtkConeSource, vtkSphereSource

# Make sure the test driver know that process has properly started
print("Process started")


def getHost(url):
    return url.split(':')[1][2:]


def getPort(url):
    return int(url.split(':')[2])


def getNodeNames(assembly):
    return [assembly.GetNodeName(nodeId)
            for nodeId in assembly.GetChildNodes(assembly.GetRootNode(), True)]


def runTest():
    options = servermanager.vtkRemotingCoreConfiguration.GetInstance()
    url = options.GetServerURL()

    smp.Connect(getHost(url), getPort(url))

    session = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSession()
    nranks = session.GetNumberOfProcesses(session.DATA_SERVER)
    assert nranks > 1, "Test must be run on 2 or more ranks!"

    # Expected statistics for a single rank.
    sphere = vtkSphereSource()
    sphere.Update()
    cone = vtkConeSource()
    cone.Update()
    pointsPerRank = sphere.GetOutput().GetNumberOfPoints() + cone.GetOutput().GetNumberOfPoints()
    cellsPerRank = sphere.GetOutput().GetNumberOfCells() + cone.GetOutput().GetNumberOfCells()

    # Every rank builds a partitioned dataset collection with two partitioned
    # datasets (each with one partition on every rank). The data assembly is the
    # same on every rank except for a node that is named after the rank producing
    # it, so we can tell whose assembly the client sees.
    pdc = smp.ProgrammableSource(registrationName="pdc")
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
        "Expected a partitioned dataset collection"

    # The statistics only add up if the information sent by every rank was parsed.
    assert info.GetNumberOfDataSets() == 2 * nranks, \
        "Wrong number of datasets: %d != %d" % (info.GetNumberOfDataSets(), 2 * nranks)
    assert info.GetNumberOfPoints() == nranks * pointsPerRank, \
        "Wrong number of points: %d != %d" % (info.GetNumberOfPoints(), nranks * pointsPerRank)
    assert info.GetNumberOfCells() == nranks * cellsPerRank, \
        "Wrong number of cells: %d != %d" % (info.GetNumberOfCells(), nranks * cellsPerRank)

    # The data assembly must be the one of rank 0.
    assembly = info.GetDataAssembly()
    assert assembly is not None, "Missing data assembly"
    assert getNodeNames(assembly) == ["Sphere", "Cone", "FromRank0"], \
        "Wrong data assembly nodes: %s" % getNodeNames(assembly)
    children = assembly.GetChildNodes(assembly.GetRootNode(), False)
    assert [list(assembly.GetDataSetIndices(child, False)) for child in children[:2]] == \
        [[0], [1]], "Wrong dataset indices in the data assembly"

    # The hierarchy must exist and describe both partitioned datasets.
    hierarchy = info.GetHierarchy()
    assert hierarchy is not None, "Missing hierarchy"
    hierarchyNames = getNodeNames(hierarchy)
    assert "sphere" in hierarchyNames and "cone" in hierarchyNames, \
        "Wrong hierarchy nodes: %s" % hierarchyNames

    # Information for an individual rank: every rank has to report valid statistics and
    # its own assembly and hierarchy.
    for rank in range(nranks):
        rankInfo = pdc.GetRankDataInformation(rank)
        assert rankInfo.GetCompositeDataSetType() == VTK_PARTITIONED_DATA_SET_COLLECTION, \
            "Rank %d: expected a partitioned dataset collection" % rank
        assert rankInfo.GetNumberOfDataSets() == 2, \
            "Rank %d: wrong number of datasets: %d" % (rank, rankInfo.GetNumberOfDataSets())
        assert rankInfo.GetNumberOfPoints() == pointsPerRank, \
            "Rank %d: wrong number of points: %d != %d" % (
                rank, rankInfo.GetNumberOfPoints(), pointsPerRank)
        assert rankInfo.GetNumberOfCells() == cellsPerRank, \
            "Rank %d: wrong number of cells: %d != %d" % (
                rank, rankInfo.GetNumberOfCells(), cellsPerRank)
        assert getNodeNames(rankInfo.GetDataAssembly()) == \
            ["Sphere", "Cone", "FromRank%d" % rank], \
            "Rank %d: wrong data assembly nodes: %s" % (rank, getNodeNames(rankInfo.GetDataAssembly()))
        rankHierarchyNames = getNodeNames(rankInfo.GetHierarchy())
        assert "sphere" in rankHierarchyNames and "cone" in rankHierarchyNames, \
            "Rank %d: wrong hierarchy nodes: %s" % (rank, rankHierarchyNames)

    smp.Disconnect()


runTest()
