/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLastSelectionInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
