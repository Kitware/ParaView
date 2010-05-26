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
class vtkPVSynchronizedRenderer;
class vtkCamera;

class VTK_EXPORT vtkPVRenderView : public vtkRenderView
{
public:
  static vtkPVRenderView* New();
  vtkTypeRevisionMacro(vtkPVRenderView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  void Initialize(unsigned int id);

  // Description:
  // Set the position on this view in the multiview configuration.
  // This can be called only after Initialize().
  void SetPosition(int, int);

  // Description:
  // Set the size of this view in the multiview configuration.
  // This can be called only after Initialize().
  void SetSize(int, int);

  // Description:
  // Gets the non-composited renderer for this view. This is typically used for
  // labels, 2D annotations etc.
  vtkGetObjectMacro(NonCompositedRenderer, vtkRenderer);

  // Description:
  // Get the active camera.
  vtkCamera* GetActiveCamera();

//BTX
protected:
  vtkPVRenderView();
  ~vtkPVRenderView();

  vtkRenderer* NonCompositedRenderer;
  vtkPVSynchronizedRenderWindows* SynchronizedWindows;
  vtkPVSynchronizedRenderer* SynchronizedRenderers;
  unsigned int Identifier;

private:
  vtkPVRenderView(const vtkPVRenderView&); // Not implemented
  void operator=(const vtkPVRenderView&); // Not implemented
//ETX
};

#endif
