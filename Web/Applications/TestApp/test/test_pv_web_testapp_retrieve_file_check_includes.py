

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

    if r.status_code != 200 :
        print 'Failing test because HTTP status code for main ' + \
            'application url was other than 200: ' + str(r.status_code)
        testing.test_fail(testName);

    # These are some vtk and paraview scripts that should appear in index.html
    # of the WebVisualizer application.
    baseUrl = 'http://localhost:' + str(args.port) + '/'
    requiredJavascriptLibs = [ baseUrl + 'lib/core/vtkweb-all.js',
                               baseUrl + 'lib/js/paraview.ui.pipeline.js',
                               baseUrl + 'lib/js/paraview.ui.toolbar.js',
                               baseUrl + 'lib/js/paraview.ui.toolbar.vcr.js',
                               baseUrl + 'lib/js/paraview.ui.toolbar.viewport.js' ]

    failedTest = False
    failMessage = "Unable to load the following modules: "

    # Iterate through the above Javascript include files and make sure
    # they are each included somewhere in the returned html text.
    for urlPath in requiredJavascriptLibs :
        r = requests.get(urlPath)

        if r.status_code != 200 :
            failMessage += '\n   ' + urlPath
            failedTest = True

    if failedTest is False :
        testing.test_pass(testName)
    else :
        print failMessage
        testing.test_fail(testName)
