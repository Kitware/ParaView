from paraview.simple import *
import sys
import SMPythonTesting

servermanager.ToggleProgressPrinting()

hasnumpy = True
try:
  from numpy import *
except ImportError:
  hasnumpy = False

if hasnumpy:
    # Apply a Python calculator that extracts the first component of Normals
    s = Sphere()
    pc = PythonCalculator(Expression="Normals[:, 0]")
    pc.UpdatePipeline()

    # Fetch the data and compare values
    s_a = servermanager.Fetch(s).GetPointData().GetArray('Normals')

    pc_a = servermanager.Fetch(pc).GetPointData().GetArray('result')

    for i in range(10):
        if s_a.GetValue(i*3) != pc_a.GetValue(i):
            raise SMPythonTesting.Error("Extracted component %d does not match original") % i

    # Try the same with the programmable filter
    pf = ProgrammableFilter(s)
    pf.Script = """
    output.PointData.append(inputs[0].PointData['Normals'][:, 0], 'result')
    """
    pf.UpdatePipeline()

    pf_a = servermanager.Fetch(pf).GetPointData().GetArray('result')

    for i in range(10):
        if s_a.GetValue(i*3) != pf_a.GetValue(i):
            raise SMPythonTesting.Error("Extracted component %d does not match original") % i

    # if not SMPythonTesting.DoRegressionTesting(ren.SMProxy):
    #     raise SMPythonTesting.Error('Image comparison failed.')

