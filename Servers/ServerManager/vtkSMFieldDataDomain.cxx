/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFieldDataDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFieldDataDomain.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMFieldDataDomain);
vtkCxxRevisionMacro(vtkSMFieldDataDomain, "1.8");

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::vtkSMFieldDataDomain()
{
}

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::~vtkSMFieldDataDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::CheckForArray(
  vtkSMSourceProxy* sp,
  int outputport,
  vtkPVDataSetAttributesInformation* info, 
  vtkSMInputArrayDomain* iad)
{
  int num = info->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    if ( iad->IsFieldValid(sp, outputport, info->GetArrayInformation(idx), 1) )
      {
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMSourceProxy* sp, 
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

  if (this->CheckForArray(sp, outputport, info->GetPointDataInformation(), iad))
    {
    this->AddEntry("Point Data", vtkDataObject::FIELD_ASSOCIATION_POINTS);
    }

  if (this->CheckForArray(sp, outputport, info->GetCellDataInformation(), iad))
    {
    this->AddEntry("Cell Data",  vtkDataObject::FIELD_ASSOCIATION_CELLS);
    }

  if (this->CheckForArray(sp, outputport, info->GetVertexDataInformation(), iad))
    {
    this->AddEntry("Vertex Data", vtkDataObject::FIELD_ASSOCIATION_VERTICES);
    }

  if (this->CheckForArray(sp, outputport, info->GetEdgeDataInformation(), iad))
    {
    this->AddEntry("Edge Data", vtkDataObject::FIELD_ASSOCIATION_EDGES);
    }

  if (this->CheckForArray(sp, outputport, info->GetRowDataInformation(), iad))
    {
    this->AddEntry("Row Data", vtkDataObject::FIELD_ASSOCIATION_ROWS);
    }

  this->InvokeModified();
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMProxyProperty* pp, 
                                  vtkSMSourceProxy* sp,
                                  int outputport)
{
  vtkSMDomainIterator* di = pp->NewDomainIterator();
  di->Begin();
  while (!di->IsAtEnd())
    {
    vtkSMInputArrayDomain* iad = vtkSMInputArrayDomain::SafeDownCast(
      di->GetDomain());
    if (iad)
      {
      this->Update(sp, iad, outputport);
      break;
      }
    di->Next();
    }
  di->Delete();
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMProperty*)
{
  this->RemoveAllEntries();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (!pp)
    {
    return;
    }
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);

  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  unsigned int i;

  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      this->Update(pp, sp, 
        (ip? ip->GetUncheckedOutputPortForConnection(i):0));
      return;
      }
    }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = pp->GetNumberOfProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetProxy(i));
    if (sp)
      {
      this->Update(pp, sp,
        (ip? ip->GetOutputPortForConnection(i):0));
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
