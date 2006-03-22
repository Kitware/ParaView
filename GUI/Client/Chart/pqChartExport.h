/*!
 * \file pqChartExport.h
 * \brief
 *   Used to switch between dll import and export on windows.
 * \date August 19, 2005
 */

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef __pqChartExport_h
#define __pqChartExport_h

#if defined(WIN32) && defined(PARAQ_BUILD_SHARED_LIBS)
# if defined(pqChart_EXPORTS)
#   define QTCHART_EXPORT __declspec(dllexport)
# else
#   define QTCHART_EXPORT __declspec(dllimport) 
# endif
#else
# define QTCHART_EXPORT
#endif

// The plugin is always dynamic.
#if defined(WIN32)
# if defined(pqChartPlugin_EXPORTS)
#   define QTCHARTPLUGIN_EXPORT __declspec(dllexport)
# else
#   define QTCHARTPLUGIN_EXPORT __declspec(dllimport)
# endif
#else
# define QTCHARTPLUGIN_EXPORT
#endif

#endif
