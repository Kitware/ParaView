/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionQueryDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionQueryDomain.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

class vtkSMSelectionQueryDomain::vtkInternals
{
public:
  int LastFieldAssociation = -1;
  vtkWeakPointer<vtkSMProxy> LastInputProxy;
  unsigned int LastInputPort;
  unsigned long LastInputProxyObserverId = 0;
};

vtkStandardNewMacro(vtkSMSelectionQueryDomain);
//----------------------------------------------------------------------------
vtkSMSelectionQueryDomain::vtkSMSelectionQueryDomain()
  : Internals(new vtkSMSelectionQueryDomain::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMSelectionQueryDomain::~vtkSMSelectionQueryDomain()
{
  auto& internals = (*this->Internals);
  if (internals.LastInputProxy && internals.LastInputProxyObserverId != 0)
  {
    internals.LastInputProxy->RemoveObserver(internals.LastInputProxyObserverId);
  }

  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkSMSelectionQueryDomain::Update(vtkSMProperty*)
{
  auto& internals = (*this->Internals);
  vtkSMUncheckedPropertyHelper fieldAssociationHelper(
    this->GetRequiredProperty("FieldAssociation"));
  vtkSMUncheckedPropertyHelper inputHelper(this->GetRequiredProperty("Input"));

  // check if the domain has truly changed and fire DomainModifiedEvent iff
  // that's the case.
  if (internals.LastFieldAssociation != fieldAssociationHelper.GetAsInt() ||
    internals.LastInputProxy != inputHelper.GetAsProxy() ||
    internals.LastInputPort != inputHelper.GetOutputPort())
  {
    internals.LastFieldAssociation = fieldAssociationHelper.GetAsInt();

    if (internals.LastInputProxy && internals.LastInputProxyObserverId != 0)
    {
      internals.LastInputProxy->RemoveObserver(internals.LastInputProxyObserverId);
    }

    internals.LastInputProxy = inputHelper.GetAsProxy();
    internals.LastInputPort = inputHelper.GetOutputPort();
    internals.LastInputProxyObserverId = 0;
    if (internals.LastInputProxy)
    {
      // observer-data updated event since that may result in changes to data
      // information.
      internals.LastInputProxyObserverId = internals.LastInputProxy->AddObserver(
        vtkCommand::UpdateDataEvent, this, &vtkSMSelectionQueryDomain::DomainModified);
    }

    this->DomainModified();
  }
}

//----------------------------------------------------------------------------
int vtkSMSelectionQueryDomain::GetElementType()
{
  vtkSMUncheckedPropertyHelper fieldAssociationHelper(
    this->GetRequiredProperty("FieldAssociation"));
  return fieldAssociationHelper.GetAsInt();
}

//----------------------------------------------------------------------------
void vtkSMSelectionQueryDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
