import sys

if len(sys.argv) != 2:
    print 'need to pass in location of plugin library'
    sys.exit(1)

import paraview.simple

paraview.simple.LoadPlugin(sys.argv[1], True, globals())
print 'loaded ', sys.argv[1], ' the first time successfully'
paraview.simple.LoadPlugin(sys.argv[1], True, globals())
print 'loaded ', sys.argv[1], ' the second time successfully'
