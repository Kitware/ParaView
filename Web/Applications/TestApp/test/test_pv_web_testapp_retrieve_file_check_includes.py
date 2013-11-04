

import sys
import requests
import re

from vtk.web import testing


# =============================================================================
# This function uses the python "requests" library and retrieves the index.html
# file of the ParaView WebVisualizer application.  Then it checks the retrieved
# html text for a few javascript source includes which are required.
# =============================================================================
def runTest(args) :

    # print 'Here is info on the supplied args: ' + str(dir(args))
    # print 'And here are the values: ' + str(args)

    # This name is used in error reporting
    testName = 'retrieve_index_and_check_includes.py'

    # Request the WebVisualizer index.html
    urlToRetrieve = 'http://localhost:' + str(args.port) + '/apps/TestApp'
    r = requests.get(urlToRetrieve)

    print "Status code: " + str(r.status_code)
    print "Content-Type: " + str(r.headers['content-type'])
    print "Encoding: " + str(r.encoding)

    indexText = r.text

    # These are some vtk and paraview scripts that should appear in index.html
    # of the WebVisualizer application.
    requiredJavascriptLibs = [ 'vtkweb-all.js',
                               'paraview.ui.pipeline.js',
                               'paraview.ui.toolbar.js',
                               'paraview.ui.toolbar.vcr.js',
                               'paraview.ui.toolbar.viewport.js' ]

    failedTest = False

    # Iterate through the above Javascript include files and make sure
    # they are each included somewhere in the returned html text.
    for jsLibName in requiredJavascriptLibs :
        regex = re.compile('<script src="../../lib/js/' + jsLibName + '"></script>')
        searchResults = regex.search(indexText)
        if searchResults is None :
            print 'ERROR: Could not find the required library: ' + jsLibName
            testing.test_fail(testName)
            return

    # If we didn't break out early, then this test passed
    testing.test_pass(testName)
