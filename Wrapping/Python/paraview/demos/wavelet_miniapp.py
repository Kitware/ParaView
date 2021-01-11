r"""
This script is miniapp that acts as a sim code. It uses `vtkRTAnalyticSource`
to generate a temporal dataset which are treated as the simulation generate
data. One can pass command line arguments to control the parameters of the
generated data. Additional command line arguments can be passed to provide a
Catalyst co-processing script for in situ processing.

This is a good demo for integration Catalyst support in any Python-based
simulation code. Look for use of the 'bridge' module from 'paraview.catalyst'
in the code to see how.
"""

import math, argparse, time, os.path

#----------------------------------------------------------------
# parse command line arguments
parser = argparse.ArgumentParser(\
    description="Wavelet MiniApp for Catalyst testing")
parser.add_argument("-t", "--timesteps", type=int,
    help="number of timesteps to run the miniapp for (default: 100)", default=100)
parser.add_argument("--size",  type=int,
    help="number of samples in each coordinate direction (default: 101)", default=101)
parser.add_argument("-s", "--script", type=str, action="append",
    help="path(s) to the Catalyst script(s) to use for in situ processing. Can be a "
    ".py file or a Python package zip or directory",
    required=True)
parser.add_argument("--script-version", type=int,
    help="choose Catalyst analysis script version explicitly, otherwise it "
    "will be determined automatically. When specifying multiple scripts, this "
    "setting applies to all scripts.", default=0)
parser.add_argument("-d", "--delay", type=float,
    help="delay (in seconds) between timesteps (default: 0.0)", default=0.0)
parser.add_argument("-c", "--channel", type=str,
    help="Catalyst channel name (default: input)", default="input")

#----------------------------------------------------------------
# A helper function that creates a VTK dataset per timestep/per rank.
def create_dataset(timestep, args, piece, npieces):

    # We'll use vtkRTAnalyticSource to generate our dataset
    # to keep things simple.
    from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
    wavelet = vtkRTAnalyticSource()
    ext = (args.size - 1) // 2
    wavelet.SetWholeExtent(-ext, ext, -ext, ext, -ext, ext)
    wholeExtent = wavelet.GetWholeExtent()

    # put in some variation in the point data that changes with timestep
    wavelet.SetMaximum(255+200*math.sin(timestep))

    # using 'UpdatePiece' lets us generate a subextent based on the
    # 'piece' and 'npieces'; thus works seamlessly in distributed and
    # non-distributed modes
    wavelet.UpdatePiece(piece, npieces, 0)

    # typically, here you'll have some adaptor code that converts your
    # simulation data into a vtkDataObject subclass. In this example,
    # there's nothing to do since we're directly generating a
    # vtkDataObject.
    dataset = wavelet.GetOutputDataObject(0)

    return (dataset, wholeExtent)

#----------------------------------------------------------------
# Here's our simulation main loop
def main(args):
    """The main loop"""

    # initialize Catalyst
    from paraview.catalyst import bridge
    from paraview import print_info, print_warning
    bridge.initialize()

    # add analysis script
    for script in args.script:
        bridge.add_pipeline(script, args.script_version)

    # Some MPI related stuff to figure out if we're running with MPI
    # and if so, on how many ranks.
    try:
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        rank = comm.Get_rank()
        num_ranks = comm.Get_size()
    except ImportError:
        print_warning("missing mpi4py, running in serial (non-distributed) mode")
        rank = 0
        num_ranks = 1

    numsteps = args.timesteps
    for step in range(numsteps):
        if args.delay > 0:
            time.sleep(args.delay)

        # assume simulation time starts at 0
        timevalue = step/float(numsteps)

        if rank == 0:
            print_info("timestep: {0}/{1}; timevalue: {2}".format(step+1, numsteps, timevalue))


        dataset, wholeExtent = create_dataset(step, args, rank, num_ranks)

        # "perform" coprocessing.  results are outputted only if
        # the passed in script says we should at timevalue/step
        bridge.coprocess(timevalue, step, dataset, name=args.channel, wholeExtent=wholeExtent)

        del dataset
        del wholeExtent

    # finalize Catalyst
    bridge.finalize()

if __name__ == "__main__":
    args = parser.parse_args()
    main(args)
