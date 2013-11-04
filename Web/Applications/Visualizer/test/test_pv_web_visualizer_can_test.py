
import time

# import modules for automating web testing using a real browser
import selenium
from selenium import webdriver
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.by import By
from selenium.common.exceptions import NoSuchElementException

from vtk.web import testing


# =============================================================================
# This function uses Selenium library to open a browser window and load the
# ParaView WebVisualizer appliction.  Then it interacts with the browser app
# to open a known file (the iron protein file).  Then the image is captured
# and compared with a baseline.
# =============================================================================
def runTest(args) :

    # print 'We were passed the following args: ' + str(args)

    # This name is used in error reporting
    testName = 'pv_web_visualizer_open_browser_and_click_renderer.py'

    # Request the WebVisualizer index.html
    urlToRetrieve = 'http://localhost:' + str(args.port) + '/apps/Visualizer'

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

    # Click on the "Open file" icon to start the process of loading a file
    filesDiv = browser.find_element_by_css_selector(".action.files")
    filesDiv.click()
    time.sleep(1)

    # Click on the "can" link to load some paraview data.  We have the
    # expectation here that the paraview data dir with which we started the
    # server points to the "Data" folder in the standard ParaViewData git
    # repo.
    canLi = browser.execute_script("return $('.open-file:contains(can.ex2)')[0]")
    canLi.click()
    time.sleep(3)

    # Now choose how to color the object
    colorByLink = browser.find_element_by_css_selector(".colorBy.color")
    colorByLink.click()
    time.sleep(1)

    colorByDispLi = browser.find_element_by_css_selector(".points[name=DISPL]")
    colorByDispLi.click()
    time.sleep(1)

    endTimeLi = browser.find_element_by_css_selector(".action[action=last]")
    endTimeLi.click()
    time.sleep(1)

    rescaleIcon = browser.find_element_by_css_selector(".rescale-data")
    rescaleIcon.click()
    time.sleep(1)

    # Now click the resetCamera icon so that we change the center of
    # rotation
    resetCameraIcon = browser.find_element_by_css_selector("[action=resetCamera]");
    resetCameraIcon.click()
    time.sleep(1)

    '''
    # Click-and-drag on the cone to change its orientation a bit.
    listenerDiv = browser.find_element_by_css_selector(".mouse-listener")
    drag = ActionChains(browser)
    drag.move_to_element(listenerDiv)
    drag.click_and_hold()
    drag.move_by_offset(100, 0)
    drag.release()
    drag.perform()
    time.sleep(1)
    '''

    # Now grab the renderer image and write it to disk
    imgdata = testing.get_image_data(browser, ".image.active>img")
    filename = 'test_pv_web_visualizer_can_test.jpg'
    testing.write_image_to_disk(imgdata, filename)

    knownGoodFileName = testing.concat_paths(baselineImgDir,
                                             'test_pv_web_visualizer_can_test_known_good.jpg')

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
