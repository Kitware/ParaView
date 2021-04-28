"""This is a test to test the paraview proxy manager API."""
from paraview import servermanager, print_info, print_error

import sys

servermanager.Connect()

for source in dir(servermanager.sources):
    try:
        print_info('Creating %s ...' % source)
        if source in ["GenericIOReader", 'EnsembleDataReader', 'openPMDReader']:
            print_info("...skipping (in exclusion list)")
        else:
            s = getattr(servermanager.sources, source)()
            s.UpdateVTKObjects()
            print_info("...... ok")
    except:
        print_error("failed!")
        raise RuntimeError('Failed to create %s' % source)
