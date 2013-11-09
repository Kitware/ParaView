import time
from sets import Set

# import modules for automating web testing using a real browser
import selenium
from selenium import webdriver
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.by import By
from selenium.common.exceptions import NoSuchElementException

from vtk.web import testing


# =============================================================================
# The runTest(args) function must be implemented for the test to be run.
# =============================================================================
def runTest(args) :
    """
    This function uses Selenium library to open a browser window and load the
    ParaView TestApp appliction.  Then it interacts with the application by
    making Javascript calls to verify that modules are registered and that
    protocols are working.
    """

    # print 'We were passed the following args: ' + str(args)

    # This name is used in error reporting
    testName = 'pv_web_test_app_all.py'

    # Request the WebVisualizer index.html
    urlToRetrieve = 'http://localhost:' + str(args.port) + '/apps/TestApp'

    # Create a Chrome window driver.
    browser = webdriver.Chrome()
    browser.set_window_size(1024, 768)
    browser.get(urlToRetrieve)

    sleepSeconds = 8
    print "Going to sleep for " + str(sleepSeconds) + " seconds to let browser load page"
    time.sleep(sleepSeconds)
    print "Ok, page should be loaded by now...continuing."

    startButton = browser.find_element_by_css_selector(".run-tests")
    startButton.click()

    # Loop until the javascript-side tests are finished
    while True :
        # Perform the check to see if tests are finished yet
        currentResults = browser.execute_script("return vtkWeb.testing.getCurrentTestResults();")

        if currentResults['finished'] is True :
            # Done with tests, check results
            testsSucceeded = currentResults['failures'] == 0

            if testsSucceeded :
                testing.test_pass(testName)
            else :
                testing.test_fail(testName)
                messageLog = browser.execute_script("return vtkWeb.testing.getTestLog();");
                print "Following is the message log from the tests:"
                print messageLog
                print

            break;

    # Don't forget to close the browser when we're done
    browser.quit()
