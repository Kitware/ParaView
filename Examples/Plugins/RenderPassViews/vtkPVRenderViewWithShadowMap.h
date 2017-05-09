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
// .NAME vtkPVRenderViewWithShadowMap
// .SECTION Description
// vtkPVRenderViewWithShadowMap is a vtkPVRenderView specialization that uses
// shadow-map render passes for rendering shadows.

#ifndef vtkPVRenderViewWithShadowMap_h
#define vtkPVRenderViewWithShadowMap_h

#include "vtkPVRenderView.h"

class VTK_EXPORT vtkPVRenderViewWithShadowMap : public vtkPVRenderView
{
public:
  static vtkPVRenderViewWithShadowMap* New();
  vtkTypeMacro(vtkPVRenderViewWithShadowMap, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // \note CallOnAllProcesses
  virtual void Initialize(unsigned int id);

protected:
  vtkPVRenderViewWithShadowMap();
  ~vtkPVRenderViewWithShadowMap();

private:
  vtkPVRenderViewWithShadowMap(const vtkPVRenderViewWithShadowMap&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVRenderViewWithShadowMap&) VTK_DELETE_FUNCTION;
};

#endif
