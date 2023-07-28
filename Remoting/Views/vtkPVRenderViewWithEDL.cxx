// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/
#include "vtkPVRenderViewWithEDL.h"

#include "vtkEDLShading.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVRenderViewWithEDL);

//----------------------------------------------------------------------------
vtkPVRenderViewWithEDL::vtkPVRenderViewWithEDL()
{
  vtkNew<vtkEDLShading> pass;
  this->SynchronizedRenderers->SetImageProcessingPass(pass);
  this->SynchronizedRenderers->SetUseDepthBuffer(true);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewWithEDL::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
