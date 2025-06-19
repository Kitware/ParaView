from paraview.simple import *
import PolarAxesUtils

axes = PolarAxesUtils.CreatePipeline()
axes.AutoPole = 1

if __name__ == "__main__":
  PolarAxesUtils.DoBaselineComparison()
