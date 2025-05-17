## Simplify RGBPoints in Python state files

Many colormaps include a large number of RGBPoints. For example, Viridis and Inferno both contain around 250 points. Previously, all of the RGBPoints would be explicitly written out to Python state files. This could be overwhelming for state files, however. For example, it may have looked like this:

```python
# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')
rTDataLUT.Set(
    RGBPoints=[
        # scalar, red, green, blue
        40.0, 0.267004, 0.004874, 0.329415,
        40.86284, 0.26851, 0.009605, 0.335427,
        # ...  ~250 lines
    ],
    # ...
)
```

Now, while saving the Python state file, we check to see if we can generate the RGB points automatically by using a preset name and a range of values. If we can generate the RGB points automatically, we call the function to do so instead of explicitly listing all RGB points. Thus, that section of the state file now looks like the following:

```python
# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')
rTDataLUT.Set(
    RGBPoints=GenerateRGBPoints(
        preset_name='Viridis (matplotlib)',
        range_min=40.0,
        range_max=260.0,
    ),
    # ...
)
```

This is much more user-friendly, since users have the opportunity to see and manually edit the min/max of the range. It is also significantly less verbose than writing out all of the points.

While saving the state file, if we cannot generate the RGB points automatically (this could happen, for example, if a user manually modifies one or more of the points), then we default to the old behavior of writing out all of the points explicitly.
