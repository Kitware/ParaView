
from vtk.web import testing
from vtk.web.testing import BrowserBasedWebTest

dependencies_met = True

try:
    # import modules for automating web testing using a real browser
    import selenium
except:
    dependencies_met = False


# =============================================================================
# Define a subclass of one of the testing base classes.  There are no
# restrictions on the class name, as nothing depends on it.
# =============================================================================
class JavascriptTestsRunner(BrowserBasedWebTest) :
    """
    This is a browser-based web test which does not (yet) require image compares,
    so it extends BrowserBasedWebTest.  It interacts with the TestApp application
    by just pushing the test button and then checking if the Javascript tests
    have finished in a loop.  When the test application indicates that it has
    finished, this test requests the results so that the test can be passed or
    failed.  In the case of failure, the entire contents of the test application
    log are printed out for CTest.
    """

    def __init__(self, host='localhost', port=8080, **kwargs) :
        # Only the author of this test script knows what application is
        # being tested and how to get to it.
        self.urlPath = '/apps/TestApp'

        self.host = host
        self.port = port

        appUrl = 'http://' + self.host + ':' + str(self.port) + self.urlPath

        # Continue with initialization of base classes
        BrowserBasedWebTest.__init__(self, url=appUrl, size=(800, 600), **kwargs)

    def checkdependencies(self):
        if dependencies_met == False:
            raise testing.DependencyError("Python module 'selenium' is missing")

    def setup(self) :
        testing.wait_with_timeout(delay=8)

        startButton = self.window.find_element_by_css_selector(".run-tests")
        startButton.click()

    def postprocess(self) :
        # Loop until the javascript-side tests are finished
        while True :
            # Perform the check to see if tests are finished yet
            currentResults = self.window.execute_script("return vtkWeb.testing.getCurrentTestResults();")

            if currentResults['finished'] is True :
                # Done with tests, check results
                testsSucceeded = currentResults['failures'] == 0

                if testsSucceeded :
                    testing.test_pass(self.testname)
                else :
                    testing.test_fail(self.testname)
                    messageLog = self.window.execute_script("return vtkWeb.testing.getTestLog();");
                    print "Following is the message log from the tests:"
                    print messageLog
                    print

                break;
