r"""
This script is miniapp that acts as a sim code. It uses `vtkRTAnalyticSource`
to generate a temporal dataset which are treated as the simulation generate
data. One can pass command line arguments to control the parameters of the
generated data. Additional command line arguments can be passed to provide a
Catalyst co-processing script for in situ processing.
"""
from __future__ import absolute_import

def GenerateDataSet(opts, timestep):
    from vtk.vtkImagingCore import vtkRTAnalyticSource
    from math import sin
    exts = opts.size-1
    src = vtkRTAnalyticSource()
    src.SetWholeExtent(0, exts, 0, exts, 0, exts)
    src.SetCenter(exts / 2.0, exts / 2.0, exts / 2.0)
    src.SetMaximum(255 + 200 * sin(timestep))
    src.Update()
    return src.GetOutputDataObject(0)


from time import sleep
import argparse

parser = argparse.ArgumentParser(\
        description="Wavelet MiniApp for Catalyst testing")
parser.add_argument("-t", "--timesteps", type=int,
        help="number of timesteps to run the miniapp for (default: 100)", default=100)
parser.add_argument("--size",  type=int,
        help="number of samples in each coordinate direction (default: 100)", default=100)
parser.add_argument("-s", "--script",
        help="path to the Catalyst script to use for in situ processing",
        required=True)
parser.add_argument("-d", "--delay", type=float,
        help="delay (in seconds) between timesteps (default: 1.0)", default=1.0)


args = parser.parse_args()

from paraview.catalyst import basic_adapter
basic_adapter.initialize()
basic_adapter.add_catalyst_pipeline(args.script)

for timestep in range(args.timesteps):
    time = timestep / float(args.timesteps)
    print("timestep %d of %d (time=%f)" % (timestep, args.timesteps, time))

    dataset = GenerateDataSet(args, timestep)
    basic_adapter.coprocess(timestep, time, dataset)
    sleep(args.delay)

basic_adapter.finalize()
