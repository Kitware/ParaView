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
// This render module user interface controls ICE-T tile display compositing.


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

  void Create(vtkKWApplication *app, const char *);

  // Description:
  // Threshold for collecting geometry to the client (vs. showing the outline
  // on the client).
  void CollectCheckCallback();
  void CollectThresholdScaleCallback();
  void CollectThresholdLabelCallback();
  void SetCollectThreshold(float val);
  vtkGetMacro(CollectThreshold, float);

protected:
  vtkPVIceTRenderModuleUI();
  ~vtkPVIceTRenderModuleUI();

  vtkKWLabel       *CollectLabel;
  vtkKWCheckButton *CollectCheck;
  vtkKWScale       *CollectThresholdScale;
  vtkKWLabel       *CollectThresholdLabel;
  float             CollectThreshold;

  vtkPVIceTRenderModuleUI(const vtkPVIceTRenderModuleUI&); // Not implemented
  void operator=(const vtkPVIceTRenderModuleUI&); // Not implemented
};


#endif
