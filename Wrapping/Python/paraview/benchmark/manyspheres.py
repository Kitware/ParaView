import datetime as dt
from paraview import servermanager
from paraview.simple import *
from paraview.benchmark import *
#import logbase, logparser

logbase.maximize_logs()
records = []
n0 = dt.datetime.now()

def get_render_view(size):
    '''Similar to GetRenderView except if a new view is created, it's
    created with the specified size instead of having t resize afterwards
    '''
    view = active_objects.view
    if not view:
        # it's possible that there's no active view, but a render view exists.
        # If so, locate that and return it (before trying to create a new one).
        view = servermanager.GetRenderView()
    if not view:
        view = CreateRenderView(ViewSize=size)
    return view


def save_render_buffer(fname):
    '''Similar to SaveScreenshot except a re-render will not be triggered'''
    from vtkmodules.vtkRenderingCore import vtkWindowToImageFilter
    w = GetRenderView().SMProxy.GetRenderWindow()
    w2i = vtkWindowToImageFilter()
    w2i.ReadFrontBufferOff()
    w2i.ShouldRerenderOff()
    w2i.SetInput(w)
    w2i.Modified()
    png = PNGWriter()
    png.Input = w2i.GetOutput()
    png.FileName = fname
    png.UpdatePipeline()


def flush_render_buffer():
    '''When running as a single process use the WindowToImage filter to
    force a framebuffer read.  This bypasses driver optimizations that
    perform lazy rendering and allows you to get actual frame rates for
    a single process with a GPU.  Multi-process doesn't need this since
    compositing forces the frame buffer read.
    '''

    # If we're not using off-screen rendering then we can bypass this since
    # the frame buffer display will force a GL flush
    w = GetRenderView().SMProxy.GetRenderWindow()
    if not w.GetOffScreenRendering():
        return

    from vtkmodules.vtkRenderingCore import vtkWindowToImageFilter
    from vtkmodules.vtkParallelCore import vtkMultiProcessController

    # If we're using MPI we can also bypass this since compositing will
    # for a GL flush
    controller = vtkMultiProcessController.GetGlobalController()
    if controller.GetNumberOfProcesses() > 1:
        return

    # Force a GL flush by retrieving the frame buffer image
    w2i = vtkWindowToImageFilter()
    w2i.ReadFrontBufferOff()
    w2i.ShouldRerenderOff()
    w2i.SetInput(w)
    w2i.Modified()
    w2i.Update()


def memtime_stamp():
    global records
    global n0
    m = logbase.get_memuse()
    n1 = dt.datetime.now()
    et = n1 - n0
    print(et, m)
    n0 = n1
    records.append([et, m])


def run(output_basename='log', num_spheres=8, num_spheres_in_scene=None,
        resolution=725, view_size=(1920, 1080), num_frames=10, save_logs=True,
        transparency=False, ospray=False):
    if num_spheres_in_scene is None:
        num_spheres_in_scene = num_spheres

    from vtkmodules.vtkParallelCore import vtkMultiProcessController
    from vtkmodules.vtkCommonSystem import vtkTimerLog

    controller = vtkMultiProcessController.GetGlobalController()

    view = get_render_view(view_size)
    if ospray:
        view.EnableRayTracing = 1

    print('Generating bounding box')
    import math
    edge = math.ceil(math.pow(num_spheres_in_scene, (1.0 / 3.0)))
    box = Box()
    box.XLength = edge
    box.YLength = edge
    box.ZLength = edge
    box.Center = [edge * 0.5, edge * 0.5, edge * 0.5]
    boxDisplay = Show()
    boxDisplay.SetRepresentationType('Outline')

    print('Generating all spheres')
    gen = ProgrammableSource(Script='''
import math
from vtkmodules.vtkParallelCore import vtkMultiProcessController
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkFiltersCore import vtkAppendPolyData
from vtkmodules.vtkCommonDataModel import vtkPolyData

try:
    num_spheres
except:
    num_spheres = 8

try:
    num_spheres_in_scene
except:
    num_spheres_in_scene = num_spheres

try:
    res
except:
    res = 725

edge = math.ceil(math.pow(num_spheres_in_scene, (1.0 / 3.0)))

controller = vtkMultiProcessController.GetGlobalController()
np = controller.GetNumberOfProcesses()
p = controller.GetLocalProcessId()

ns=lambda rank:num_spheres/np + (1 if rank >= np-num_spheres%np else 0)

# Not sure why but the builtin sum() gives weird results here so we'll just
# so it manually
start=0
for r in range(0,p):
    start += int(ns(r))
end=start+ns(p)
start = int(start)
end = int(end)

ss = vtkSphereSource()
ss.SetPhiResolution(res)
ss.SetThetaResolution(res)

ap = vtkAppendPolyData()
print('  source %d: generating %d spheres from %d to %d' % (p, end-start, start, end))
for x in range(start,end):
    i = x%edge
    j = math.floor((x / edge))%edge
    k = math.floor((x / (edge * edge)))
    ss.SetCenter(i + 0.5,j + 0.5,k + 0.5)
    ss.Update()
    pd = vtkPolyData()
    pd.ShallowCopy(ss.GetOutput())
    # pd.GetPointData().RemoveArray('Normals')
    ap.AddInputData(pd)

ap.Update()
self.GetOutput().ShallowCopy(ap.GetOutput())
''')

    paramprop = gen.GetProperty('Parameters')
    paramprop.SetElement(0, 'num_spheres_in_scene')
    paramprop.SetElement(1, str(num_spheres_in_scene))
    paramprop.SetElement(2, 'num_spheres')
    paramprop.SetElement(3, str(num_spheres))
    paramprop.SetElement(4, 'res')
    paramprop.SetElement(5, str(resolution))
    gen.UpdateProperty('Parameters')

    print('Assigning colors')
    pidScale = ProcessIdScalars()
    pidScaleDisplay = Show()
    pidScaleDisplay.SetRepresentationType('Surface')

    if transparency:
        print('Enabling 50% transparency')
        pidScaleDisplay.Opacity = 0.5

    print('Repositioning initial camera')
    c = GetActiveCamera()
    c.Azimuth(22.5)
    c.Elevation(22.5)

    print('Rendering first frame')
    Render()

    print('Saving frame 0 screenshot')
    fdigits = int(math.ceil(math.log(num_frames, 10)))
    frame_fname_fmt = output_basename + '.scene.f%(f)0' + str(fdigits) + 'd.png'
    SaveScreenshot(frame_fname_fmt % {'f': 0})

    print('Gathering geometry counts')
    vtkTimerLog.MarkStartEvent('GetViewItemStats')
    num_polys = 0
    num_points = 0
    for r in view.Representations:
        num_polys  += r.GetRepresentedDataInformation().GetNumberOfCells()
        num_points += r.GetRepresentedDataInformation().GetNumberOfPoints()
    vtkTimerLog.MarkEndEvent('GetViewItemStats')

    print('Beginning benchmark loop')
    deltaAz = 45.0 / num_frames
    deltaEl = 45.0 / num_frames
    memtime_stamp()
    fpsT0 = dt.datetime.now()
    for frame in range(1, num_frames):
        c.Azimuth(deltaAz)
        c.Elevation(deltaEl)
        Render()
        flush_render_buffer()
        memtime_stamp()
    fpsT1 = dt.datetime.now()

    if controller.GetLocalProcessId() == 0:
        if save_logs:
            # Save the arguments this was executed with
            with open(output_basename + '.args.txt', 'w') as argfile:
                argfile.write(str({
                    'output_basename': output_basename,
                    'num_spheres': num_spheres,
                    'num_spheres_in_scene': num_spheres_in_scene,
                    'resolution': resolution, 'view_size': view_size,
                    'num_frames': num_frames, 'save_logs': save_logs,
                    'transparency': transparency, 'ospray': ospray}))

            # Save the memory statistics collected
            with open(output_basename + '.mem.txt', 'w') as ofile:
                ofile.write('\n'.join([str(x) for x in records]))

        # Process frame timing statistics
        logparser.summarize_results(num_frames, (fpsT1-fpsT0).total_seconds(),
                                    num_polys, 'Polys', save_logs,
                                    output_basename)
        print('Points / Frame: %d' % (num_points))


def main(argv):
    import argparse
    parser = argparse.ArgumentParser(
        description='Benchmark ParaView geometry rendering')
    parser.add_argument('-o', '--output-basename', default='log', type=str,
                        help='Basename to use for generated output files')
    parser.add_argument('-s', '--spheres', default=100, type=int,
                        help='The total number of spheres to render')
    parser.add_argument('-n', '--spheres-in-scene', type=int,
                        help='The number of spheres in the entire scene, including those not rendered.')
    parser.add_argument('-r', '--resolution', default=4, type=int,
                        help='Theta and Phi resolution to use for the spheres')
    parser.add_argument('-v', '--view-size', default=[400, 400],
                        type=lambda s: [int(x) for x in s.split(',')],
                        help='View size used to render')
    parser.add_argument('-f', '--frames', default=10, type=int,
                        help='Number of frames')
    parser.add_argument('-t', '--transparency', action='store_true',
                        help='Enable transparency')
    parser.add_argument('-y', '--ospray', action='store_true',
                        help='Use OSPRAY to render')

    args = parser.parse_args(argv)

    options = servermanager.vtkProcessModule.GetProcessModule().GetOptions()
    url = options.GetServerURL()
    if url:
        import re
        m = re.match('([^:/]*://)?([^:]*)(:([0-9]+))?', url)
        if m.group(4):
            Connect(m.group(2), m.group(4))
        else:
            Connect(m.group(2))

    run(output_basename=args.output_basename, num_spheres=args.spheres,
        num_spheres_in_scene=args.spheres_in_scene, resolution=args.resolution,
        view_size=args.view_size, num_frames=args.frames,
        transparency=args.transparency, ospray=args.ospray)

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
