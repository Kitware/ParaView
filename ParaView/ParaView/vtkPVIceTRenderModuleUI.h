/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIceTRenderModuleUI - UI for MPI and Client server.
// .SECTION Description
// For the moment, This subclass does nothing.
// I will specialize the UI in the future.
// We need a class of this name because of the way 
// RenderModuleName is used to create the classes.
// In the future, we will use XML ...


#ifndef __vtkPVIceTRenderModuleUI_h
#define __vtkPVIceTRenderModuleUI_h

#include "vtkPVMultiDisplayRenderModuleUI.h"

class vtkPVIceTRenderModule;

class VTK_EXPORT vtkPVIceTRenderModuleUI : public vtkPVMultiDisplayRenderModuleUI
{
public:
  static vtkPVIceTRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVIceTRenderModuleUI,vtkPVMultiDisplayRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVIceTRenderModuleUI();
  ~vtkPVIceTRenderModuleUI();

  vtkPVIceTRenderModuleUI(const vtkPVIceTRenderModuleUI&); // Not implemented
  void operator=(const vtkPVIceTRenderModuleUI&); // Not implemented
};


#endif
