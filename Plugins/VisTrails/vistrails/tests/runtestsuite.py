
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

"""Runs all tests available in VisTrails modules by importing all of
them, stealing the classes that look like unit tests, and running
all of them.

runtestsuite.py also reports all VisTrails modules that don't export
any unit tests, as a crude measure of code coverage.

"""

import os
import sys
import unittest
import os.path

# Makes sure we can import modules as if we were running VisTrails
# from the root directory
if __name__ == '__main__':
    _this_dir = sys.argv[0]
else:
    _this_dir = sys.modules[__name__].__file__
_this_dir = os.path.split(_this_dir)[0]
if not _this_dir:
    root_directory = os.path.join('.','..')
else:
    root_directory = os.path.join(_this_dir,  '..')
sys.path.append(root_directory)

###############################################################################
# Utility

def sub_print(s, overline=False):
    """Prints line with underline (and optionally overline) ASCII dashes."""
    if overline:
        print "-" * len(s)
    print s
    print "-" * len(s)

def get_test_cases(module):
    """Return all test cases from the module. Test cases are classes derived
    from unittest.TestCase"""
    result = []
    import inspect
    for member_name in dir(module):
        member = getattr(module, member_name)
        if inspect.isclass(member) and issubclass(member, unittest.TestCase):
            result.append(member)
    return result

###############################################################################

verbose = 0
if len(sys.argv) > 2:
    if sys.argv[1] == '-verbose':
        verbose = int(sys.argv[2])
        sys.argv = [sys.argv[0]] + sys.argv[3:]

test_modules = None
if len(sys.argv) > 1:
    test_modules = set(sys.argv[1:])


# creates the app so that testing can happen
import gui.application
sys.argv = sys.argv[:1]
gui.application.start_application({'interactiveMode': False,
                                   'nologger': True})

print "Test Suite for VisTrails"

main_test_suite = unittest.TestSuite()

if test_modules:
    sub_print("Trying to import some of the modules")
else:
    sub_print("Trying to import all modules")

for (p, subdirs, files) in os.walk(root_directory):
    # skip subversion subdirectories
    if p.find('.svn') != -1:
        continue
    for filename in files:
        # skip files that don't look like VisTrails python modules
        if not filename.endswith('.py'):
            continue
#        module = p[5:] + '/' + filename[:-3]
        module = p[len(root_directory)+1:] + os.sep + filename[:-3]
        if (module.startswith('tests') or
            module.startswith(os.sep) or
            module.startswith('\\') or
            ('#' in module)):
            continue
        if ('system' in module and not
            module.endswith('__init__')):
            continue
        if test_modules and not module in test_modules:
            continue
        msg = ("%s %s |" % (" " * (40 - len(module)), module))

        # use qualified import names with periods instead of
        # slashes to avoid duplicates in sys.modules
        module = module.replace('/','.')
        module = module.replace('\\','.')
        if module.endswith('__init__'):
            module = module[:-9]
        try:
            if '.' in module:
                m = __import__(module, globals(), locals(), ['foo'])
            else:
                m = __import__(module)
        except:
            print msg, "ERROR: Could not import module!"
            continue

        test_cases = get_test_cases(m)
        for test_case in test_cases:
            suite = unittest.TestLoader().loadTestsFromTestCase(test_case)
            main_test_suite.addTests(suite)

        if not test_cases and verbose >= 1:
            print msg, "WARNING: %s has no tests!" % filename
        elif verbose >= 2:
            print msg, "Ok: %s test cases." % len(test_cases)

unittest.TextTestRunner().run(main_test_suite)

gui.application.VistrailsApplication.finishSession()
