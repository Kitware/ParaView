/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * H5api_adpt.h
 * Used for the HDF5 dll project
 * Created by Patrick Lu on 1/12/99
 */
#ifndef H5API_ADPT_H
#define H5API_ADPT_H

#if defined(WIN32) && !defined(__CYGWIN__)

#if defined(_HDF5DLL_)
#pragma warning(disable: 4273)  /* Disable the dll linkage warnings */
#define H5_DLL __declspec(dllexport)
#define H5_DLLVAR __declspec(dllexport)
#elif defined(_HDF5USEDLL_)
#define H5_DLL __declspec(dllimport)
#define H5_DLLVAR __declspec(dllimport)
#else
#define H5_DLL
#define H5_DLLVAR extern
#endif /* _HDF5DLL_ */

#if defined(_HDF5TESTDLL_)
#pragma warning(disable: 4273)  /* Disable the dll linkage warnings */
#define H5TEST_DLL __declspec(dllexport)
#define H5TEST_DLLVAR __declspec(dllexport)
#elif defined(_HDF5TESTUSEDLL_)
#define H5TEST_DLL __declspec(dllimport)
#define H5TEST_DLLVAR __declspec(dllimport)
#else
#define H5TEST_DLL
#define H5TEST_DLLVAR extern
#endif /* _HDF5TESTDLL_ */

// Added to export or to import C++ APIs - BMR (02-15-2002)
#if defined(HDF5_CPPDLL_EXPORTS) // this name is generated at creation
#define H5_DLLCPP __declspec(dllexport)
#elif defined(HDF5CPP_USEDLL)
#define H5_DLLCPP __declspec(dllimport)
#else
#define H5_DLLCPP
#endif /* HDF5_CPPDLL_EXPORTS */

#else /*WIN32*/
#define H5_DLL
#define H5_DLLVAR extern
#define H5_DLLCPP
#define H5TEST_DLL
#define H5TEST_DLLVAR extern
#endif

#endif /* H5API_ADPT_H */
