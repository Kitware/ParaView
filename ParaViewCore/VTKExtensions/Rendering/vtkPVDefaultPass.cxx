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
#include "vtkPVDefaultPass.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"

#include "vtkgl.h"

vtkStandardNewMacro(vtkPVDefaultPass);
//----------------------------------------------------------------------------
vtkPVDefaultPass::vtkPVDefaultPass()
{
}

//----------------------------------------------------------------------------
vtkPVDefaultPass::~vtkPVDefaultPass()
{
}

//----------------------------------------------------------------------------
void vtkPVDefaultPass::Render(const vtkRenderState* render_state)
{
  vtkRenderer* renderer = render_state->GetRenderer();
  this->ClearLights(renderer);
  this->UpdateLightGeometry(renderer);
  this->UpdateLights(renderer);

  //// set matrix mode for actors
  GLint saved_matrix_mode;
  glGetIntegerv(GL_MATRIX_MODE, &saved_matrix_mode);
  glMatrixMode(GL_MODELVIEW);
  this->UpdateGeometry(renderer);
  glMatrixMode(saved_matrix_mode);
}

//----------------------------------------------------------------------------
void vtkPVDefaultPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
