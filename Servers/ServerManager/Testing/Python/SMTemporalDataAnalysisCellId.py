# Tests temporal data analysis filter, generating a temporal plot for 
# cell picked by Id.

import os
import os.path
import sys

if os.name == "posix":
  from libvtkPVServerCommonPython import *
  from libvtkPVServerManagerPython import *
else:
  from vtkPVServerCommonPython import *
  from vtkPVServerManagerPython import *
import SMPythonTesting
  
SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "SMTemporalDataAnalysisCellId.pvsm")
print "State file: %s" % pvsm_file

SMPythonTesting.LoadServerManagerState(pvsm_file)
pxm = vtkSMObject.GetProxyManager()

proxy = pxm.GetProxy("displays","Sources.DataAnalysis0.TemporalXYPlotDisplay")
proxy.GenerateTemporalPlot()


reader = pxm.GetProxy("sources","XMLStructuredGridReader0")
prop = reader.GetProperty("FileName")
prop.SetElement(0, prop.GetDomain("files").GetString(0))
reader.UpdateVTKObjects()

rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
rmProxy.StillRender()
if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)
