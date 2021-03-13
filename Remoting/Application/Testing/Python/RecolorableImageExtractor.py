from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

def generate_extracts(dirname):
    # state file generated using paraview version 5.9.0-642-ga1033b58a4


    # ----------------------------------------------------------------
    # setup views used in the visualization
    # ----------------------------------------------------------------

    # Create a new 'Render View'
    renderView1 = CreateView('RenderView')
    renderView1.ViewSize = [400, 400]
    renderView1.CameraPosition = [0.0, 0.0, 68.97180749800867]
    renderView1.CameraFocalDisk = 1.0
    renderView1.CameraParallelScale = 17.888714788579016

    SetActiveView(None)

    # ----------------------------------------------------------------
    # setup view layouts
    # ----------------------------------------------------------------

    # create new layout object 'Layout #1'
    layout1 = CreateLayout(name='Layout #1')
    layout1.AssignView(0, renderView1)
    layout1.SetSize(400, 400)

    # ----------------------------------------------------------------
    # restore active view
    SetActiveView(renderView1)
    # ----------------------------------------------------------------

    # ----------------------------------------------------------------
    # setup the data processing pipelines
    # ----------------------------------------------------------------

    # create a new 'Wavelet'
    wavelet1 = Wavelet(registrationName='Wavelet1')

    # ----------------------------------------------------------------
    # setup the visualization in view 'renderView1'
    # ----------------------------------------------------------------

    # show data from wavelet1
    wavelet1Display = Show(wavelet1, renderView1, 'UniformGridRepresentation')

    # get color transfer function/color map for 'RTData'
    rTDataLUT = GetColorTransferFunction('RTData')
    rTDataLUT.RGBPoints = [37.35310363769531, 0.231373, 0.298039, 0.752941, 157.0909652709961, 0.865003, 0.865003, 0.865003, 276.8288269042969, 0.705882, 0.0156863, 0.14902]
    rTDataLUT.ScalarRangeInitialized = 1.0

    # get opacity transfer function/opacity map for 'RTData'
    rTDataPWF = GetOpacityTransferFunction('RTData')
    rTDataPWF.Points = [37.35310363769531, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]
    rTDataPWF.ScalarRangeInitialized = 1

    # trace defaults for the display properties.
    wavelet1Display.Representation = 'Surface'
    wavelet1Display.ColorArrayName = ['POINTS', 'RTData']
    wavelet1Display.LookupTable = rTDataLUT

    # setup the color legend parameters for each legend in this view

    # get color legend/bar for rTDataLUT in view renderView1
    rTDataLUTColorBar = GetScalarBar(rTDataLUT, renderView1)
    rTDataLUTColorBar.Title = 'RTData'
    rTDataLUTColorBar.ComponentTitle = ''

    # set color bar visibility
    rTDataLUTColorBar.Visibility = 1

    # show color legend
    wavelet1Display.SetScalarBarVisibility(renderView1, True)

    # ----------------------------------------------------------------
    # setup color maps and opacity mapes used in the visualization
    # note: the Get..() functions create a new object, if needed
    # ----------------------------------------------------------------

    # ----------------------------------------------------------------
    # setup extractors
    # ----------------------------------------------------------------

    # create extractor
    recolorableImage1 = CreateExtractor('RecolorableImage', renderView1, registrationName='Recolorable Image1')
    # trace defaults for the extractor.
    recolorableImage1.Trigger = 'TimeStep'

    # init the 'TimeStep' selected for 'Trigger'
    recolorableImage1.Trigger.UseEndTimeStep = 1

    # init the 'Recolorable Image' selected for 'Writer'
    recolorableImage1.Writer.FileName = 'result.vtk'
    recolorableImage1.Writer.ImageResolution = [400, 400]
    recolorableImage1.Writer.Format = 'VTK'
    recolorableImage1.Writer.DataSource = wavelet1

    # ----------------------------------------------------------------
    # restore active source
    SetActiveSource(recolorableImage1)
    # ----------------------------------------------------------------

    if __name__ == '__main__':
        # generate extracts
        SaveExtracts(ExtractsOutputDirectory=dirname)


#==============================================================================
def validate_results(fname):
    from paraview import smtesting
    rv = CreateRenderView()
    rv.ViewSize = [400, 400]

    source = OpenDataFile(fname)
    display = Show()
    display.Representation = 'Slice'

    # get color transfer function/color map for 'scalars'
    scalarsLUT = GetColorTransferFunction('scalars')
    scalarsLUT.RGBPoints = [37.50877380371094, 0.231373, 0.298039, 0.752941,
                            105.91077423095703, 0.865003, 0.865003, 0.865003,
                            174.31277465820312, 0.705882, 0.0156863, 0.14902]
    scalarsLUT.ScalarRangeInitialized = 1.0

    display.ColorArrayName = ['POINTS', 'scalars']
    display.LookupTable = scalarsLUT
    display.Diffuse = 0
    display.Specular = 0
    display.Ambient =1

    Render()
    ResetCamera()
    smtesting.DoRegressionTesting(rv.SMProxy)


#==============================================================================
import os.path
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

# remove directory
dirname = smtesting.GetUniqueTempDirectory("recolorable-extractor")
print("===============================================")
print("directory", dirname)

generate_extracts(dirname)
validate_results(os.path.join(dirname, "result.vtk"))
