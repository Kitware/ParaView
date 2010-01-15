# A simple python implementation of the test driver.

import vtk, libvtkCPTestDriverPython

FieldFunction = libvtkCPTestDriverPython.vtkCPLinearScalarFieldFunction()
FieldFunction.SetConstant(2)
FieldFunction.SetTimeMultiplier(.1)
FieldFunction.SetYMultiplier(23)

FieldBuilder = libvtkCPTestDriverPython.vtkCPNodalFieldBuilder()
FieldBuilder.SetArrayName("Velocity")
FieldBuilder.SetTensorFieldFunction(FieldFunction)

GridBuilder = libvtkCPTestDriverPython.vtkCPUniformGridBuilder()
GridBuilder.SetDimensions((50, 50, 50))
GridBuilder.SetSpacing((.2, .3, .3))
GridBuilder.SetOrigin((10, 20, 300))
GridBuilder.SetFieldBuilder(FieldBuilder)

TestDriver = libvtkCPTestDriverPython.vtkCPTestDriver()
TestDriver.SetNumberOfTimeSteps(100)
TestDriver.SetGridBuilder(GridBuilder)

TestDriver.Run()

print "finished"

  
#if __name__ == "__main__":
#  global testvtkcpCoreIsRun
#  testvtkcpCoreIsRun = 0
#  cases = [(TestCoProcessing, 'test')]
#  del TestCoProcessing
#  Testing.main(cases)
