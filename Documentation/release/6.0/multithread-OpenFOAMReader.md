## OpenFOAMReader: Multithreaded Reading of case files

The `OpenFOAM` reader now supports multithreaded reading of OpenFOAM case files. This feature is on by default, and it
can be disabled by setting the `Sequential Processing` property to true. It can be useful to be enabled when reading
large case files that are stored on a network drive. If the case file is stored on a local drive, it may be better to
keep this feature off. An additional property, `Read All Files To Determine Structure` has been added, which enables
reading only the proc0 directory to determine the structure of the case file, and broadcasts it to all processors.
This is off by default, because there is not such guarantee that the proc0 directory contains all the necessary
information to determine the structure of the full set of data files. Finally, the default value of property
Create cell-to-point filtered data has been changed to false.
