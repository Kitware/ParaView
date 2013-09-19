from paraview.simple import *
import sys
from paraview import smtesting

smtesting.ProcessCommandLineArguments()
reader = ExodusIIReader(FileName=smtesting.DataDir+'/Data/can.ex2')

if len(reader.TimestepValues) != 44:
    raise smtesting.Error('Wrong amount of time steps.')

if reader.TimestepValues[0] != 0.0 or reader.TimestepValues[-1] != 0.004299988504499197:
    raise smtesting.Error('Wrong time step value.')

fields = reader.PointVariables

if 'DISPL' not in fields:
    raise smtesting.Error('DISPL not available.')

if 'VEL' not in fields:
    raise smtesting.Error('VEL not available.')

if 'ACCL' not in fields:
    raise smtesting.Error('ACCL not available.')

fields = reader.PointVariables.Available

if 'DISPL' not in fields:
    raise smtesting.Error('DISPL not available.')

if 'VEL' not in fields:
    raise smtesting.Error('VEL not available.')

if 'ACCL' not in fields:
    raise smtesting.Error('ACCL not available.')

reader.PointVariables = ["DISPL"]
fields = reader.PointVariables

if 'DISPL' not in fields:
    raise smtesting.Error('DISPL not available.')

if 'VEL' in fields:
    raise smtesting.Error('VEL should not available.')

if 'ACCL' in fields:
    raise smtesting.Error('ACCL should not available.')

reader.UpdatePipeline()

