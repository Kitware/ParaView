# - Try to find the Selenium browser drivers
# Once run, this macro will define the following
#
# CHROMEDRIVER_EXECUTABLE
# FIREFOXDRIVER_EXTENSION
# IEDRIVER_EXECUTABLE
# SAFARIDRIVER_EXTENSION

find_program(CHROMEDRIVER_EXECUTABLE
  NAMES chromedriver
  PATHS CHROMEDRIVER_HOME ENV PATH
  PATH_SUFFIXES chromedriver bin chromedriver/bin
  )

# TODO: Actually find these things
set(FIREFOXDRIVER_EXTENSION
  "FIREFOXDRIVER_EXTENSION-NOTFOUND"
  CACHE
  STRING
  "The location or presence of the Selenium Firefox driver extension")
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
