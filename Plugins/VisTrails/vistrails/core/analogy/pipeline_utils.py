
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
##
############################################################################


############################################################################
# Utility functions for debugging on eigen.py

from core.data_structures.point import Point

def smart_sum(v):
    try:
        fst = v.next()
        return sum(v, fst)
    except:
        pass
    fst = v[0]
    return sum(v[1:], fst)

def pipeline_centroid(pipeline):
    """Returns the centroid of a given pipeline."""
    return (smart_sum(x.location for
                      x in pipeline.modules.itervalues()) *
            (1.0 / len(pipeline.modules)))

def pipeline_bbox(pipeline):
    mn_x = 1000000000.0
    mn_y = 1000000000.0
    mx_x = -1000000000.0
    mx_y = -1000000000.0
    for m in pipeline.modules.itervalues():
        mn_x = min(mn_x, m.location.x)
        mn_y = min(mn_y, m.location.y)
        mx_x = max(mx_x, m.location.x)
        mx_y = max(mx_y, m.location.y)
    return (Point(mn_x, mn_y), Point(mx_x, mx_y))
