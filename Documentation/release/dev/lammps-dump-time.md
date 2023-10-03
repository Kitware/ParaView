# Allow VisIt LAMMPS dump reader to read multiple time steps

The LAMMPS atom dump supports writing out multiple time steps. The VisIt
reader supports reading these timesteps, and that works in VisIt proper,
but ParaView was only reading the first time step.

The problem was that although the reader was properly filling the AVT
metadata with the cycles, it does not properly override the `GetCycles`
method of `avtFileFormat`. The VTK bridge was only looking at the data
returned from `GetCycles`, so was missing the time information.

I don't know why this information is stored in two places, which place
is used for what, or if this is a bug in `avtLAMMPSDumpFileFormat`.
Regardless, the bridge works fine if you check for time information in
the metadata if the reader itself does not return it.
