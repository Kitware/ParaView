from paraview.simple import *
import sys
from paraview import smtesting

smtesting.ProcessCommandLineArguments()
reader = ExodusIIReader(FileName=smtesting.DataDir+'/Data/can.ex2')

if len(reader.TimestepValues) != 44:
    raise smtesting.TestError('Wrong amount of time steps.')

if reader.TimestepValues[0] != 0.0 or reader.TimestepValues[-1] != 0.004299988504499197:
    raise smtesting.TestError('Wrong time step value.')

fields = reader.PointVariables

if 'DISPL' not in fields:
    raise smtesting.TestError('DISPL not available.')

if 'VEL' not in fields:
    raise smtesting.TestError('VEL not available.')

if 'ACCL' not in fields:
    raise smtesting.TestError('ACCL not available.')

fields = reader.PointVariables.Available

if 'DISPL' not in fields:
    raise smtesting.TestError('DISPL not available.')

if 'VEL' not in fields:
    raise smtesting.TestError('VEL not available.')

if 'ACCL' not in fields:
    raise smtesting.TestError('ACCL not available.')

reader.PointVariables = ["DISPL"]
fields = reader.PointVariables

if 'DISPL' not in fields:
    raise smtesting.TestError('DISPL not available.')

if 'VEL' in fields:
    raise smtesting.TestError('VEL should not available.')

if 'ACCL' in fields:
    raise smtesting.TestError('ACCL should not available.')

reader.UpdatePipeline()

