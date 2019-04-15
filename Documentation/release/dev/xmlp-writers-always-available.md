# Make the parallel XML writers easily available in serial.

The parallel XML writers are now available for saving data in the
GUI as well as with the Python SaveData() method when the server is
also a single process so that things like traces and then running
the Python script later on work regardless of the number of MPI processes.
