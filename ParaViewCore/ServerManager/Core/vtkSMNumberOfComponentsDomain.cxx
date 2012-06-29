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
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMNumberOfComponentsDomain);
//----------------------------------------------------------------------------
vtkSMNumberOfComponentsDomain::vtkSMNumberOfComponentsDomain()
{
}

//----------------------------------------------------------------------------
vtkSMNumberOfComponentsDomain::~vtkSMNumberOfComponentsDomain()
{
}

//----------------------------------------------------------------------------
void vtkSMNumberOfComponentsDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
  this->RemoveAllMinima();
  this->RemoveAllMaxima();
  this->AddMinimum(0, 0);
  this->AddMaximum(0, 0);

  vtkSMProxyProperty* ip = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetRequiredProperty("ArraySelection"));
  if (!ip || !svp)
    {
    // Missing required properties.
    return;
    }

  if (svp->GetNumberOfElements() != 5 && svp->GetNumberOfElements() != 2 && 
    svp->GetNumberOfElements() != 1)
    {
    // We can only handle array selection properties with 5, 2 or 1 elements.
    // For 5 elements the array name is at indices [4]; for 2
    // elements it's at [1], while for 1 elements, it's at [0].
    return;
    }

  int index = svp->GetNumberOfElements()-1;
  const char* arrayName = svp->GetUncheckedElement(index);
  if (!arrayName || arrayName[0] == 0)
    {
    arrayName = svp->GetElement(index);
    }
  
  if (!arrayName || arrayName[0] == 0)
    {
    // No array choosen.
    return;
    }

  vtkSMInputArrayDomain* iad = 0;
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
    return;
    }

  vtkSMInputProperty* inputProp = vtkSMInputProperty::SafeDownCast(ip);
  unsigned int i;
  unsigned int numProxs = ip->GetNumberOfUncheckedProxies();
  for (i=0; i<numProxs; i++)
    {
    // Use the first input
    vtkSMSourceProxy* source = 
      vtkSMSourceProxy::SafeDownCast(ip->GetUncheckedProxy(i));
    if (source)
      {
      this->Update(arrayName, source, iad,
        (inputProp? inputProp->GetUncheckedOutputPortForConnection(i): 0));
      return;
      }
    }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = ip->GetNumberOfProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* source = 
      vtkSMSourceProxy::SafeDownCast(ip->GetProxy(i));
    if (source)
      {
      this->Update(arrayName, source, iad,
        (inputProp? inputProp->GetOutputPortForConnection(i): 0));
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMNumberOfComponentsDomain::Update(const char* arrayName,
                                   vtkSMSourceProxy* sp,
                                   vtkSMInputArrayDomain* iad,
                                   int outputport)
{
  // Make sure the outputs are created.
  sp->CreateOutputPorts();
  vtkPVDataInformation* info = sp->GetDataInformation(outputport);

  if (!info)
    {
    return;
    }

  int iadAttributeType = iad->GetAttributeType();
  vtkPVArrayInformation* ai = 0;

  if (iadAttributeType == vtkSMInputArrayDomain::POINT ||
    iadAttributeType == vtkSMInputArrayDomain::ANY)
    {
    ai = info->GetPointDataInformation()->GetArrayInformation(arrayName);
    }
  else if (iadAttributeType == vtkSMInputArrayDomain::CELL || 
    (iadAttributeType == vtkSMInputArrayDomain::ANY && !ai))
    {
    ai = info->GetCellDataInformation()->GetArrayInformation(arrayName);
    }
  else if (iadAttributeType == vtkSMInputArrayDomain::VERTEX || 
    (iadAttributeType == vtkSMInputArrayDomain::ANY && !ai))
    {
    ai = info->GetVertexDataInformation()->GetArrayInformation(arrayName);
    }
  else if (iadAttributeType == vtkSMInputArrayDomain::EDGE || 
    (iadAttributeType == vtkSMInputArrayDomain::ANY && !ai))
    {
    ai = info->GetEdgeDataInformation()->GetArrayInformation(arrayName);
    }
  else if (iadAttributeType == vtkSMInputArrayDomain::ROW || 
    (iadAttributeType == vtkSMInputArrayDomain::ANY && !ai))
    {
    ai = info->GetRowDataInformation()->GetArrayInformation(arrayName);
    }

  if (ai)
    {
    this->AddMaximum(0, ai->GetNumberOfComponents()-1);
    this->InvokeModified();
    }
}

//----------------------------------------------------------------------------
void vtkSMNumberOfComponentsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


