/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNumberOfComponentsDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNumberOfComponentsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMNumberOfComponentsDomain);
//----------------------------------------------------------------------------
vtkSMNumberOfComponentsDomain::vtkSMNumberOfComponentsDomain()
{
  this->EnableMagnitude = false;
}

//----------------------------------------------------------------------------
vtkSMNumberOfComponentsDomain::~vtkSMNumberOfComponentsDomain() = default;

//---------------------------------------------------------------------------
int vtkSMNumberOfComponentsDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  // Check for enable magnitude
  int enable_magnitude = 0;
  if (element->GetScalarAttribute("enable_magnitude", &enable_magnitude))
  {
    this->EnableMagnitude = (enable_magnitude != 0);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSMNumberOfComponentsDomain::Update(vtkSMProperty*)
{
  vtkSMProxyProperty* ip = vtkSMProxyProperty::SafeDownCast(this->GetRequiredProperty("Input"));
  vtkSMStringVectorProperty* svp =
    vtkSMStringVectorProperty::SafeDownCast(this->GetRequiredProperty("ArraySelection"));
  if (!ip || !svp)
  {
    // Missing required properties.
    this->RemoveAllEntries();
    return;
  }

  if (svp->GetNumberOfUncheckedElements() != 5 && svp->GetNumberOfUncheckedElements() != 2 &&
    svp->GetNumberOfUncheckedElements() != 1)
  {
    // We can only handle array selection properties with 5, 2 or 1 elements.
    // For 5 elements the array name is at indices [4]; for 2
    // elements it's at [1], while for 1 elements, it's at [0].
    this->RemoveAllEntries();
    return;
  }

  int index = svp->GetNumberOfUncheckedElements() - 1;
  const char* arrayName = svp->GetUncheckedElement(index);
  if (!arrayName || arrayName[0] == 0)
  {
    // No array chosen.
    this->RemoveAllEntries();
    return;
  }

  vtkSMInputArrayDomain* iad = nullptr;
  vtkSMDomainIterator* di = ip->NewDomainIterator();
  di->Begin();
  while (!di->IsAtEnd())
  {
    // We have to figure out whether we are working with cell data,
    // point data or both.
    iad = vtkSMInputArrayDomain::SafeDownCast(di->GetDomain());
    if (iad)
    {
      break;
    }
    di->Next();
  }
  di->Delete();
  if (!iad)
  {
    // Failed to locate a vtkSMInputArrayDomain on the input property, which is
    // required.
    this->RemoveAllEntries();
    return;
  }

  vtkSMInputProperty* inputProp = vtkSMInputProperty::SafeDownCast(ip);
  unsigned int i;
  unsigned int numProxs = ip->GetNumberOfUncheckedProxies();
  for (i = 0; i < numProxs; i++)
  {
    // Use the first input
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(ip->GetUncheckedProxy(i));
    if (source)
    {
      this->Update(arrayName, source, iad,
        (inputProp ? inputProp->GetUncheckedOutputPortForConnection(i) : 0));
      return;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMNumberOfComponentsDomain::Update(
  const char* arrayName, vtkSMSourceProxy* sp, vtkSMInputArrayDomain* iad, int outputport)
{
  // Make sure the outputs are created.
  sp->CreateOutputPorts();
  vtkPVDataInformation* info = sp->GetDataInformation(outputport);
  if (!info)
  {
    this->RemoveAllEntries();
    return;
  }

  int iadAttributeType = iad->GetAttributeType();
  vtkPVArrayInformation* ai = nullptr;

  if (iadAttributeType == vtkSMInputArrayDomain::POINT ||
    iadAttributeType == vtkSMInputArrayDomain::ANY ||
    iadAttributeType == vtkSMInputArrayDomain::ANY_EXCEPT_FIELD)
  {
    ai = info->GetPointDataInformation()->GetArrayInformation(arrayName);
  }
  if (iadAttributeType == vtkSMInputArrayDomain::CELL ||
    ((iadAttributeType == vtkSMInputArrayDomain::ANY ||
       iadAttributeType == vtkSMInputArrayDomain::ANY_EXCEPT_FIELD) &&
        !ai))
  {
    ai = info->GetCellDataInformation()->GetArrayInformation(arrayName);
  }
  if (iadAttributeType == vtkSMInputArrayDomain::VERTEX ||
    ((iadAttributeType == vtkSMInputArrayDomain::ANY ||
       iadAttributeType == vtkSMInputArrayDomain::ANY_EXCEPT_FIELD) &&
        !ai))
  {
    ai = info->GetVertexDataInformation()->GetArrayInformation(arrayName);
  }
  if (iadAttributeType == vtkSMInputArrayDomain::EDGE ||
    ((iadAttributeType == vtkSMInputArrayDomain::ANY ||
       iadAttributeType == vtkSMInputArrayDomain::ANY_EXCEPT_FIELD) &&
        !ai))
  {
    ai = info->GetEdgeDataInformation()->GetArrayInformation(arrayName);
  }
  if (iadAttributeType == vtkSMInputArrayDomain::ROW ||
    ((iadAttributeType == vtkSMInputArrayDomain::ANY ||
       iadAttributeType == vtkSMInputArrayDomain::ANY_EXCEPT_FIELD) &&
        !ai))
  {
    ai = info->GetRowDataInformation()->GetArrayInformation(arrayName);
  }

  if (ai)
  {
    this->RemoveAllEntries();
    for (int i = 0; i < ai->GetNumberOfComponents(); i++)
    {
      const char* name = ai->GetComponentName(i);
      this->AddEntry(name, i);
    }
    if (this->EnableMagnitude && ai->GetNumberOfComponents() > 1)
    {
      this->AddEntry("Magnitude", ai->GetNumberOfComponents());
    }
    this->DomainModified();
  }
}

//----------------------------------------------------------------------------
void vtkSMNumberOfComponentsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
