
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

import copy

class Bidict(dict):
    """Subclass of mapping that automatically keeps track of the
inverse mapping. Note: self.inverse is a simple dict, so it won't keep
track of deletions directly to self.inverse and things like that. Use
this for lookups ONLY!. Also, if mapping is not bijective, there's no
guarantee the inverse mapping will be consistent (particularly in the
presence of deletions.)"""

    def __init__(self, *args, **kwargs):

        dict.__init__(self, *args, **kwargs)
        self.inverse = {}
        for (k, v) in self.iteritems():
            self.inverse[v] = k

    def __setitem__(self, key, value):
        dict.__setitem__(self, key, value)
        self.inverse[value] = key

    def __delitem__(self, key):
        v = self[key]
        dict.__delitem__(self, key)
        # Might not be true if mapping was not bijective
        if v in self.inverse:
            del self.inverse[v]

    def __copy__(self):
        r = Bidict()
        r.inverse = copy.copy(self.inverse)
        r.update(self)
        return r

    def update(self, other):
        try:
            for i in other.iterkeys():
                self[i] = other[i]
        except:
            for (k,v) in other:
                self[k] = v

##############################################################################

import unittest

class TestBidict(unittest.TestCase):

    def test1(self):
        x = Bidict()
        for i in xrange(10):
            x[i] = 9-i
        for i in xrange(10):
            self.assertEquals(x[i], 9-i)
            self.assertEquals(x.inverse[i], 9-i)
        del x[1]
        self.assertRaises(KeyError, x.__getitem__, 1)
        self.assertRaises(KeyError, x.inverse.__getitem__, 8)

    def test_non_bijective(self):
        """Tests resilience (not correctness!) under non-bijectiveness."""
        x = Bidict()
        x[1] = 2
        x[3] = 2
        del x[1]
        del x[3]

    def test_copy(self):
        """Tests copying a Bidict."""
        x = Bidict()
        x[1] = 2
        x[3] = 4
        y = copy.copy(x)
        assert y.inverse[4] == x.inverse[4]
        assert y.inverse[2] == x.inverse[2]

    def test_update(self):
        """Tests if updating a bidict with a dict works"""
        x = {1:2, 3:4}
        y = Bidict()
        y.update(x)
        assert y.inverse[4] == 3
        assert y.inverse[2] == 1
        


if __name__ == '__main__':
    unittest.main()
