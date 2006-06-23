/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqPythonExport_h
#define _pqPythonExport_h

#if defined(WIN32) && defined(PARAVIEW_BUILD_SHARED_LIBS)
# if defined(pqPython_EXPORTS)
#   define PQPYTHON_EXPORT __declspec(dllexport)
# else
#   define PQPYTHON_EXPORT __declspec(dllimport)
# endif
#else
# define PQPYTHON_EXPORT
#endif

#endif // !_pqPythonExport_h
