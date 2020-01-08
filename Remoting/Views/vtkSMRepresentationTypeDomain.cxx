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
#include "vtkSMRepresentationTypeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMRepresentationTypeDomain);
//----------------------------------------------------------------------------
vtkSMRepresentationTypeDomain::vtkSMRepresentationTypeDomain()
{
}

//----------------------------------------------------------------------------
vtkSMRepresentationTypeDomain::~vtkSMRepresentationTypeDomain()
{
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentationTypeDomain::GetInputInformation()
{
  vtkSMProperty* inputProperty = this->GetRequiredProperty("Input");
  if (!inputProperty)
  {
    return NULL;
  }

  vtkSMUncheckedPropertyHelper helper(inputProperty);
  if (helper.GetNumberOfElements() > 0)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
    if (sp)
    {
      return sp->GetDataInformation(helper.GetOutputPort());
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkSMRepresentationTypeDomain::SetDefaultValues(
  vtkSMProperty* property, bool use_unchecked_values)
{
  vtkPVDataInformation* info = this->GetInputInformation();
  if (!info || this->GetNumberOfStrings() <= 1)
  {
    return this->Superclass::SetDefaultValues(property, use_unchecked_values);
  }

  vtkSMPropertyHelper helper(property);
  helper.SetUseUnchecked(use_unchecked_values);

  unsigned int temp;

  // if number of cells > user-specified threshold, render as outline by
  // default.
  vtkIdType numCells = vtkPVRenderViewSettings::GetInstance()->GetOutlineThreshold() * 1e6;
  if (info->GetNumberOfCells() >= numCells && this->IsInDomain("Outline", temp))
  {
    helper.Set("Outline");
    return 1;
  }

  // for vtkImageData show slice (for 2D datasets) or outline (for all others).
  if (info->DataSetTypeIsA("vtkImageData"))
  {
    const int* ext = info->GetExtent();
    if ((info->GetCompositeDataSetType() == -1) &&
      (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5]) && this->IsInDomain("Slice", temp))
    {
      helper.Set("Slice");
      return 1;
    }
    else if (this->IsInDomain("Outline", temp))
    {
      helper.Set("Outline");
      return 1;
    }
  }

  // for other structured data, show surface for 2D and outline for all others.
  if (info->IsDataStructured())
  {
    const int* ext = info->GetExtent();
    if ((ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5]) &&
      this->IsInDomain("Surface", temp))
    {
      helper.Set("Surface");
      return 1;
    }
    else if (this->IsInDomain("Outline", temp))
    {
      helper.Set("Outline");
      return 1;
    }
  }

  if (this->IsInDomain("Surface", temp))
  {
    helper.Set("Surface");
    return 1;
  }

  return this->Superclass::SetDefaultValues(property, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationTypeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
