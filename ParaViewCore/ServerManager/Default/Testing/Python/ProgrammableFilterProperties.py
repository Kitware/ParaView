import os
import random
import textwrap

from paraview import smtesting
import paraview.simple as smp


def testScript(programmableFilter, script):

    arrayName = str(random.random())
    programmableFilter.Script = script + textwrap.dedent('''
        passedArray = vtk.vtkIntArray()
        passedArray.SetName('%s')
        self.GetOutput().GetFieldData().AddArray(passedArray)''' % arrayName)

    programmableFilter.UpdatePipeline()
    return programmableFilter.GetClientSideObject().GetOutput().GetFieldData().GetArray(arrayName) is not None


smtesting.ProcessCommandLineArguments()
tempDir = smtesting.TempDir
stateDir = smtesting.SMStatesDir


sphere = smp.Sphere()

f = smp.ProgrammableFilter(sphere)


script = '''
assert 1+1 == 2
'''

assert testScript(f, script)


script = '''
assert foo == 'bar'
'''

f.SetPropertyWithName('Parameters', ['foo', '"bar"'])

assert testScript(f, script)


smp.LoadPlugin(os.path.join(stateDir, 'ProgrammableFilterPropertiesTest.xml'))

f = smp.ProgrammableFilterPropertiesTest(sphere)


assert f.DoubleTest == 1.23
assert f.IntTest == 123
assert f.StringTest == 'string value'

assert testScript(f, f.Script)
