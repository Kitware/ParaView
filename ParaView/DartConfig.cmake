# Dashboard is opened for submissions for a 24 hour period starting at
# the specified NIGHLY_START_TIME. Time is specified in 24 hour format.
SET (NIGHTLY_START_TIME "0:30:00 EDT")

# Dart server to submit results (used by client)
IF(DROP_METHOD MATCHES http)
  SET (DROP_SITE "public.kitware.com")
  SET (DROP_LOCATION "/cgi-bin/HTTPUploadDartFile.cgi")
ELSE(DROP_METHOD MATCHES http)
  SET (DROP_SITE "public.kitware.com")
  SET (DROP_LOCATION "/incoming")
  SET (DROP_SITE_USER "ftpuser")
  SET (DROP_SITE_PASSWORD "public")
ENDIF(DROP_METHOD MATCHES http)

SET (TRIGGER_SITE 
       "http://${DROP_SITE}/cgi-bin/Submit-ParaView-TestingResults.pl")

# Project Home Page
SET (PROJECT_URL "http://www.paraview.org")

# Dart server configuration 
SET (ROLLUP_URL "http://${DROP_SITE}/cgi-bin/paraview-rollup-dashboard.sh")
SET (CVS_WEB_URL "http://${DROP_SITE}/cgi-bin/cvsweb.cgi/ParaView/")
SET (CVS_WEB_CVSROOT "ParaView")
SET (USE_DOXYGEN "On")
SET (DOXYGEN_URL "${PROJECT_URL}/doc" )
SET (GNATS_WEB_URL "${PROJECT_URL}/Bug/query.php?projects=3&status%5B%5D=1&status%5B%5D=2&status%5B%5D=3&status%5B%5D=4&status%5B%5D=6&op=doquery")
SET (USE_GNATS "On")

# copy over the testing logo
CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/Resources/ParaViewLogo.gif ${PARAVIEW_BINARY_DIR}/Testing/HTML/TestingResults/Icons/Logo.gif COPYONLY)

# Continuous email delivery variables
SET (CONTINUOUS_FROM "paraview-dashboard@public.kitware.com")
SET (SMTP_MAILHOST "public.kitware.com")
SET (CONTINUOUS_MONITOR_LIST "paraview-dashboard@public.kitware.com andy.cedilnik@kitware.com")
SET (CONTINUOUS_BASE_URL "${PROJECT_URL}/Testing")

