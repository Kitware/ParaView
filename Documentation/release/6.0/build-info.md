## New Python package paraview.info

ParaView has now a new python module `info` that allows to get diagnostic
information about the current build.

You can access it via `pvbatch`:

```
pvbatch -m paraview.info
```

To get output as json use
```
pvbatch -m paraview.info --json
```

In pvpython  (or a catalyst script) you can now do


```
>>> from paraview.info import print_env
>>> print_env()
```

Sample output:
```
ParaView Version           5.13.2
VTK Version                9.4.1-1738-gcd19f92852
Python Library Path        /usr/lib/python3.10
Python Library Version     3.10.12 (main, Feb  4 2025, 14:57:36) [GCC 11.4.0]
Python Numpy Support       True
Python Numpy Version       1.21.5
Python Matplotlib Support  False
Python Matplotlib Version
MPI Enabled                True
--MPI Rank/Size            0/1
Disable Registry           False
SMP Backend                Sequential
SMP Max Number of Threads  1
OpenGL Vendor              Intel
OpenGL Version             4.6 (Core Profile) Mesa 23.2.1-1ubuntu3.1~22.04.3
OpenGL Renderer            Mesa Intel(R) UHD Graphics (TGL GT1)
```
