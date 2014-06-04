
from vtk.web import testing
from vtk.web.testing import ImageComparatorWebTest

dependencies_met = True

try:
    # import modules for automating web testing using a real browser
    import selenium, Image
except:
    dependencies_met = False


# =============================================================================
# Define a subclass of one of the testing base classes.
# =============================================================================
class VisualizerCrushedCanTest(ImageComparatorWebTest) :
    """
    This class is based on ImageComparatorWebTest, and is designed to interact
    with the WebVisualizer application.  It overrides the setup phase of testing
    to open the can.ex2 data file, changes the coloring scheme, advances to the
    final time step, rescales the data coloring, and then re-centers the data.
    It overrides the capture phase of testing to save only a portion of the
    browser window as a png.  It reliese on the base class postprocessing phase.
    """
    def __init__(self, host='localhost', port=8080, **kwargs) :
        # Only the author of this test script knows what application is
        # being tested and how to get to it.
        self.urlPath = '/apps/Visualizer'

        self.host = host
        self.port = port

        appUrl = 'http://' + self.host + ':' + str(self.port) + self.urlPath

        # Continue with initialization of base classes
        ImageComparatorWebTest.__init__(self, url=appUrl, size=(720, 520), **kwargs)

    def checkdependencies(self):
        if dependencies_met == False:
            raise testing.DependencyError("One of python modules 'selenium' or 'Image' is missing")

    def setup(self) :
        testing.wait_with_timeout(delay=8)

        # First change the viewport size so that all browsers get the
        # same results.
        # $(".viewport-container").css("width", "350px").css("height", "350px")
        scriptToExecute = "$('.renderers').parent().css('width', '350px').css('height', '350px')"
        self.window.execute_script(scriptToExecute)
        testing.wait_with_timeout(delay=1)

        # Click on the "Open file" icon to start the process of loading a file
        filesDiv = self.window.find_element_by_css_selector(".action.files")
        filesDiv.click()
        testing.wait_with_timeout(delay=1)

        # Click on the "can" link to load some paraview data.  We have the
        # expectation here that the paraview data dir with which we started the
        # server points to the "Data" folder in the standard ParaViewData git
        # repo.
        canLi = self.window.execute_script("return $('.vtk-files.action:contains(can.ex2)')[0]")
        canLi.click()
        testing.wait_with_timeout(delay=3)

        # Now choose how to color the object
        colorByLink = self.window.find_element_by_css_selector(".colorBy.color")
        colorByLink.click()
        testing.wait_with_timeout(delay=1)

        colorByDispLi = self.window.find_element_by_css_selector(".points[name=DISPL]")
        colorByDispLi.click()
        testing.wait_with_timeout(delay=1)

        # Jump to the final time step
        endTimeLi = self.window.find_element_by_css_selector(".action[action=last]")
        endTimeLi.click()
        testing.wait_with_timeout(delay=1)

        # Rescale now that we're at the final time step
        rescaleIcon = self.window.find_element_by_css_selector(".rescale-data")
        rescaleIcon.click()
        testing.wait_with_timeout(delay=1)

        # Now click the resetCamera icon so that we change the center of
        # rotation
        resetCameraIcon = self.window.find_element_by_css_selector("[action=resetCamera]");
        resetCameraIcon.click()
        testing.wait_with_timeout(delay=1)

    def capture(self) :
        # Now grab the renderer image and write it to disk
        testing.save_image_data_as_png(self.window, ".image.active>img", self.filename)
