## Track Proxy Properties Defaults

Defaults for proxy properties are now tracked so that they may be excluded from Python state files when the Python trace is set to record only modified properties. This results in a reduction of the content in Python state files.

For a simple example where I created a wavelet, a contour, and a couple of extractors, the Python state file size was reduced from 215 lines to 199 lines. For example, the renderView was changed from this:

```python
renderView1.Set(
    AxesGrid='Grid Axes 3D Actor',
    StereoType=0,
    CameraPosition=[0.0, -0.3340263366699219, 66.1845775207595],
    CameraFocalPoint=[0.0, -0.3340263366699219, 0.0],
    CameraFocalDisk=1.0,
    CameraParallelScale=17.129829154436734,
    LegendGrid='Legend Grid Actor',
    PolarGrid='Polar Grid Actor',
)
```

to this (three default proxy properties were removed):

```python
renderView1.Set(
    StereoType=0,
    CameraPosition=[0.0, -0.3340263366699219, 66.1845775207595],
    CameraFocalPoint=[0.0, -0.3340263366699219, 0.0],
    CameraFocalDisk=1.0,
    CameraParallelScale=17.129829154436734,
)
```

Several other default proxy properties were likewise removed from
other functions. For example, `PointMergeMethod='Uniform Binning'`
was removed from the contour settings, the following were all
removed from the geometry representation:

```python
    TextureTransform='Transform2',
    OSPRayScaleFunction='Piecewise Function',
    GlyphType='Arrow',
    ScaleTransferFunction='Piecewise Function',
    OpacityTransferFunction='Piecewise Function',
    DataAxesGrid='Grid Axes Representation',
    PolarAxes='Polar Axes Representation',
```

And `Trigger = 'Time Step'` was removed from the extractors.
