/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMPIRenderModuleUI - UI for MPI and Client server.
// .SECTION Description
// For the moment, This subclass does nothing.
// I will specialize the UI in the future.
// We need a class of this name because of the way 
// RenderModuleName is used to create the classes.
// In the future, we will use XML ...


#ifndef __vtkPVMPIRenderModuleUI_h
#define __vtkPVMPIRenderModuleUI_h

#include "vtkPVCompositeRenderModuleUI.h"

class VTK_EXPORT vtkPVMPIRenderModuleUI : public vtkPVCompositeRenderModuleUI
{
public:
  static vtkPVMPIRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVMPIRenderModuleUI,vtkPVCompositeRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVMPIRenderModuleUI();
  ~vtkPVMPIRenderModuleUI();
 
  vtkPVMPIRenderModuleUI(const vtkPVMPIRenderModuleUI&); // Not implemented
  void operator=(const vtkPVMPIRenderModuleUI&); // Not implemented
};


#endif
