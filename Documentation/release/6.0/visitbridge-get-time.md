## Get time from VisIt readers for single time step

This gets the proper time values when reading a time series of
files, but each file defines the proper simulation time. Previously
any time values were ignored and the file index was used for the
time. Now, a time value can be read from a single file to get the
appropriate simulation time.
