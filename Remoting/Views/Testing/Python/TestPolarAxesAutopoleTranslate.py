from paraview.simple import *
import PolarAxesUtils

axes = PolarAxesUtils.CreatePipeline()
axes.AutoPole = 1
PolarAxesUtils.TransformBounds(axes, translation = [2, 0, 0])

if __name__ == "__main__":
  PolarAxesUtils.DoBaselineComparison()
