/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderModuleUI - User interface for a rendering module.
// .SECTION Description
// This is a superclass for a render module.  We do not create this class ,
// This is created by the vtkPVRenderView.

#ifndef __vtkPVRenderModuleUI_h
#define __vtkPVRenderModuleUI_h

#include "vtkPVTracedWidget.h"

class vtkPVApplication;
class vtkSMRenderModuleProxy;
class vtkKWCheckButton;
class vtkKWFrameWithLabel;

class VTK_EXPORT vtkPVRenderModuleUI : public vtkPVTracedWidget
{
public:
  static vtkPVRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVRenderModuleUI,vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
      
  // Description:
  // This method is called right before the application starts its
  // main loop.  It was created to disable compositing after the 
  // server information in the process module is valid.
  virtual void Initialize() {};

  // Description:
  // Sets the render module proxy.
  void SetRenderModuleProxy(vtkSMRenderModuleProxy*);
  vtkGetObjectMacro(RenderModuleProxy, vtkSMRenderModuleProxy);

  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Export the render module to a file.
  virtual void SaveState(ofstream *) {}
  
  // Description:
  // This threshold determines the default representation that will be
  // used for unstructured grid.  The units of this value are numer of cells.
  vtkGetMacro(OutlineThreshold, float);

  // Description:
  // This method Sets all IVars to NULL and unregisters
  // vtk objects.  This should eliminate circular references.
  void PrepareForDelete();

  // Description:
  // Resets all the settings to default values.
  virtual void ResetSettingsToDefault();

  // Description:
  void MeasurePolygonsPerSecondCallback(int val);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkPVRenderModuleUI();
  ~vtkPVRenderModuleUI();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkSMRenderModuleProxy* RenderModuleProxy;

  vtkKWFrameWithLabel *RenderModuleFrame;
  vtkKWCheckButton* MeasurePolygonsPerSecondFlag;

  float OutlineThreshold;
 
  vtkPVRenderModuleUI(const vtkPVRenderModuleUI&); // Not implemented
  void operator=(const vtkPVRenderModuleUI&); // Not implemented
};


#endif
