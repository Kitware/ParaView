/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentedArrayListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRepresentedArrayListDomain.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMRepresentationProxy.h"

vtkStandardNewMacro(vtkSMRepresentedArrayListDomain);
//----------------------------------------------------------------------------
vtkSMRepresentedArrayListDomain::vtkSMRepresentedArrayListDomain()
{
  this->ObserverId = 0;
}

//----------------------------------------------------------------------------
vtkSMRepresentedArrayListDomain::~vtkSMRepresentedArrayListDomain()
{
  if (this->RepresentationProxy && this->ObserverId)
    {
    this->RepresentationProxy->RemoveObserver(this->ObserverId);
    this->ObserverId = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::Update(vtkSMProperty* property)
{
  // When the update happens the first time, save a reference to the
  // representation proxy and add observers so that we can monitor the
  // representation updates.
  vtkSMRepresentationProxy* selfProxy = (this->GetProperty()?
    vtkSMRepresentationProxy::SafeDownCast(this->GetProperty()->GetParent()) : NULL);
  this->SetRepresentationProxy(selfProxy);
  this->Superclass::Update(property);
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::SetRepresentationProxy(
  vtkSMRepresentationProxy* repr)
{
  if (this->RepresentationProxy != repr)
    {
    if (this->RepresentationProxy && this->ObserverId)
      {
      this->RepresentationProxy->RemoveObserver(this->ObserverId);
      }
    this->RepresentationProxy = repr;
    if (repr)
      {
      this->ObserverId = this->RepresentationProxy->AddObserver(
        vtkCommand::UpdateDataEvent,
        this,
        &vtkSMRepresentedArrayListDomain::OnRepresentationDataUpdated);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::OnRepresentationDataUpdated()
{
  this->Update(NULL);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentedArrayListDomain::GetExtraDataInformation()
{
  return this->RepresentationProxy?
    this->RepresentationProxy->GetRepresentedDataInformation() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
