/*=========================================================================

  Program:   ParaView
  Module:    vtkSMApplication.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMApplication -
// .SECTION Description

#ifndef __vtkSMApplication_h
#define __vtkSMApplication_h

#include "vtkSMObject.h"

class VTK_EXPORT vtkSMApplication : public vtkSMObject
{
public:
  static vtkSMApplication* New();
  vtkTypeRevisionMacro(vtkSMApplication, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void Initialize();

  // Description:
  void Finalize();

protected:
  vtkSMApplication();
  ~vtkSMApplication();

private:
  vtkSMApplication(const vtkSMApplication&); // Not implemented
  void operator=(const vtkSMApplication&); // Not implemented
};

#endif
