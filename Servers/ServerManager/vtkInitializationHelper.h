/*=========================================================================

  Program:   ParaView
  Module:    vtkInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInitializationHelper
// .SECTION Description

#ifndef __vtkInitializationHelper_h
#define __vtkInitializationHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkInitializationHelper : public vtkObject
{
public: 
  vtkTypeRevisionMacro(vtkInitializationHelper,vtkObject);

  static void Initialize();

protected:
  vtkInitializationHelper() {};
  virtual ~vtkInitializationHelper() {};

private:

  vtkInitializationHelper(const vtkInitializationHelper&); // Not implemented
  void operator=(const vtkInitializationHelper&); // Not implemented
};

#endif
