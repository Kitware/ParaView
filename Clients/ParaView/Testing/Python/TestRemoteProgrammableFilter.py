import os
import random
import textwrap

from paraview import servermanager
import paraview.simple as smp


# Make sure the test driver know that process has properly started
print ("Process started")


def getHost(url):
   return url.split(':')[1][2:]


def getPort(url):
   return int(url.split(':')[2])


def testScript(programmableFilter, script):

    arrayName = str(random.random())
    programmableFilter.Script = script + textwrap.dedent('''
        passedArray = vtk.vtkIntArray()
        passedArray.SetName('%s')
        passedArray.SetNumberOfTuples(1)
        passedArray.SetValue(0, 1)
        self.GetOutput().GetFieldData().AddArray(passedArray)''' % arrayName)

    programmableFilter.UpdatePipeline()
    return programmableFilter.FieldData.GetArray(arrayName) is not None


def runTest():

    options = servermanager.vtkProcessModule.GetProcessModule().GetOptions()
    url = options.GetServerURL()

    smp.Connect(getHost(url), getPort(url))

    sphere = smp.Sphere()

    f = smp.ProgrammableFilter(sphere)

    # test that vtk is imported automatically and contains the name vtkPolyData
    script = 'assert vtk.vtkPolyData'
    assert testScript(f, script)

    # test that variables can be passed using the Parameters property
    script = 'assert foo == "bar"'
    f.SetPropertyWithName('Parameters', ['foo', '"bar"'])
    assert testScript(f, script)

    smp.Disconnect()


runTest()
