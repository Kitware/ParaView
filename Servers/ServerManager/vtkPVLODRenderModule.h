/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLODRenderModule - Mangages rendering and LODs.
// .SECTION Description
// This class can be used alone when running serially.
// It handles the two pipeline branches which render in parallel.
// Subclasses handle parallel rendering.

#ifndef __vtkPVLODRenderModule_h
#define __vtkPVLODRenderModule_h

#include "vtkPVSimpleRenderModule.h"

class vtkPVTreeComposite;

class VTK_EXPORT vtkPVLODRenderModule : public vtkPVSimpleRenderModule
{
public:
  static vtkPVLODRenderModule* New();
  vtkTypeRevisionMacro(vtkPVLODRenderModule,vtkPVSimpleRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // In Addition to the superclass call, this method sets up
  // abort check observer on the render widnow.
  virtual void SetProcessModule(vtkProcessModule *pm);

  // Description:
  // This method makes the descision on whether to use LOD for rendering.
  virtual void InteractiveRender();

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  vtkSetMacro(LODThreshold, float);
  vtkGetMacro(LODThreshold, float);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetLODResolution(int);
  vtkGetMacro(LODResolution, int);

  // I might be able to make these private now that I moved them from 
  // RenderWindow to this class. !!!

  // Needed so to make global LOD descision.
  unsigned long GetTotalVisibleGeometryMemorySize();

  // Subclass create their own vtkSMPartDisplay object by
  // implementing this method.
  virtual vtkSMPartDisplay* CreatePartDisplay();
  
protected:
  vtkPVLODRenderModule();
  ~vtkPVLODRenderModule();

  // Move these to a render module when it is created.
  void ComputeTotalVisibleMemorySize();
  unsigned long TotalVisibleGeometryMemorySize;
  unsigned long TotalVisibleLODMemorySize;

  float LODThreshold;
  int   LODResolution;

  unsigned long AbortCheckTag;

  vtkPVLODRenderModule(const vtkPVLODRenderModule&); // Not implemented
  void operator=(const vtkPVLODRenderModule&); // Not implemented
};


#endif
