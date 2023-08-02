// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
