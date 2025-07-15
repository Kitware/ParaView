from paraview.simple import *
import PolarAxesUtils

polarAxes = PolarAxesUtils.CreatePipeline()
PolarAxesUtils.TransformBounds(polarAxes, rotation = [0, 0, 45])

if __name__ == "__main__":
  PolarAxesUtils.DoBaselineComparison()
