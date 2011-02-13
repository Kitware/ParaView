/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPID.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPID - Get the process id information.
// .SECTION Description
// Allow to query the system in order to get the process id

#ifndef __vtkPID_h
#define __vtkPID_h

#include "vtkObject.h"

class VTK_EXPORT vtkPID : public vtkObject
{
public:
  static vtkPID* New();
  vtkTypeMacro(vtkPID, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  int GetPID();

  int GetParentPID();
protected:
  vtkPID();
  ~vtkPID();

private:
  vtkPID(const vtkPID&); // Not implemented.
  void operator=(const vtkPID&); // Not implemented.
};

#endif
