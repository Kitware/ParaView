
from vtk.web import testing
from vtk.web.testing import WebTest

dependencies_met = True

try:
    # import module for scripting HTTP requests from Python
    import requests
except:
    dependencies_met = False


# =============================================================================
# This test class extends the most basic kind of web test, WebTest.  It over-
# rides the setup, and postprocess methods to do it's work, which does not
# require a browser, but rather simply uses the the Python "requests" module
# to fetch content over HTTP.  The work done in this test could have been split
# up differently, for example, we could have overridden capture() to do some
# of the work.  The only requiremen
# =============================================================================
class SimpleRequestBasedTest(WebTest) :

    def __init__(self, host=None, port=None, **kwargs) :
        self.urlPath = '/apps/TestApp'

        self.host = host
        self.port = port

        appUrl = 'http://' + self.host + ':' + str(self.port) + self.urlPath

        # Continue with initialization of base classes
        WebTest.__init__(self, url=appUrl, **kwargs)

    def checkdependencies(self):
        if dependencies_met == False:
            raise testing.DependencyError("Python module 'requests' is missing")

    def setup(self) :
        self.requestResult = requests.get(self.url)

        print "Status code: " + str(self.requestResult.status_code)
        print "Content-Type: " + str(self.requestResult.headers['content-type'])
        print "Encoding: " + str(self.requestResult.encoding)

    def postprocess(self) :
        if self.requestResult.status_code != 200 :
            print 'Failing test because HTTP status code for main ' + \
                'application url was other than 200: ' + \
                str(self.requestResult.status_code)
            testing.test_fail(self.testname);

        # These are some vtk and paraview scripts that should appear in index.html
        # of the WebVisualizer application.
        baseUrl = 'http://' + self.host + ':' + str(self.port) + '/'
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
            testing.test_pass(self.testname)
        else :
            print failMessage
            testing.test_fail(self.testname)
