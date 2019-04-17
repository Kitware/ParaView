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

    from vtkmodules.vtkParallelCore import vtkMultiProcessController
    from vtkmodules.vtkRenderingCore import vtkWindowToImageFilter

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


def run(output_basename='log', dimension=100, view_size=(1920, 1080),
        num_frames=10, save_logs=True, ospray=False):

    from vtkmodules.vtkParallelCore import vtkMultiProcessController
    from vtkmodules.vtkCommonSystem import vtkTimerLog

    controller = vtkMultiProcessController.GetGlobalController()

    view = get_render_view(view_size)
    if ospray:
        view.EnableRayTracing = 1

    print('Generating wavelet')
    wavelet = Wavelet()
    d2 = dimension//2
    wavelet.WholeExtent = [-d2, d2, -d2, d2, -d2, d2]
    wavelet.Maximum = 100.0
    waveletDisplay = Show()
    waveletDisplay.SetRepresentationType('Volume')

    print('Repositioning initial camera')
    c = GetActiveCamera()
    c.Azimuth(22.5)
    c.Elevation(22.5)

    print('Rendering first frame')
    Render()

    print('Saving frame 0 screenshot')
    import math
    fdigits = int(math.ceil(math.log(num_frames, 10)))
    frame_fname_fmt = output_basename + '.scene.f%(f)0' + str(fdigits) + 'd.png'
    SaveScreenshot(frame_fname_fmt % {'f': 0})

    print('Gathering geometry counts')
    vtkTimerLog.MarkStartEvent('GetViewItemStats')
    num_voxels = 0
    for r in view.Representations:
        num_voxels += r.GetRepresentedDataInformation().GetNumberOfCells()
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
                    'dimension': dimension,
                    'view_size': view_size,
                    'num_frames': num_frames,
                    'ospray' : ospray,
                    'save_logs': save_logs}))

            # Save the memory statistics collected
            with open(output_basename + '.mem.txt', 'w') as ofile:
                ofile.write('\n'.join([str(x) for x in records]))

        # Process frame timing statistics
        logparser.summarize_results(num_frames, (fpsT1-fpsT0).total_seconds(),
                                    num_voxels, 'Voxels', save_logs,
                                    output_basename)


def main(argv):
    import argparse
    parser = argparse.ArgumentParser(
        description='Benchmark ParaView geometry rendering')
    parser.add_argument('-o', '--output-basename', default='log', type=str,
                        help='Basename to use for generated output files')
    parser.add_argument('-d', '--dimension', default=100, type=int,
                        help='The dimension of each side of the cubic volume')
    parser.add_argument('-v', '--view-size', default=[400, 400],
                        type=lambda s: [int(x) for x in s.split(',')],
                        help='View size used to render')
    parser.add_argument('-f', '--frames', default=10, type=int,
                        help='Number of frames')
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

    run(output_basename=args.output_basename, dimension=args.dimension,
        view_size=args.view_size, num_frames=args.frames, ospray=args.ospray)

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
