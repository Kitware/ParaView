/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMultiDisplayRenderModuleUI - UI for MPI and Client server.
// .SECTION Description
// For the moment, This subclass does nothing.
// I will specialize the UI in the future.
// We need a class of this name because of the way 
// RenderModuleName is used to create the classes.
// In the future, we will use XML ...


#ifndef __vtkPVMultiDisplayRenderModuleUI_h
#define __vtkPVMultiDisplayRenderModuleUI_h

#include "vtkPVCompositeRenderModuleUI.h"

class vtkPVMultiDisplayRenderModule;

class VTK_EXPORT vtkPVMultiDisplayRenderModuleUI : public vtkPVCompositeRenderModuleUI
{
public:
  static vtkPVMultiDisplayRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVMultiDisplayRenderModuleUI,vtkPVCompositeRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Create(vtkKWApplication *app, const char *);

protected:
  vtkPVMultiDisplayRenderModuleUI();
  ~vtkPVMultiDisplayRenderModuleUI();
 
  vtkPVMultiDisplayRenderModuleUI(const vtkPVMultiDisplayRenderModuleUI&); // Not implemented
  void operator=(const vtkPVMultiDisplayRenderModuleUI&); // Not implemented
};


#endif
