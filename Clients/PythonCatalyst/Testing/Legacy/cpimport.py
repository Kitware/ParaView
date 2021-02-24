# Script to make sure we can import other scripts from
# a Catalyst script

import cpprogrammablefilter

def RequestDataDescription(datadescription):
    "Callback to populate the request for current timestep -- nullptr for this test"
    print("RequestDataDescription called")
    pass

def DoCoProcessing(datadescription):
    "Callback to do co-processing for current timestep -- nullptr for this test"
    print("DoCoProcessing called")
    pass
