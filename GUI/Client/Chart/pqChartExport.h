/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/*!
 * \file pqChartExport.h
 * \brief
 *   Used to switch between dll import and export on windows.
 * \date August 19, 2005
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
