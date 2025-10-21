## Fixed crash with SILO Reader

The SILO reader was experiencing a crash when compiled with the latest version
of the SILO library (4.11). This was caused by a metadata load that failed to
capture variable names that were later expected to be found. The appropriate
metadata is now loaded.

This fixes a problem that has been reported a number of times:
https://gitlab.kitware.com/paraview/paraview/-/issues/22373 and
https://github.com/LLNL/Silo/issues/445.
