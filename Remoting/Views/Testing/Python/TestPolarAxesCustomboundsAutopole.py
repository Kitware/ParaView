from paraview.simple import *
import PolarAxesUtils

axes = PolarAxesUtils.CreatePipeline()
PolarAxesUtils.CustomBounds(axes, bounds=[0, 1, 0, 1, 0, 1])
axes.AutoPole = 1

if __name__ == "__main__":
  PolarAxesUtils.DoBaselineComparison()
