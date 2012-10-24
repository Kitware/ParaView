/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMantaView VTK level view that uses Manta instead of GL
// .SECTION Description
// A 3D view that uses the manta ray tracer instead of openGL for rendering

#ifndef __vtkPVMantaView_h
#define __vtkPVMantaView_h

#include "vtkPVRenderView.h"

class vtkDataRepresentation;

class VTK_EXPORT vtkPVMantaView : public vtkPVRenderView
{
public:
  static vtkPVMantaView* New();
  vtkTypeMacro(vtkPVMantaView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

  // Description:
  //Controls number of render threads.
  virtual void SetThreads(int val);
  vtkGetMacro(Threads, int);

  // Description:
  // Parameters that controls ray tracing quality
  // Defaults are for minimal quality and maximal speed.
  virtual void SetEnableShadows(int val);
  vtkGetMacro(EnableShadows, int);
  virtual void SetSamples(int val);
  vtkGetMacro(Samples, int);
  virtual void SetMaxDepth(int val);
  vtkGetMacro(MaxDepth, int);

  // Overridden to ensure that we always use an vtkOpenGLCamera of the 2D
  // renderer.
  virtual void SetActiveCamera(vtkCamera*);

//BTX
protected:
  vtkPVMantaView();
  ~vtkPVMantaView();

  int EnableShadows;
  int Threads;
  int Samples;
  int MaxDepth;

private:
  vtkPVMantaView(const vtkPVMantaView&); // Not implemented
  void operator=(const vtkPVMantaView&); // Not implemented
//ETX
};

#endif
