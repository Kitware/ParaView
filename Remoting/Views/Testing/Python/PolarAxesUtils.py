# state file generated using paraview version 6.0.0-RC1-116-g479d675ec1
import paraview
paraview.compatibility.major = 6
paraview.compatibility.minor = 0

#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# ----------------------------------------------------------------
def CreatePipeline():
    """Create and return a PolarAxes in a simple pipeline.
    Create an Outline source around [1, 2, 1, 2, 0, 1].
    Set up parallel projection rendering for better visualization of coordinates.
    Add a PolarAxes based on the Outline source, and disable custom values
    (angles, radius and pole)."""
    renderView1 = GetActiveViewOrCreate('RenderView')
    renderView1.Set(
        InteractionMode='2D',
        CenterOfRotation=[1.5, 1.5, 0.5],
        CameraPosition=[1.5, 1.5, 3.913365056234633],
        CameraFocalPoint=[1.5, 1.5, 0.5],
        CameraParallelScale=1.0704643485939147,
        CameraParallelProjection=1,
    )

    outlineSource1 = OutlineSource(registrationName='OutlineSource1')
    outlineSource1.Bounds = [1.0, 2.0, 1.0, 2.0, 0.0, 1.0]
    outlineSource1Display = Show(outlineSource1, renderView1, 'GeometryRepresentation')
    outlineSource1Display.Set(
        Representation='Surface',
        ColorArrayName=[None, ''],
    )

    outlineSource1Display.PolarAxes.Set(
        Visibility=1,
        AutoPole=0,
        CustomMinRadius=0,
        CustomAngles=0,
    )

    SetActiveSource(outlineSource1)

    return outlineSource1Display.PolarAxes

# ----------------------------------------------------------------
def TransformBounds(polarAxes, translation = [0, 0, 0], rotation = [0, 0, 0], scale = [1, 1, 1]):
    """Apply input transform elements to the PlarAxes.
    Also create a Transform filter on active source with same parameters
    and display it with red coloration."""
    transform1 = Transform(registrationName='Transform1')
    transform1.Transform.Set(
        Translate=translation,
        Rotate=rotation,
        Scale=scale,
    )

    transform1Display = Show(transform1)
    transform1Display.Set(
        Representation='Surface',
        AmbientColor=[1.0, 0.0, 0.0],
        DiffuseColor=[1.0, 0.0, 0.0],
    )

    polarAxes.Set(
        Translation=translation,
        Scale=scale,
        Orientation=rotation,
    )

    ResetCamera()

# ----------------------------------------------------------------
def CustomBounds(polarAxes, bounds):
    """Apply custom bounds to the polarAxes.
    Also create a new Outline source with given bounds
    and display it with yellow coloration."""
    polarAxes.Set(
        EnableCustomBounds=[1, 1, 1],
        CustomBounds=bounds,
    )

    customSource = OutlineSource()
    customSource.Bounds = bounds
    customDisplay = Show()
    customDisplay.Set(
        Representation='Surface',
        AmbientColor=[1.0, 1.0, 0.0],
        DiffuseColor=[1.0, 1.0, 0.0],
    )

    ResetCamera()

# ----------------------------------------------------------------
def DoBaselineComparison():
    """Perform baseline comparison.
    Test ActiveView against the baseline found from script "-V" argv input argument. """
    from paraview.vtk.util.misc import vtkGetTempDir
    import os, sys
    from paraview.vtk.test import Testing

    try:
      baselineIndex = sys.argv.index('-V')+1
      baselinePath = sys.argv[baselineIndex]
    except:
      print("Could not get baseline directory. Test failed.")
      exit(1)

    Testing.VTK_TEMP_DIR = vtkGetTempDir()
    Testing.compareImage(GetActiveView().GetRenderWindow(), baselinePath)
    Testing.interact()
