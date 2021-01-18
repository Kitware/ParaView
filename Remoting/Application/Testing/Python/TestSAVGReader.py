from paraview import simple
from paraview import smtesting

smtesting.ProcessCommandLineArguments()

savg_file_path = smtesting.DataDir + '/Testing/Data/test.savg'

reader = simple.SAVGReader(FileName=savg_file_path)

view = simple.CreateView('RenderView')
view.ViewSize = [300, 300]
view.CameraPosition = [2.001753664815067, -1.9031225547829735, 2.5909229860343705]
view.CameraFocalPoint = [0.20133052026610077, -0.011765732546725525, -0.045838942383670714]
view.CameraViewUp = [-0.5327942053471628, 0.4720135613104274, 0.7023770587708092]
view.CameraFocalDisk = 1.0
view.CameraParallelScale = 0.9604686396486893

rep = simple.Show(reader, view, 'GeometryRepresentation')
rep.Representation = 'Surface'
rep.ColorArrayName = ['POINTS', 'rgba_colors']
rep.MapScalars = 0
rep.PointSize = 4.0
rep.LineWidth = 2.0

simple.Render(view)

if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError('Image comparison failed.')
