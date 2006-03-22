/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _QtWidgetsExport_h
#define _QtWidgetsExport_h

#if defined(WIN32) && defined(PARAQ_BUILD_SHARED_LIBS)
# if defined(QtWidgets_EXPORTS)
#   define QTWIDGETS_EXPORT __declspec(dllexport)
# else
#   define QTWIDGETS_EXPORT __declspec(dllimport)
# endif
#else
# define QTWIDGETS_EXPORT
#endif

#endif // !_QtWidgetsExport_h
