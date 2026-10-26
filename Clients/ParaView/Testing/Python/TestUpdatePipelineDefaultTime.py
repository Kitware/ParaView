from paraview.simple import *
paraview.simple._DisableFirstRenderCameraReset()

from paraview import print_info

from vtkmodules.vtkCommonCore import vtkLogger
from paraview.modules.vtkPVVTKExtensionsCore import vtkPVLogger

def pipeline(usetime: bool, useproxy: bool):
    """Create a fresh temporal pipeline, update it and then show it.

    The arguments usetime and useproxy allows to select wich variant of `UpdatePipeline()`
    should be called.
    Whichever UpdatePipeline is used, calling `Show` thereafter should not trigger an update again.
    This is ensured by the error message logged from the programmable filter:
    while verbosity is off during `UpdatePipeline` call, it is restored before the `Show` leading
    to a test failure if the filter is executed again.

    This test non regression of #23440.
    """
    print_info(f"Run case usetime = {usetime}, useproxy = {useproxy}")
    ResetSession()
    Wavelet()

    tsg=GenerateTimeSteps()
    # use a random value: avoid 0 in case it is used somewhere as default for "no time provided".
    tsg.TimeStepValues = [1.42]

    # this filter will print error when executed, making the test fail
    tester = ProgrammableFilter()
    tester.CopyArrays = True
    tester.Script = """import vtk
vtk.vtkLogger.Log(vtk.vtkLogger.VERBOSITY_ERROR, "ProgFilter RequestData", 0, "ERROR: Filter is updated")
"""

    f=PointDatatoCellData()

    # disable logs on first execution to skip the first ProgrammableFilter error message
    vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_OFF)

    # get current time: be sure the timekeeper knows about our pipeline
    scene = GetAnimationScene()
    scene.UpdateAnimationUsingDataTimeSteps()
    tk = GetTimeKeeper()
    time = tk.Time
    if usetime and useproxy:
        f.UpdatePipeline(time)
    elif usetime:
        UpdatePipeline(time)
    elif useproxy:
        f.UpdatePipeline(time)
    else:
        UpdatePipeline()

    # re-enable logs, so if the pipeline is updated again an error will show up
    vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_INFO)
    Show()
    v=Render()


# Test all 4 combinations
pipeline(False, False)
pipeline(False, True)
pipeline(True, False)
pipeline(True, True)
