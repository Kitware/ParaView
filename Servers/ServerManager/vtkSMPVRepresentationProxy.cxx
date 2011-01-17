/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"

#include <vtkstd/string>
#include <vtkstd/map>
#include <vtkstd/set>

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->SetKernelClassName("vtkPMPVRepresentationProxy");
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return;
    }

  // Ensure that we update the RepresentationTypesInfo property and the domain
  // for "Representations" property before CreateVTKObjects() is finished. This
  // ensure that all representations have valid Representations domain.
  this->UpdatePropertyInformation();
  this->GetProperty("RepresentationTypesInfo")->UpdateDependentDomains();

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  this->AddObserver(vtkCommand::UpdatePropertyEvent,
    this, &vtkSMPVRepresentationProxy::OnPropertyUpdated);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::OnPropertyUpdated(vtkObject*,
  unsigned long, void* calldata)
{
  const char* pname = reinterpret_cast<const char*>(calldata);
  if (pname && strcmp(pname, "Representation") == 0)
    {
    this->InvalidateDataInformation();
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


