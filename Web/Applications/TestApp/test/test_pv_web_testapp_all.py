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
    browser.set_window_size(720, 480)
    browser.get(urlToRetrieve)

    sleepSeconds = 8
    print "Going to sleep for " + str(sleepSeconds) + " seconds to let browser load page"
    time.sleep(sleepSeconds)
    print "Ok, page should be loaded by now...continuing."

    browser.execute_script("session.call('vtk:listFiles').then(callback)")
    time.sleep(2)
    listObject = browser.execute_script("return tmpReturnValue;")

    for item in listObject :
        print item['name']

    '''
    neededModules = ['', '', ...]
    anyIn = lambda a, b: not set(b).isdisjoint(a)
    anyIn(neededModules, listObject)
    '''

    browser.quit()

    if compareResult != 0 :
        print "Images were different, diffsum was: " + str(compareResult)
        testing.testFail(testName)

    testing.testPass(testName)
