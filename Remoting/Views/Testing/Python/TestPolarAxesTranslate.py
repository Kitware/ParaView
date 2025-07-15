from paraview.simple import *
import PolarAxesUtils

polarAxes = PolarAxesUtils.CreatePipeline()
PolarAxesUtils.TransformBounds(polarAxes, translation = [2, 0, 0])

if __name__ == "__main__":
  PolarAxesUtils.DoBaselineComparison()
