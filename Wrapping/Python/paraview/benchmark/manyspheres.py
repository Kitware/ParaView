import sys
import argparse
import math
import pickle
import datetime as dt

from paraview.simple import *
import vtk

import paraview.benchmark as bm
from . import logparser


bm.maximize_logs()
records = []
n0 = dt.datetime.now()


def memtime_stamp():
    global records
    global n0
    m = bm.get_memuse()
    n1 = dt.datetime.now()
    et = n1 - n0
    print et, m
    n0 = n1
    records.append([et, m])


def run(output_basename='log', num_spheres=8, num_spheres_in_scene=None,
        resolution=725, view_size=(1920, 1080), num_frames=10, save_logs=True):

    if num_spheres_in_scene is None:
        num_spheres_in_scene = num_spheres

    if save_logs:
        with open(output_basename + '.args.txt', 'w') as memfile:
            memfile.write(str(n0) + '\n')
            memfile.write(str({
                'output_basename': output_basename, 'num_spheres': num_spheres,
                'num_spheres_in_scene':num_spheres_in_scene,
                'resolution': resolution, 'view_size': view_size,
                'num_frames': num_frames, 'save_logs': save_logs}))

    view = GetRenderView()
    view.ViewSize = view_size

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
np = 1
p = 0
try:
    from mpi4py import MPI
    p = MPI.COMM_WORLD.Get_rank()
    np = MPI.COMM_WORLD.Get_size()
    print 'p,np', p,np
except:
    pass
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

    pidScale = ProcessIdScalars()
    pidLUT = GetColorTransferFunction('ProcessId')
    pidScaleDisplay = Show()
    pidScaleDisplay.ColorArrayName = ['POINTS', 'ProcessId']
    pidScaleDisplay.LookupTable = pidLUT
    pidScaleDisplay.SetScaleArray = ['POINTS', 'ProcessId']

    c = GetActiveCamera()
    c.Azimuth(22.5)
    c.Elevation(22.5)
    deltaAz = 45.0 / num_frames
    deltaEl = 45.0 / num_frames
    # deltaZm = math.pow(1.5, 1.0 / num_frames)
    deltaZm = 1

    Show()
    ResetCamera()
    fdigits = int(math.ceil(math.log(num_frames, 10)))
    frame_fname_fmt = output_basename + '.scene.f%(f)0' + str(fdigits) + 'd.tiff'

    vtk.vtkTimerLog.MarkStartEvent('pvbatch::RenderAndWrite')
    SaveScreenshot(frame_fname_fmt % {'f': 0})
    vtk.vtkTimerLog.MarkEndEvent('pvbatch::RenderAndWrite')
    memtime_stamp()

    for frame in range(1, num_frames):
        c.Azimuth(deltaAz)
        c.Elevation(deltaEl)
        c.Zoom(deltaZm)
        vtk.vtkTimerLog.MarkStartEvent('pvbatch::RenderAndWrite')
        SaveScreenshot(frame_fname_fmt % {'f': frame})
        vtk.vtkTimerLog.MarkEndEvent('pvbatch::RenderAndWrite')
        memtime_stamp()

    if save_logs:
        with open(output_basename + '.mem.txt', 'w') as memfile:
            memfile.write('\n'.join([str(x) for x in records]))

    rank_frame_logs = logparser.process_logs(num_frames - 1)
    if save_logs:
        pickle.dump(rank_frame_logs, open(output_basename + '.logs.bin', 'wb'))

    print '\nStatistics:\n' + '=' * 40 + '\n'

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
        if save_logs:
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

        # Render time is pvbatch::RenderAndWrite - vtkSMUtilities::SaveImage
        spf = summary_stats[0]['Stats'].Mean - summary_stats[1][2]['Stats'].Mean
        print '\nFPS: %(fps).2f' % {'fps': 1.0 / spf}
