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
// .NAME vtkPVRenderViewWithSobel
// .SECTION Description
// vtkPVRenderViewWithSobel demonstrates how to create custom render-view
// subclasses that use a image-processing render pass for processing the image
// before rendering it on the screen.

#ifndef vtkPVRenderViewWithSobel_h
#define vtkPVRenderViewWithSobel_h

#include "vtkPVRenderView.h"

class VTK_EXPORT vtkPVRenderViewWithSobel : public vtkPVRenderView
{
public:
  static vtkPVRenderViewWithSobel* New();
  vtkTypeMacro(vtkPVRenderViewWithSobel, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // \note CallOnAllProcesses
  virtual void Initialize(unsigned int id);

protected:
  vtkPVRenderViewWithSobel();
  ~vtkPVRenderViewWithSobel();

private:
  vtkPVRenderViewWithSobel(const vtkPVRenderViewWithSobel&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVRenderViewWithSobel&) VTK_DELETE_FUNCTION;
};

#endif
