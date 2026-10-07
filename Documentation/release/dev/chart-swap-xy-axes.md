## Line and point charts can swap X and Y axes

The **Line Chart View** and **Point Chart View** now have a **Swap X/Y Axes** property. When checked, the **X Array** (or the index, when **Use Index For X Axis** is checked) is plotted along the vertical axis and all the selected series are plotted along the horizontal axis. Previously, the independent variable could be shown along the vertical axis only by plotting a single dependent variable as the **X Array**. You can now plot several dependent variables against an independent variable shown along the vertical axis, e.g., temperature and salinity profiles over depth.

When swapped, the time marker is shown along the vertical axis if the **X Array** is named `Time`. The axes keep their own properties, so the **Left Axis** properties now apply to the independent variable and the **Bottom Axis** properties apply to the series.

In Python, set the `SwapXYAxes` property on the view:

```python
lineChartView1 = CreateView('XYChartView')
lineChartView1.SwapXYAxes = 1
```
