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

#include "vtkKWWidget.h"

class vtkPVApplication;
class vtkPVRenderModule;

class VTK_EXPORT vtkPVRenderModuleUI : public vtkKWWidget
{
public:
  static vtkPVRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVRenderModuleUI,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
      
  // Description:
  // This method is called right before the application starts its
  // main loop.  It was created to disable compositing after the 
  // server information in the process module is valid.
  virtual void Initialize() {};
   
  // Description:
  // The subclass should implement this method and 
  // downcast it to the right type.  It can then access
  // any unique methods of the specific render module.
  virtual void SetRenderModule(vtkPVRenderModule *rm);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication* app, const char *args);

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

protected:
  vtkPVRenderModuleUI();
  ~vtkPVRenderModuleUI();

  float OutlineThreshold;
 
  vtkPVRenderModuleUI(const vtkPVRenderModuleUI&); // Not implemented
  void operator=(const vtkPVRenderModuleUI&); // Not implemented
};


#endif
