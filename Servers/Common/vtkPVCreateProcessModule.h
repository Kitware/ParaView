/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCreateProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCreateProcessModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// This super class assumes the application is running all in one process
// with no MPI.

#ifndef __vtkPVCreateProcessModule_h
#define __vtkPVCreateProcessModule_h

#include "vtkObject.h"

class vtkProcessModule;
class vtkPVOptions;

class VTK_EXPORT vtkPVCreateProcessModule : public vtkObject
{
public:
  vtkTypeMacro(vtkPVCreateProcessModule,vtkObject);
  static vtkProcessModule* CreateProcessModule(vtkPVOptions* options);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVCreateProcessModule() {}
  ~vtkPVCreateProcessModule() {}

private:
  vtkPVCreateProcessModule(const vtkPVCreateProcessModule&); // Not implemented
  void operator=(const vtkPVCreateProcessModule&); // Not implemented
};

#endif

