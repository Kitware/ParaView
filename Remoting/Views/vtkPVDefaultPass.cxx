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
#include "vtkOpenGLError.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVDefaultPass);
//----------------------------------------------------------------------------
vtkPVDefaultPass::vtkPVDefaultPass() = default;

//----------------------------------------------------------------------------
vtkPVDefaultPass::~vtkPVDefaultPass() = default;

//----------------------------------------------------------------------------
void vtkPVDefaultPass::Render(const vtkRenderState* render_state)
{
  vtkOpenGLClearErrorMacro();

  vtkRenderer* renderer = render_state->GetRenderer();
  this->ClearLights(renderer);
  this->UpdateLightGeometry(renderer);
  this->UpdateLights(renderer);

  this->UpdateGeometry(renderer, render_state->GetFrameBuffer());

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkPVDefaultPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
