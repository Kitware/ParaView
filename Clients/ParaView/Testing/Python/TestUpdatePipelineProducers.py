from paraview.simple import *
from paraview import print_info

def createPipeline(updateEachStep: bool):
    """Create a Pipeline with filters using different kind of ProxyProperties,
    including multiple InputProperty and non-InputProperty.

    This is an integration test of the internal procedure to retrieve producers
    of a Proxy, as used in the `UpdatePipeline` call.
    """
    # source with Vector to test NumberOfComponentsDomain update
    fastUniformGrid1 = FastUniformGrid()

    if updateEachStep:
        UpdatePipeline()

    # Set(SelectedComponent='Y') will be defered to the first pipeline update
    # as it requires input to be up to date to convert Y into numerical value
    threshold1 = Threshold()
    threshold1.Set(
        Scalars=['POINTS', 'Swirl'],
        SelectedComponent='Y',
        LowerThreshold=1.0,
        UpperThreshold=5.0,
    )

    if updateEachStep:
        UpdatePipeline()

    clip1 = Clip()
    # Clip as a ProxyProperty
    clip1.ClipType.Origin = [5.0, 0.0, 3.0]

    if updateEachStep:
        UpdatePipeline()

    # AppendDatasets has repeatable input
    appendDatasets1 = AppendDatasets(Input=[clip1, threshold1])
    UpdatePipeline()

    return appendDatasets1.GetInformation()

def TestUpdatePipelineProducers():
    """Test that updating each pipeline source leads to same result as doing
    a global final update.
    """

    print_info("TestUpdatePipelineOrder")
    stepResult = createPipeline(True)
    endResult = createPipeline(False)

    assert stepResult.number_of_points == endResult.number_of_points
    assert stepResult.number_of_cells == endResult.number_of_cells
    assert stepResult.bounds == endResult.bounds

TestUpdatePipelineProducers()
