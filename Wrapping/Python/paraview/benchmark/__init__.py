'''
This module has utilities to benchmark paraview.

logbase contains core routines for collecting and gathering timing logs from
all nodes.
logparser contains additional routines for parsing the raw logs and
calculating statistics across ranks and frames.

manyspheres is a geometry rendering benchmark that generates a large number
of spheres and moves the camera around the scene.  To run the benchmark,
either explicitly import manyspheres from paraview.benchmark and call it's
run method, or call the manyspheres.py module directly via pvbatch or pvpython.

::

    TODO: this doesn't handle split render/data server mode
    TODO: the end of frame markers are heuristic, likely buggy
'''

from __future__ import absolute_import

from . import logbase
from . import logparser

__all__ = ['logbase', 'logparser']
