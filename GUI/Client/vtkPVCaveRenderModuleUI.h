/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCaveRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCaveRenderModuleUI - UI for Cave.
// .SECTION Description
// For now this does nothing more than the superclass


#ifndef __vtkPVCaveRenderModuleUI_h
#define __vtkPVCaveRenderModuleUI_h

#include "vtkPVLODRenderModuleUI.h"

class vtkPVCaveRenderModule;

class VTK_EXPORT vtkPVCaveRenderModuleUI : public vtkPVLODRenderModuleUI
{
public:
  static vtkPVCaveRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVCaveRenderModuleUI,vtkPVLODRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPVCaveRenderModuleUI();
  ~vtkPVCaveRenderModuleUI();
 
  vtkPVCaveRenderModuleUI(const vtkPVCaveRenderModuleUI&); // Not implemented
  void operator=(const vtkPVCaveRenderModuleUI&); // Not implemented
};


#endif
