# Dart server to submit results (used by client)
SET (DROP_SITE "public.kitware.com")
SET (DROP_LOCATION "/incoming")
SET (DROP_SITE_USER "ftpuser")
SET (DROP_SITE_PASSWORD "public")
SET (TRIGGER_SITE 
       "http://${DROP_SITE}/cgi-bin/Submit-ParaView-TestingResults.pl")

# Dart server configuration 
SET (CVS_WEB_URL "http://${DROP_SITE}/cgi-bin/cvsweb.cgi/ParaView/")
SET (CVS_WEB_CVSROOT "ParaView")
SET (DOXYGEN_URL "http://${DROP_SITE}/" )
SET (GNATS_WEB_URL "http://${DROP_SITE}/")

# copy over the testing logo
CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/Web/Art/ParaViewLogo.gif ${PARAVIEW_BINARY_DIR}/Testing/HTML/TestingResults/Icons/Logo.gif COPYONLY)
