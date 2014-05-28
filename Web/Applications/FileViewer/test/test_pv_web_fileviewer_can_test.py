
import time
from vtk.web import testing

dependencies_met = True

try:
    # import modules for automating web testing using a real browser
    import selenium, Image
    from selenium import webdriver
    from selenium.webdriver.common.action_chains import ActionChains
    from selenium.webdriver.common.by import By
    from selenium.common.exceptions import NoSuchElementException
    from selenium.webdriver.common.keys import Keys
except:
    dependencies_met = False


# =============================================================================
# This function uses Selenium library to open a browser window and load the
# ParaView WebVisualizer appliction.  Then it interacts with the browser app
# to open a known file (the iron protein file).  Then the image is captured
# and compared with a baseline.
# =============================================================================
def runTest(args) :

    # print 'We were passed the following args: ' + str(args)

    if dependencies_met == False:
        raise testing.DependencyError("One of python modules 'selenium' or 'Image' is missing or deficient")

    # This name is used in error reporting
    testName = 'pv_web_file_loader_open_browser_and_click_renderer.py'

    # Request the WebVisualizer index.html
    urlToRetrieve = 'http://localhost:' + str(args.port) + '/apps/FileViewer'

    # The author of pv_web_visualizer.py grabbed the --data-dir argument
    # from the command line and put it in a variable called "path" in the
    # arguments object, so that's where we look for the ParaViewData dir
    # inside this test script.
    baselineImgDir = args.baselineImgDir
    print 'The baseline images are located here: ' + baselineImgDir

    # Create a Chrome window driver.
    browser = webdriver.Chrome()
    browser.set_window_size(720, 480)
    browser.get(urlToRetrieve)

    sleepSeconds = 8
    print "Going to sleep for " + str(sleepSeconds) + " seconds to let browser load page"
    time.sleep(sleepSeconds)
    print "Ok, page should be loaded by now...continuing."

    # First we need to hit the enter key to make the modal pop-up go away
    browser.switch_to_alert().accept()
    time.sleep(3)

    sphereDataLi = browser.execute_script("return $('.jstree-leaf:contains(dualSphereAnimation_P00T0003.vtp)')[0]")
    sphereDataLi.click()
    time.sleep(1)

    # Now click the resetCamera icon so that we change the center of
    # rotation
    resetCameraIcon = browser.find_element_by_css_selector("[action=resetCamera]");
    resetCameraIcon.click()
    time.sleep(1)

    # Now grab the renderer image and write it to disk
    imgdata = testing.get_image_data(browser, ".image.active>img")
    filename = 'image_sphere_part.jpg'
    testing.write_image_to_disk(imgdata, filename)

    print 'About to compare images...'

    knownGoodFileName = testing.concat_paths(baselineImgDir,
                                             'image_sphere_part_known_good.jpg')

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
