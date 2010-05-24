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
// .NAME vtkPVRenderView
// .SECTION Description
// vtkRenderView equivalent that is specialized for ParaView. Eventually
// vtkRenderView should have a abstract base-class that this will derive from
// instead of vtkRenderView since we do not use the labelling/icon stuff from
// vtkRenderView.

#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkRenderView.h"
class vtkPVSynchronizedRenderWindows;

class VTK_EXPORT vtkPVRenderView : public vtkRenderView
{
public:
  static vtkPVRenderView* New();
  vtkTypeRevisionMacro(vtkPVRenderView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Gets the non-composited renderer for this view.
  vtkGetObjectMacro(NonCompositedRenderer, vtkRenderer);

  // Description:
  // Initialize the view with an identifier.
  void Initialize(unsigned int id);

  void SetPosition(int, int);
  void SetSize(int, int);

//BTX
protected:
  vtkPVRenderView();
  ~vtkPVRenderView();

  vtkRenderer* NonCompositedRenderer;
  vtkPVSynchronizedRenderWindows* SynchronizedWindows;
  unsigned int Identifier;

private:
  vtkPVRenderView(const vtkPVRenderView&); // Not implemented
  void operator=(const vtkPVRenderView&); // Not implemented
//ETX
};

#endif
