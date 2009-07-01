
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

"""Utilities for debugging garbage collection, leaked memory, etc."""

import gc

def get_objects_by_typename():
    gc.collect()
    x = {}
    for obj in gc.get_objects():
        t = type(obj).__name__
        if t in x:
            x[t].append(obj)
        else:
            x[t] = [obj]
    return x

def get_objects_by_livecount(obj_dict=None):
    if not obj_dict:
        obj_dict = get_objects_by_typename()
    else:
        gc.collect()
    lst = [(len(val), k) for (k, val) in obj_dict.iteritems()]
    lst.sort()
    return lst

def get_referrers(obj, path=None):
    if not path:
        path = []
    referrers = gc.get_referrers(obj)
    for edge in path:
        referrers = gc.get_referrers(referrers[edge])
    return referrers

def count_object_by_name(name):
    objects = get_objects_by_livecount()
    for (count, oName) in objects:
        if oName==name:
            return count
    return 0
