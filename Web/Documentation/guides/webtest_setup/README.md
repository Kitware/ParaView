# How to satisfy Web-based testing requirements on a new machine

## Introduction

This document describes how to install the requirements to run automated tests of ParaViewWeb.  Automated testing of ParaViewWeb is accomplished in Python scripts written specifically for each web application, so the required modules are mostly Python modules.  However, some external applications are also required, and this document covers installing all necessary components.

## Overview

Automated web testing for ParaViewWeb relies on Selenium, specifically the selenium Python module.  Browser-based tests are available currently for Chrome and Firefox (eventually Safari and Internet Explorer could be supported as well), and these tests are made possible by Selenium "webdrivers", which are available for each of the browsers.  And to be clear, a system must first have a browser installed before it makes sense to install the Selenium webdriver for that browser.  In addition to Selenium, a few other Python modules must be installed as well, like "Image" for comprehending and storing jpg and png images, as well as the "requests" module for doing HTTP requests.  At a high level, these are the steps we will follow in these instructions:

1. Install Python setuptools for installing Python modules
1. Install required Python modules
1. Install webdrivers (currently Chrome and Firefox are supported)

## Install Python setuptools

More detailed instructions for installing setuptools for different platforms can be found [here](https://pypi.python.org/pypi/setuptools#installation-instructions), but basically you get the ez_setup.py file and run it with Python.

    wget https://bitbucket.org/pypa/setuptools/raw/bootstrap/ez_setup.py -O - | sudo python

If you're installing using an alternate Python, just set the LD_LIBRARY_PATH to point to the appropriate libpython.so (Python shared library), and then run that version of Python, perhaps without root access if applicable:

    wget https://bitbucket.org/pypa/setuptools/raw/bootstrap/ez_setup.py
    export LD_LIBRARY_PATH=<path-to-your-python-install>/lib
    <path-to-your-python-install>/bin/python ez_setup.py

After this, your Python bin directory should contain a new program, "easy_install".  Instead of repeating both alternatives above for every step in the Python modules installation, below, just remember that both alternatives are available (system Python or custom installed Python), and choose the one which is right for you.

## Install Python modules

Now we will install the Python modules required before we can enable automated testing for ParaViewWeb.

### Install selenium module

To install the selenium module, you can download it from [here](https://pypi.python.org/pypi/selenium).  For example:

    wget https://pypi.python.org/packages/source/s/selenium/selenium-2.39.0.tar.gz
    tar -zxvf selenium-2.39.0.tar.gz
    cd selenium-2.39.0
    <path-to-python>/bin/python setup.py install

### Install requests module

If you have easy_install installed properly, then you can just type:

    <path-to-python>/bin/easy_install requests

and you're done.  Otherwise, you might have to install by:

    wget https://github.com/kennethreitz/requests/tarball/master
    tar -zxvf master
    cd kennethreitz-requests-...
    <path-to-python>/bin/python setup.py install

### Install image module

If the Python image module (imported with "import Image") already exists in your Python, then this requirement is already met.  If you need ot install it, follow these instructions, more or less:

    wget http://effbot.org/downloads/Imaging-1.1.7.tar.gz
    tar -zxvf Imaging-1.1.7.tar.gz
    cd Imaging-1.1.7

You will need to install zlib-devel or libjpeg libraries for png and jpg support if you don't have them already.  The module build and install process will tell you if it found those libraries.  So now follow the README instructions in the Imaging-1.1.7 directory and do an in-place build:

    <path-to-python>/bin/python setup.py build_ext -i

Now you'll see some output which ends with something like:

    --------------------------------------------------------------------
    PIL 1.1.7 SETUP SUMMARY
    --------------------------------------------------------------------
    version       1.1.7
    platform      linux2 2.7.3 (default, Jan 10 2014, 13:22:44)
                  [GCC 4.4.7 20120313 (Red Hat 4.4.7-4)]
    --------------------------------------------------------------------
    *** TKINTER support not available (Tcl/Tk 8.5 libraries needed)
    *** JPEG support not available
    *** ZLIB (PNG/ZIP) support not available
    *** FREETYPE2 support not available
    *** LITTLECMS support not available
    --------------------------------------------------------------------
    To add a missing option, make sure you have the required
    library, and set the corresponding ROOT variable in the
    setup.py script.

    To check the build, run the selftest.py script.

If you know you have those libraries installed, you may need to edit the setup.py as follows:

    TCL_ROOT = None
    JPEG_ROOT = "/usr/lib64"
    ZLIB_ROOT = "/usr/lib64"
    TIFF_ROOT = None
    FREETYPE_ROOT = "/usr/lib64"
    LCMS_ROOT = None

where you will substitute "/usr/lib64" with the path appropriate for your system.  Then run the in-place build again:

    <path-to-python>/bin/python setup.py build_ext -i --force

This time the ZLIB and JPEG support should be marked as "available".  Now you can run the self-test and install:

    <path-to-python>/bin/python selftest.py
    <path-to-python>/bin/python setup.py install

## Install browsers and webdrivers

First, any browser which is to be used in automated testing must be installed on the machine, and this is beyond the scope of this document.  Here we will focus only on installing the webdrivers for Chrome and Firefox.

### Installing Chrome webdriver

The Chrome webdriver is installed as an external application and must be available in your PATH, or equivalent.  When you get a response to

    which chromedriver

then you are ready to go.  Basically, the procedure is as follows:

1. Download the application.  There is a list of versions available [here](http://chromedriver.storage.googleapis.com/index.html).
1. Unpack it somewhere and add that location to your PATH environment variable.

### Installing Firefox webdriver

The Firefox webdriver is not an external application, but rather a Firefox extension.  There is a list of the currently available downloads [here](https://code.google.com/p/selenium/downloads/list).  Then use the following steps to get it installed:

    wget https://selenium.googlecode.com/files/selenium-server-standalone-2.39.0.jar
    mkdir selenium-server-standalone-2.39.0
    cd selenium-server-standalone-2.39.0
    jar -xvf selenium-server-standalone-2.39.0.jar
    find ./ | grep "webdriver\\.xpi"

The reason for making the directory in the steps above is that the jar does not have any containment, files get put right into the current directory.  Your "find" command should come back with the location of the xpi file which you'll need to install as a Firefox extension.  For example:

    ./org/openqa/selenium/firefox/webdriver.xpi

Now start Firefox and install the add-on by pointing directly at the webdriver.xpi file:

1. Tools -> Add-ons
1. Select tool dropdown and choose "Install add-on from file"
1. Use the file chooser dialog to find the webdriver.xpi file
1. Hit "Install Now" button
1. Firefox will have to restart for changes to take effect, then you can go to "Extensions", in the panel on the left, to see if the Webdriver was really installed.

## Running a ParaViewWeb test

To run a test, you will just run the web application (Visualizer, Parallel, FileViewer, etc...) that has a test written for it with a few extra command line arguments.  Below is an example of how you might run a test from your ParaView build directory, but you can run from an install directory by changing the arguments appropriately:

    ./bin/pvpython ./lib/site-packages/paraview/web/pv_web_visualizer.py --content ./www --data-dir /home/kitware/projects/ParaViewData/Data --port 8080 --baseline-img-dir /home/kitware/projects/ParaViewData/Baseline/WebTesting/ParaView --run-test-script ./lib/site-packages/paraview/web/test/test_pv_web_visualizer_can_test.py --test-use-browser firefox --temporary-directory /home/kitware/Documents/dashboardtesting/MyTests/Experimental/ParaView-Debug-nogui/Testing/Temporary --test-image-file-name pvweb-firefox.Visualizer-can_test.png

The test-specific arguments are explained below:

    --baseline-img-dir: location of baseline images, these live in a ParaViewData subdirectory as above
    --run-test-script: path to a test script, there are several available in the same directory
    --test-browser: One of the supported browser types, if the test is a browser based test: "chrome"|"firefox"
    --temporary-directory: Required to get diff images on the dashboard machine in the case of test failures
    --test-image-file-name: A name to use for the temporary image generated by the test

In practice, these tests will be run by CTest, not from the command-line.  However, running the above command will give an indication of whether all the automated testing requirements have been met.

There are currently several web-based tests being run on a regular basis on ParaView dashboards.  The "Blight.kitware" dashboard with build name "ubuntu-x64-nightlymaster" (dashboards are located [here](http://open.cdash.org/index.php?project=ParaView)), runs most of the ParaViewWeb tests.  If you look for tests with the prefix "pvweb", you can see examples of command-lines that run automated web tests.
