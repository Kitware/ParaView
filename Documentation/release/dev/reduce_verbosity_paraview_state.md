## Reduce verbosity in ParaView python state file


Python state files will now be less verbose. The new approach records only
properties that are used in the visualization instead of all modified
properties of a proxy.

To use the previous verbosity level choose "any *modified* properties" in the
"Properties to Trace on Create" dropdown of the Python State options dialog.


For example, in ParaView 5.13.3 the saved state of a
Clip filter will look like this:

```python
clip = Clip(Input=unstructured_grid)
clip.ClipType='Plane'
clip.HyperTreeGridClipper='Plane'
clip.Scalars=['POINTS', 'RTData']
clip.Value=170.39517211914062
```

The new aprroach takes advantage from the fact that the default value of Clip
is a Plane and that for a clip that uses a plane on an unstructured grid all
the other properties are not used

```python
clip = Clip(Input=unstructured_grid)
```

Similarly for representations
```python
wavelet1Display.Representation = 'Outline'
wavelet1Display.ColorArrayName = ['POINTS', '']
wavelet1Display.SelectNormalArray = 'None'
wavelet1Display.SelectTangentArray = 'None'
wavelet1Display.SelectTCoordArray = 'None'
wavelet1Display.TextureTransform = 'Transform2'
wavelet1Display.OSPRayScaleArray = 'RTData'
wavelet1Display.OSPRayScaleFunction = 'Piecewise Function'
wavelet1Display.Assembly = ''
wavelet1Display.SelectedBlockSelectors = ['']
wavelet1Display.SelectOrientationVectors = 'None'
wavelet1Display.ScaleFactor = 2.0
wavelet1Display.SelectScaleArray = 'RTData'
wavelet1Display.GlyphType = 'Arrow'
wavelet1Display.GlyphTableIndexArray = 'RTData'
wavelet1Display.GaussianRadius = 0.1
wavelet1Display.SetScaleArray = ['POINTS', 'RTData']
wavelet1Display.ScaleTransferFunction = 'Piecewise Function'
wavelet1Display.OpacityArray = ['POINTS', 'RTData']
wavelet1Display.OpacityTransferFunction = 'Piecewise Function'
wavelet1Display.DataAxesGrid = 'Grid Axes Representation'
wavelet1Display.PolarAxes = 'Polar Axes Representation'
wavelet1Display.ScalarOpacityUnitDistance = 1.7320508075688774
wavelet1Display.OpacityArrayName = ['POINTS', 'RTData']
wavelet1Display.ColorArray2Name = ['POINTS', 'RTData']
wavelet1Display.IsosurfaceValues = [157.0909652709961]
wavelet1Display.SliceFunction = 'Plane'
wavelet1Display.Slice = 10
wavelet1Display.SelectInputVectors = [None, '']
wavelet1Display.WriteLog = '
```

We now have

```python
wavelet1Display.Set(
    Representation='Outline',
    ColorArrayName=['POINTS', ''],
    SelectNormalArray='None',
    SelectTangentArray='None',
    SelectTCoordArray='None',
    Assembly='',
    SelectedBlockSelectors=[''],
)
```
