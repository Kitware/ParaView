from __future__ import print_function
import datetime as dt
import sys
from paraview.simple import *
import paraview


def __render(ss, v, title, nframes):
    print ('============================================================')
    print (title)
    res = []
    res.append(title)
    for phires in (500, 1000):
        ss.PhiResolution = phires
        c = v.GetActiveCamera()
        v.CameraPosition = [-3, 0, 0]
        v.CameraFocalPoint = [0, 0, 0]
        v.CameraViewUp = [0, 0, 1]
        Render()
        c1 = dt.datetime.now()
        for i in range(nframes):
            c.Elevation(0.5)
            Render()
        tpr = (dt.datetime.now() - c1).total_seconds() / nframes
        ncells = ss.GetDataInformation().GetNumberOfCells()
        print (tpr, " secs/frame")
        print (ncells, " polys")
        print (ncells/tpr, " polys/sec")

        res.append((ncells, ncells/tpr))
    return res


def run(filename=None, nframes=60):
    '''Runs the benchmark. If a filename is specified, it will write the
    results to that file as csv. The number of frames controls how many times
    a particular configuration is rendered. Higher numbers lead to more
    accurate averages.
    '''
    # Turn off progress printing
    paraview.servermanager.SetProgressPrintingEnabled(0)

    # Create a sphere source to use in the benchmarks
    ss = Sphere(ThetaResolution=1000, PhiResolution=500)
    rep = Show()
    v = Render()
    results = []

    # Start with these defaults
    # v.RemoteRenderThreshold = 0
    obj = servermanager.misc.GlobalMapperProperties()
    obj.GlobalImmediateModeRendering = 0

    # Test different configurations
    title = 'display lists, no triangle strips, solid color'
    obj.GlobalImmediateModeRendering = 0
    results.append(__render(ss, v, title, nframes))

    title = 'no display lists, no triangle strips, solid color'
    obj.GlobalImmediateModeRendering = 1
    results.append(__render(ss, v, title, nframes))

    # Color by normals
    lt = servermanager.rendering.PVLookupTable()
    rep.LookupTable = lt
    rep.ColorArrayName = "Normals"
    lt.RGBPoints = [-1, 0, 0, 1, 0.0288, 1, 0, 0]
    lt.ColorSpace = 'HSV'
    lt.VectorComponent = 0

    title = 'display lists, no triangle strips, color by array'
    obj.GlobalImmediateModeRendering = 0
    results.append(__render(ss, v, title, nframes))

    title = 'no display lists, no triangle strips, color by array'
    obj.GlobalImmediateModeRendering = 1
    results.append(__render(ss, v, title, nframes))

    if filename:
        f = open(filename, "w")
    else:
        f = sys.stdout
    print ('configuration, %d, %d' % (results[0][1][0], results[0][2][0]), file=f)
    for i in results:
        print ('"%s", %g, %g' % (i[0], i[1][1], i[2][1]), file=f)


def test_module():
    '''Simply exercises a few components of the module.'''
    maximize_logs()

    paraview.servermanager.SetProgressPrintingEnabled(0)
    ss = Sphere(ThetaResolution=1000, PhiResolution=500)
    rep = Show()
    v = Render()

    print_logs()

if __name__ == "__main__":
    if "--test" in sys.argv:
        test_module()
    else:
        run()
