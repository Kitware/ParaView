SET(NIGHTLY_START_TIME "21:30:00 EDT")

IF(DROP_METHOD MATCHES http)
  SET(DROP_SITE "public.kitware.com")
  SET(DROP_LOCATION "/cgi-bin/HTTPUploadDartFile.cgi")
ELSE(DROP_METHOD MATCHES http)
  SET(DROP_SITE "public.kitware.com")
  SET(DROP_LOCATION "/incoming")
  SET(DROP_SITE_USER "ftpuser")
  SET(DROP_SITE_PASSWORD "public")
ENDIF(DROP_METHOD MATCHES http)

SET(TRIGGER_SITE 
  "http://${DROP_SITE}/cgi-bin/Submit-KWWidgets-TestingResults.pl")

SET(PROJECT_URL "http://www.kwwidgets.org")

SET(ROLLUP_URL "http://${DROP_SITE}/cgi-bin/KWWidgets-rollup-dashboard.sh")

SET(CVS_WEB_URL "http://kwwidgets.org/cgi-bin/viewcvs.cgi/")
SET(CVS_WEB_CVSROOT "KWWidgets")

SET(USE_DOXYGEN "On")
SET(DOXYGEN_URL "http://www.kwwidgets.org/doc/nightly/html/")

SET(USE_GNATS "On")
SET(GNATS_WEB_URL "${PROJECT_URL}/Bug/query.php?projects=15&status%5B%5D=1&status%5B%5D=2&status%5B%5D=3&status%5B%5D=4&status%5B%5D=6&op=doquery")

CONFIGURE_FILE(
  ${KWWidgets_SOURCE_DIR}/Examples/Resources/KWWidgetsSplashScreenSmall.gif 
  ${KWWidgets_BINARY_DIR}/Testing/HTML/TestingResults/Icons/Logo.gif 
  COPYONLY)
