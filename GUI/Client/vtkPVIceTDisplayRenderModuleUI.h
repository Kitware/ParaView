/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTDisplayRenderModuleUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIceTDisplayRenderModuleUI - UI for MPI and Client server.
// .SECTION Description
// For the moment, This subclass does nothing.
// I will specialize the UI in the future.
// We need a class of this name because of the way 
// RenderModuleName is used to create the classes.
// In the future, we will use XML ...


#ifndef __vtkPVIceTDisplayRenderModuleUI_h
#define __vtkPVIceTDisplayRenderModuleUI_h

#include "vtkPVLODRenderModuleUI.h"

class vtkPVIceTDisplayRenderModule;

class VTK_EXPORT vtkPVIceTDisplayRenderModuleUI : public vtkPVLODRenderModuleUI
{
public:
  static vtkPVIceTDisplayRenderModuleUI* New();
  vtkTypeRevisionMacro(vtkPVIceTDisplayRenderModuleUI,vtkPVLODRenderModuleUI);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Tracing uses the method with the argument.
  // A reduction value of 1 is equivalent to having the feature
  // disabled.
  void ReductionCheckCallback();
  void ReductionFactorScaleCallback();
  void SetReductionFactor(int val);

  // Description:
  // Downcasts rm pointer to vtkPVIceTDisplayRenderModule for internal use.
  virtual void SetRenderModule(vtkPVRenderModule* rm);

  // Description:
  // Creates the UI.
  virtual void Create(vtkKWApplication *app, const char *);

  // Description:
  // Export the render module state to a file.
  virtual void SaveState(ofstream *file);
  
protected:
  vtkPVIceTDisplayRenderModuleUI();
  ~vtkPVIceTDisplayRenderModuleUI();
 
  vtkKWLabel*       ReductionLabel;
  vtkKWCheckButton* ReductionCheck;
  vtkKWScale*       ReductionFactorScale;
  vtkKWLabel*       ReductionFactorLabel;
  int               ReductionFactor;

  vtkPVIceTDisplayRenderModule* IceTRenderModule;

  vtkPVIceTDisplayRenderModuleUI(const vtkPVIceTDisplayRenderModuleUI&); // Not implemented
  void operator=(const vtkPVIceTDisplayRenderModuleUI&); // Not implemented
};


#endif
