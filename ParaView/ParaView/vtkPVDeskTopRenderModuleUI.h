/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDeskTopRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDeskTopRenderModuleUI - UI for MPI and Client server.
// .SECTION Description
// For the moment, This subclass does nothing.
// I will specialize the UI in the future.
// We need a class of this name because of the way 
// RenderModuleName is used to create the classes.
// In the future, we will use XML ...


#ifndef __vtkPVDeskTopRenderModuleUI_h
#define __vtkPVDeskTopRenderModuleUI_h

#include "vtkPVMPIRenderModuleUI.h"

class vtkPVDeskTopRenderModule;

class VTK_EXPORT vtkPVDeskTopRenderModuleUI : public vtkPVMPIRenderModuleUI
{
public:
  static vtkPVDeskTopRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVDeskTopRenderModuleUI,vtkPVMPIRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Create(vtkKWApplication *app, const char *);

  void EnableRenductionFactor();

protected:
  vtkPVDeskTopRenderModuleUI();
  ~vtkPVDeskTopRenderModuleUI();

  vtkPVDeskTopRenderModuleUI(const vtkPVDeskTopRenderModuleUI&); // Not implemented
  void operator=(const vtkPVDeskTopRenderModuleUI&); // Not implemented
};


#endif
