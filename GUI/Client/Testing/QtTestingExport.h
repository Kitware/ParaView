/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _QtTestingExport_h
#define _QtTestingExport_h

#if defined(WIN32) && defined(PARAQ_BUILD_SHARED_LIBS)
# if defined(QtTesting_EXPORTS)
#   define QTTESTING_EXPORT __declspec(dllexport)
# else
#   define QTTESTING_EXPORT __declspec(dllimport)
# endif
#else
# define QTTESTING_EXPORT
#endif

#endif // !_QtTestingExport_h
