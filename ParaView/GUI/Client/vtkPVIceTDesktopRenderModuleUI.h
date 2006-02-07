/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTDesktopRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIceTDesktopRenderModuleUI - UI for MPI and Client server.
// .SECTION Description
// For the moment, This subclass does nothing.
// I will specialize the UI in the future.
// We need a class of this name because of the way 
// RenderModuleName is used to create the classes.
// In the future, we will use XML ...


#ifndef __vtkPVIceTDesktopRenderModuleUI_h
#define __vtkPVIceTDesktopRenderModuleUI_h

#include "vtkPVMPIRenderModuleUI.h"

class vtkPVIceTDesktopRenderModule;

class VTK_EXPORT vtkPVIceTDesktopRenderModuleUI : public vtkPVMPIRenderModuleUI
{
public:
  static vtkPVIceTDesktopRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVIceTDesktopRenderModuleUI,vtkPVMPIRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  void Create();

  // Description:
  // Callback for the ordered composite check button.
  virtual void SetOrderedCompositingFlag(int state);

  void EnableRenductionFactor();

  // Description:
  // Resets all the settings to default values.
  virtual void ResetSettingsToDefault();

protected:
  vtkPVIceTDesktopRenderModuleUI();
  ~vtkPVIceTDesktopRenderModuleUI();

  vtkKWCheckButton *OrderedCompositingCheck;
  int               OrderedCompositingFlag;

  vtkPVIceTDesktopRenderModuleUI(const vtkPVIceTDesktopRenderModuleUI&); // Not implemented
  void operator=(const vtkPVIceTDesktopRenderModuleUI&); // Not implemented
};


#endif
