from paraview import simple
from paraview import smtesting

renderView = simple.CreateView('RenderView')

renderView.AxesGrid = 'GridAxes3DActor'
renderView.AxesGrid.Visibility = 1
renderView.AxesGrid.XTitleColor = [0.0, 0.0, 0.0]
renderView.AxesGrid.YTitleColor = [0.0, 0.0, 0.0]
renderView.AxesGrid.ZTitleColor = [0.0, 0.0, 0.0]
renderView.AxesGrid.GridColor = [0.0, 0.0, 0.0]
renderView.AxesGrid.ShowGrid = 1
renderView.AxesGrid.XLabelColor = [0.0, 0.0, 0.0]
renderView.AxesGrid.YLabelColor = [0.0, 0.0, 0.0]
renderView.AxesGrid.ZLabelColor = [0.0, 0.0, 0.0]
renderView.AxesGrid.DataScale = [ 1, 1, 1 ]

renderView.OrientationAxesVisibility = 0
renderView.Background = [1.0, 1.0, 1.0]

simple.Render(renderView)

if not smtesting.DoRegressionTesting(renderView.SMProxy):
    raise smtesting.TestError('Image comparison failed.')
