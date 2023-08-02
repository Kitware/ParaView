// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVLastSelectionInformation.h"

#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkSelection.h"

vtkStandardNewMacro(vtkPVLastSelectionInformation);
//----------------------------------------------------------------------------
vtkPVLastSelectionInformation::vtkPVLastSelectionInformation() = default;

//----------------------------------------------------------------------------
vtkPVLastSelectionInformation::~vtkPVLastSelectionInformation() = default;

//----------------------------------------------------------------------------
void vtkPVLastSelectionInformation::CopyFromObject(vtkObject* obj)
{
  this->Initialize();

  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(obj);
  if (pvview)
  {
    this->GetSelection()->ShallowCopy(pvview->GetLastSelection());
  }
}

//----------------------------------------------------------------------------
void vtkPVLastSelectionInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
