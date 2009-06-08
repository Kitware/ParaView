from paraview.simple import *
import sys
import SMPythonTesting

SMPythonTesting.ProcessCommandLineArguments()
reader = ExodusIIReader(FileName=SMPythonTesting.DataDir+'/Data/can.ex2')

if len(reader.TimestepValues) != 44:
    raise SMPythonTesting.Error('Wrong amount of time steps.')

if reader.TimestepValues[0] != 0.0 or reader.TimestepValues[-1] != 0.004299988504499197:
    raise SMPythonTesting.Error('Wrong time step value.')    

fields = reader.PointVariables

if 'DISPL' not in fields:
    raise SMPythonTesting.Error('DISPL not available.')

if 'VEL' not in fields:
    raise SMPythonTesting.Error('VEL not available.')

if 'ACCL' not in fields:
    raise SMPythonTesting.Error('ACCL not available.')

fields = reader.PointVariables.Available

if 'DISPL' not in fields:
    raise SMPythonTesting.Error('DISPL not available.')

if 'VEL' not in fields:
    raise SMPythonTesting.Error('VEL not available.')

if 'ACCL' not in fields:
    raise SMPythonTesting.Error('ACCL not available.')

reader.PointVariables = ["DISPL"]
fields = reader.PointVariables

if 'DISPL' not in fields:
    raise SMPythonTesting.Error('DISPL not available.')

if 'VEL' in fields:
    raise SMPythonTesting.Error('VEL should not available.')

if 'ACCL' in fields:
    raise SMPythonTesting.Error('ACCL should not available.')

reader.UpdatePipeline()

