import datetime as dt

from paraview import servermanager
from paraview.simple import *
from paraview.benchmark import *

logbase.maximize_logs()
records = []
n0 = dt.datetime.now()

def flush_render_buffer():
    '''When running as a single process use the WindowToImage filter to
    force a framebuffer read.  This bypasses driver optimizations that
    perform lazy rendering and allows you to get actual frame rates for
    a single process with a GPU.  Multi-process doesn't need this since
    compositing forces the frame buffer read.
    '''

    # If we're not using offscreen rendering then we can bypass this since
    # the frame buffer display will force a GL flush
    w = GetRenderView().SMProxy.GetRenderWindow()
    if not w.GetOffScreenRendering():
        return

    import vtk

    # If we're using MPI we can also bybass this since compositing will
    # for a GL flush
    controller = vtk.vtkMultiProcessController.GetGlobalController()
    if controller.GetNumberOfProcesses() > 1:
        return

    # Force a GL flush by retrieving the frame buffer image
    w2i = vtk.vtkWindowToImageFilter()
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
    print et, m
    n0 = n1
    records.append([et, m])


def run(output_basename='log', num_spheres=8, num_spheres_in_scene=None,
        resolution=725, view_size=(1920, 1080), num_frames=10, save_logs=True,
        color=False):
    if num_spheres_in_scene is None:
        num_spheres_in_scene = num_spheres

    import vtk
    controller = vtk.vtkMultiProcessController.GetGlobalController()

    view = GetRenderView()
    view.ViewSize = view_size

    import math
    edge = math.ceil(math.pow(num_spheres_in_scene, (1.0 / 3.0)))
    box = Box()
    box.XLength = edge
    box.YLength = edge
    box.ZLength = edge
    box.Center = [edge * 0.5, edge * 0.5, edge * 0.5]
    boxDisplay = Show()
    boxDisplay.SetRepresentationType('Outline')

    gen = ProgrammableSource(Script='''
import math
import vtk

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

controller = vtk.vtkMultiProcessController.GetGlobalController()
np = controller.GetNumberOfProcesses()
p = controller.GetLocalProcessId()
start = (num_spheres / np) * p
end = (num_spheres / np) * (p + 1)
if (p == np - 1):
    end = num_spheres

ss = vtk.vtkSphereSource()
ss.SetPhiResolution(res)
ss.SetThetaResolution(res)

ap = vtk.vtkAppendPolyData()
print 'source',p,': generating',end - start,'spheres from',start,'to',end
for x in range(start,end):
    i = x%edge
    j = math.floor((x / edge))%edge
    k = math.floor((x / (edge * edge)))
    ss.SetCenter(i + 0.5,j + 0.5,k + 0.5)
    ss.Update()
    pd = vtk.vtkPolyData()
    pd.ShallowCopy(ss.GetOutput())
    # pd.GetPointData().RemoveArray('Normals')
    ap.AddInputData(pd)

print 'Appending PolyData objects'
ap.Update()

self.GetOutput().ShallowCopy(ap.GetOutput())
print 'NUMCELLS(', p, ',):', ap.GetOutput().GetNumberOfCells()
''')

    paramprop = gen.GetProperty('Parameters')
    paramprop.SetElement(0, 'num_spheres_in_scene')
    paramprop.SetElement(1, str(num_spheres_in_scene))
    paramprop.SetElement(2, 'num_spheres')
    paramprop.SetElement(3, str(num_spheres))
    paramprop.SetElement(4, 'res')
    paramprop.SetElement(5, str(resolution))
    gen.UpdateProperty('Parameters')
    genDisplay = Show()
    genDisplay.SetRepresentationType('Surface')

    if color:
        pidScale = ProcessIdScalars()
        pidScaleDisplay = Show()

    deltaAz = 45.0 / num_frames
    deltaEl = 45.0 / num_frames

    ResetCamera()

    # Use a dummy camera to workaround MPI bugs when directly interacting with
    # the view's camera
    c = vtk.vtkCamera()
    c.SetPosition(view.CameraPosition)
    c.SetFocalPoint(view.CameraFocalPoint)
    c.SetViewUp(view.CameraViewUp)
    c.SetViewAngle(view.CameraViewAngle)

    c.Azimuth(22.5)
    c.Elevation(22.5)
    view.CameraPosition = c.GetPosition()
    view.CameraFocalPoint = c.GetFocalPoint()
    view.CameraViewUp = c.GetViewUp()
    view.CameraViewAngle = c.GetViewAngle()

    fdigits = int(math.ceil(math.log(num_frames, 10)))
    frame_fname_fmt = output_basename + '.scene.f%(f)0' + str(fdigits) + 'd.tiff'
    SaveScreenshot(frame_fname_fmt % {'f': 0})
    memtime_stamp()
    vtk.vtkTimerLog.MarkStartEvent('GetTotalCellCount')
    num_polys = sum([r.GetRepresentedDataInformation().GetNumberOfCells() for r in view.Representations])
    vtk.vtkTimerLog.MarkEndEvent('GetTotalCellCount')

    fpsT0 = dt.datetime.now()
    for frame in range(1, num_frames):
        c.Azimuth(deltaAz)
        c.Elevation(deltaEl)
        view.CameraPosition = c.GetPosition()
        view.CameraFocalPoint = c.GetFocalPoint()
        view.CameraViewUp = c.GetViewUp()
        view.CameraViewAngle = c.GetViewAngle()
        Render()
        flush_render_buffer()
        memtime_stamp()
    fpsT1 = dt.datetime.now()

    if controller.GetLocalProcessId() == 0:
        if save_logs:
            with open(output_basename + '.args.txt', 'w') as argfile:
                argfile.write(str({
                    'output_basename': output_basename,
                    'num_spheres': num_spheres,
                    'num_spheres_in_scene':num_spheres_in_scene,
                    'resolution': resolution, 'view_size': view_size,
                    'num_frames': num_frames, 'save_logs': save_logs,
                    'color': color}))
        process_logs(num_frames, (fpsT1-fpsT0).total_seconds(), num_polys,
                     'Polys', save_logs, output_basename)


def process_logs(num_frames, num_seconds_m0, items_per_frame, item_label,
                 save_logs=False, output_basename=None):
    '''Process the timing logs to display, save, and gather stats'''

    if save_logs:
        with open(output_basename + '.mem.txt', 'w') as ofile:
            ofile.write('\n'.join([str(x) for x in records]))

    comp_rank_frame_logs = logparser.process_logs(num_frames - 1)
    if save_logs:
        logbase.dump_logs(output_basename + '.logs.raw.bin')
        with open(output_basename + '.logs.parsed.bin', 'wb') as ofile:
            import pickle
            pickle.dump(comp_rank_frame_logs, ofile)

    # Only deal with the server logs
    if 'Servers' in comp_rank_frame_logs.keys():
        rank_frame_logs = comp_rank_frame_logs['Servers']
    elif 'ClientAndServers' in comp_rank_frame_logs.keys():
        rank_frame_logs = comp_rank_frame_logs['ClientAndServers']
    else:
        rank_frame_logs = None

    print '\nStatistics:\n' + '=' * 40 + '\n'
    if rank_frame_logs:
        print 'Rank 0 Frame 0\n' + '-' * 40
        print rank_frame_logs[0][0]
        print ''
        if save_logs:
            with open(output_basename + '.stats.r0f0.txt', 'w') as ofile:
                ofile.write(str(rank_frame_logs[0][0]))

        frame_stats, summary_stats = logparser.summarize_all_logs(rank_frame_logs)
        if frame_stats:
            for f in range(0, len(frame_stats)):
                print 'Frame ' + str(f + 1) + '\n' + '-' * 40
                logparser.write_stats_to_file(frame_stats[f], outfile=sys.stdout)
                print ''
                with open(output_basename + '.stats.frame.txt', 'w') as ofile:
                    for f in range(0, len(frame_stats)):
                        ofile.write('Frame ' + str(f + 1) + '\n' + '-' * 40 + '\n')
                        logparser.write_stats_to_file(frame_stats[f], outfile=ofile)
                        ofile.write('\n')

        if summary_stats:
            print 'Frame Summary\n' + '-' * 40
            logparser.write_stats_to_file(summary_stats, outfile=sys.stdout)
            if save_logs:
                with open(output_basename + '.stats.summary.txt', 'w') as ofile:
                    logparser.write_stats_to_file(summary_stats, outfile=ofile)

    fps = (num_frames - 1) / num_seconds_m0
    ips = fps * items_per_frame
    print ''
    print 'Frames / Sec: %(fps).2f' % {'fps': fps}
    print '%(ilabel)s / Sec: %(ips)d' % {'ilabel': item_label, 'ips': int(ips)}


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
    parser.add_argument('-v', '--view-size', default=[400,400],
                        type=lambda s: [int(x) for x in s.split(',')],
                        help='View size used to render')
    parser.add_argument('-f', '--frames', default=10, type=int,
                        help='Number of frames')
    parser.add_argument('-c', '--color', action='store_true',
                        help='Enable color renderings')

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
        view_size=args.view_size, num_frames=args.frames, color=args.color)

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
