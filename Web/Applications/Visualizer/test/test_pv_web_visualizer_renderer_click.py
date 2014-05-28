
from vtk.web import testing
from vtk.web.testing import ImageComparatorWebTest

dependencies_met = True

try:
    # import modules for automating web testing using a real browser
    import selenium, Image
except:
    dependencies_met = False


# =============================================================================
# Define a subclass of one of the testing base classes.  The class name can be
# anything, as nothing depends on it.
# =============================================================================
class VisualizerRendererClick(ImageComparatorWebTest) :
    """
    This class is based on the ImageComparatorWebTest class defined in the
    vtk.web.testing module.  It overrides the setup function simply to clicks in
    the window and clean up the display.  It overrides the capture function so
    that it can store a png of the desired image element rather than capturing
    the entire browser window.  The default (parent) postprocess method calls
    the vtk image comparison function and either passes or fails the test,
    depending on the image comparison result.  Likewise, the default (parent)
    initialize method is good enough as it initializes the desired browser type,
    sets the window size, and loads the required url.
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
        scriptToExecute = "$('.renderers').parent().css('width', '350px').css('height', '350px')"
        self.window.execute_script(scriptToExecute)
        testing.wait_with_timeout(delay=1)

        clickPanel = self.window.find_element_by_css_selector(".mouse-listener")
        clickPanel.click()
        testing.wait_with_timeout(delay=1)

    def capture(self) :
        # Now grab the renderer image and write it to disk
        testing.save_image_data_as_png(self.window, ".image.active>img", self.filename)
