/*------------------------------------------------------------------------------*/
/**
 *  \file   Stdafx.h
 *  \brief  Include for precompiled header.
 */
/*------------------------------------------------------------------------------*/
#ifndef _GW_CORE_STDAFX_H_
#define _GW_CORE_STDAFX_H_

#pragma once

/* 'identifier' : identifier was truncated to 'number' characters in the debug information */
#if defined(_MSC_VER)
#pragma warning( disable : 4786 )
#endif

//-------------------------------------------------------------------------
/** \name C++ STL */
//-------------------------------------------------------------------------
//@{
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
using std::string;
using std::cerr;
using std::cout;
using std::endl;
//@}

//-------------------------------------------------------------------------
/** \name classical ANSI c libraries */
//-------------------------------------------------------------------------
//@{
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
// #include <malloc.h>
#include <memory.h>
//#include <tchar.h>
#include <ios>
#include <stdarg.h>
//#include <conio.h>
//#include <crtdbg.h>
//@}
#include "../gw_core/GW_MathsWrapper.h"


/*----------------------------------------------------------------------*/
/* win32 #include                                                       */
/*----------------------------------------------------------------------*/

#ifdef _WIN32
    /** main win32 header */
//    #include <windows.h>
#endif // _USE_WIN32_


#endif // _GW_CORE_STDAFX_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) Gabriel Peyré
///////////////////////////////////////////////////////////////////////////////
//                               END OF FILE                                 //
///////////////////////////////////////////////////////////////////////////////
