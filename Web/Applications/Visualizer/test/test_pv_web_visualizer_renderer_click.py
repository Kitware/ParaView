

import sys
import time
import re

# import modules for automating web testing using a real browser
import selenium
from selenium import webdriver
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.by import By
from selenium.common.exceptions import NoSuchElementException

from vtk.web import testing


# =============================================================================
# This function uses Selenium library to open a browser window and load the
# ParaView WebVisualizer appliction.  It simply clicks in the window to clean
# up the display, then does an image capture and compares to a baseline.
# =============================================================================
def runTest(args) :

    # This name is used in error reporting
    testName = 'open_browser_and_click_renderer.py'

    # Request the WebVisualizer index.html
    urlToRetrieve = 'http://localhost:' + str(args.port) + '/apps/Visualizer'

    # print 'The args parameter contains: ' + str(args)

    # The author of pv_web_visualizer.py grabbed the --data-dir argument
    # from the command line and put it in a variable called "path" in the
    # arguments object, so that's where we look for the ParaViewData dir
    # inside this test script.
    baselineImgDir = args.baselineImgDir

    # Create a Chrome window driver.
    browser = webdriver.Chrome()
    browser.set_window_size(720, 480)
    browser.get(urlToRetrieve)

    sleepSeconds = 8
    time.sleep(sleepSeconds)

    clickPanel = browser.find_element_by_css_selector(".mouse-listener")
    clickPanel.click()
    time.sleep(1)

    # Now grab the renderer image and write it to disk
    imgdata = testing.get_image_data(browser, ".image.active>img")
    filename = 'test_pv_web_visualizer_renderer_click.jpg'
    testing.write_image_to_disk(imgdata, filename)

    knownGoodFileName = testing.concat_paths(baselineImgDir,
                                             'test_pv_web_visualizer_renderer_click_known_good.jpg')

    compareResult = -1

    try :
        compareResult = testing.compare_images(knownGoodFileName, filename)
        print 'Images compared with diff = ' + str(compareResult)
    except Exception as inst :
        print 'Caught exception in compareImages:'
        print inst
        testing.test_fail(testName)

    browser.quit()

    if compareResult != 0 :
        print "Images were different, diffsum was: " + str(compareResult)
        testing.test_fail(testName)

    testing.test_pass(testName)
