// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    warningState.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef warningState_h
#define warningState_h

#if defined(DISABLE_NOISY_COMPILE_WARNINGS)
// disables certain "noisy" compile warnings (e.g. Qt creates a plethora of 4127 and 4512)
// if DISABLE_NOISY_COMPILE_WARNINGS is defined
#if defined(_MSC_VER)
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#pragma warning(disable : 4512) // warning C4512: assignment operator could not be generated
#endif
#endif

#endif // warningState_h
