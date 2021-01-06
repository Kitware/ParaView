r"""
This script is a miniapp that acts a Catalyst-instrumented simulation code.
Instead of doing some computation, however, this script reads the files
specified through command line arguments and provides the data read in as the
simulation data.

Example usage:

    mpirun -np 8 ./bin/pvbatch -sym -m paraview.demos.filedriver_miniapp \
          -g "/tmp/extracts/Wavelet1_*.pvti" -s /tmp/foo.py

"""

import argparse, time, os.path, glob

parser = argparse.ArgumentParser(\
        description="File-based MiniApp for Catalyst testing")

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
parser.add_argument("-g", "--glob", type=str,
    help="Pattern to use to locate input filenames.", required=True)

def create_reader(files):
    from paraview import simple
    reader = simple.OpenDataFile(files)
    if not reader:
        raise RuntimeError("Failed to create a suitable reader for files: %s", str(files))
    return reader

def read_dataset(reader, time, rank, num_ranks):
    reader.UpdatePipeline(time)

    vtk_reader = reader.GetClientSideObject()
    ds = vtk_reader.GetOutputDataObject(0)
    if ds.GetExtentType() == 1:  # VTK_3D_EXTENT
        key = vtk_reader.GetExecutive().WHOLE_EXTENT()
        whole_extent = vtk_reader.GetOutputInformation(0).Get(key)[:]
        return (ds, whole_extent)
    else:
        return (ds, None)

def main(args):
    """The main loop"""

    # this globbling logic is copied from `filedriver.py`. It may be worth
    # cleaning this up to ensure it handles typical use-cases we encounter.
    files = glob.glob(args.glob)

    # In case the filenames aren't padded we sort first by shorter length and then
    # alphabetically. This is a slight modification based on the question by Adrian and answer by
    # Jochen Ritzel at:
    # https://stackoverflow.com/questions/4659524/how-to-sort-by-length-of-string-followed-by-alphabetical-order
    files.sort(key=lambda item: (len(item), item))

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

    reader = create_reader(files)
    timesteps = reader.TimestepValues[:]
    step = 0
    numsteps = len(timesteps)
    for time in timesteps:
        if args.delay > 0:
            import time
            time.sleep(args.delay)

        if rank == 0:
            print_info("timestep: {0} of {1}".format((step+1), numsteps))

        dataset, wholeExtent = read_dataset(reader, time, rank, num_ranks)

        # "perform" coprocessing.  results are outputted only if
        # the passed in script says we should at time/step
        bridge.coprocess(time, step, dataset, name=args.channel, wholeExtent=wholeExtent)

        del dataset
        del wholeExtent
        step += 1

    # finalize Catalyst
    bridge.finalize()

if __name__ == "__main__":
    args = parser.parse_args()
    main(args)
