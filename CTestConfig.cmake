SET (CTEST_PROJECT_NAME "ParaView3")
SET (CTEST_NIGHTLY_START_TIME "21:00:00 EDT")
SET (CTEST_DROP_METHOD "http")
SET (CTEST_DROP_SITE "www.paraview.org")
SET (CTEST_DROP_LOCATION "/cgi-bin/HTTPUploadDartFile.cgi")
SET (CTEST_TRIGGER_SITE "http://${CTEST_DROP_SITE}/cgi-bin/Submit-ParaView3-TestingResults.cgi")

