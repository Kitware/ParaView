file(GLOB pyFiles *.py)
install(FILES ${pyFiles} DESTINATION lib/python${PY_MAJOR}.${PY_MINOR} )
