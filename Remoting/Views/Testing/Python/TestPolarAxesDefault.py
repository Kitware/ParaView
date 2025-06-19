from paraview.simple import *
import PolarAxesUtils

polarAxes = PolarAxesUtils.CreatePipeline()

if __name__ == "__main__":
  PolarAxesUtils.DoBaselineComparison()
