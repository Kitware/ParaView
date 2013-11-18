# - Try to find the Selenium browser drivers
# Once run, this macro will define the following
#
# CHROMEDRIVER_EXECUTABLE
# FIREFOXDRIVER_EXTENSION
# IEDRIVER_EXECUTABLE
# SAFARIDRIVER_EXTENSION

# This is a standard find, as the chromedriver executable is an
# external program on the system
find_program(CHROMEDRIVER_EXECUTABLE
  NAMES chromedriver
  PATHS CHROMEDRIVER_HOME ENV PATH
  PATH_SUFFIXES chromedriver bin chromedriver/bin
  )

# Now check if the firefox selenium driver extension is available
set(DO_FIREFOX_DRIVER_EXTENSION_CHECK FALSE)

# The execute_process which runs python to check for the presence of
# the firefox webdriver should only be run if either the
# FIREFOXDRIVER_EXTENSION variable is not yet defined or if it came
# up NOTFOUND in the past.  Because CMake does not seem to do short-
# circuit evaluation, we have to do this check in multiple steps.
if(NOT DEFINED FIREFOXDRIVER_EXTENSION)
  set(DO_FIREFOX_DRIVER_EXTENSION_CHECK TRUE)
endif()

if(NOT ${DO_FIREFOX_DRIVER_EXTENSION_CHECK})
  if(${FIREFOXDRIVER_EXTENSION} MATCHES "NOTFOUND")
    set(DO_FIREFOX_DRIVER_EXTENSION_CHECK TRUE)
  endif()
endif()

if(${DO_FIREFOX_DRIVER_EXTENSION_CHECK})
  # The selenium firefox driver is not an external program, but rather
  # a Firefox extension.  This approach to finding the firefox selenium
  # webdriver seems to work, but it pops up a browser window, which is
  # less than desirable.  Improving this is something we will have to
  # think about in the future.
  execute_process(COMMAND ${PYTHON_EXECUTABLE}
    -c
    "
import os

try :
    import selenium
    from selenium import webdriver
    browser = webdriver.Firefox()
    browser.quit()
except :
    os._exit(0)

os._exit(1)
"
    RESULT_VARIABLE HAS_FIREFOXDRIVER_EXTENSION)

  if("1" STREQUAL ${HAS_FIREFOXDRIVER_EXTENSION})
    set(FIREFOX_EXT_VALUE "FIREFOXDRIVER_EXTENSION-FOUND")
  else()
    set(FIREFOX_EXT_VALUE "FIREFOXDRIVER_EXTENSION-NOTFOUND")
  endif()

  set(FIREFOXDRIVER_EXTENSION
    ${FIREFOX_EXT_VALUE}
    CACHE
    STRING
    "The location or presence of the Selenium Firefox driver extension"
    FORCE)
endif()

# TODO: Actually find these things
set(IEDRIVER_EXECUTABLE
  "IEDRIVER_EXECUTABLE-NOTFOUND"
  CACHE
  STRING
  "The location of the Selenium Internet Explorer driver executable")

set(SAFARIDRIVER_EXTENSION
  "SAFARIDRIVER_EXTENSION-NOTFOUND"
  CACHE
  STRING
  "The location or presence of the Selenium Safari driver executable")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SeleniumDrivers
  DEFAULT_MSG
  CHROMEDRIVER_EXECUTABLE
  FIREFOXDRIVER_EXTENSION
  IEDRIVER_EXECUTABLE
  SAFARIDRIVER_EXTENSION )

mark_as_advanced(CHROMEDRIVER_EXECUTABLE)
mark_as_advanced(FIREFOXDRIVER_EXTENSION)
mark_as_advanced(IEDRIVER_EXECUTABLE)
mark_as_advanced(SAFARIDRIVER_EXTENSION)
