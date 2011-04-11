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
// .NAME vtkPVDefaultPass - encapsulates the traditional OpenGL pipeline
// (minus the camera).
// .SECTION Description
// vtkPVDefaultPass is a simple render pass that encapsulates the traditional
// OpenGL pipeline (minus the camera).

#ifndef __vtkPVDefaultPass_h
#define __vtkPVDefaultPass_h

#include "vtkRenderPass.h"

class VTK_EXPORT vtkPVDefaultPass : public vtkRenderPass
{
public:
  static vtkPVDefaultPass* New();
  vtkTypeMacro(vtkPVDefaultPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Actual rendering code.
  virtual void Render(const vtkRenderState* render_state);

protected:
  vtkPVDefaultPass();
  ~vtkPVDefaultPass();

private:
  vtkPVDefaultPass(const vtkPVDefaultPass&); // Not implemented
  void operator=(const vtkPVDefaultPass&); // Not implemented
//ETX
};

#endif
