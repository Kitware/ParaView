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
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMFieldDataDomain);

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::vtkSMFieldDataDomain()
{
  this->EnableFieldDataSelection = false;
  this->DisableUpdateDomainEntries = false;
  this->ForcePointAndCellDataSelection = false;
  this->DefaultValue = -1;
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
    if (iad == 0 || iad->IsFieldValid(sp, outputport, info->GetArrayInformation(idx), 1) )
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

  bool has_pd = 0 !=
    this->CheckForArray(sp, outputport, info->GetPointDataInformation(), iad);
  bool has_cd = 0 !=
    this->CheckForArray(sp, outputport, info->GetCellDataInformation(), iad);
  bool has_vd = 0 !=
    this->CheckForArray(sp, outputport, info->GetVertexDataInformation(), iad);
  bool has_ed = 0 !=
    this->CheckForArray(sp, outputport, info->GetEdgeDataInformation(), iad);
  bool has_rd = 0 !=
    this->CheckForArray(sp, outputport, info->GetRowDataInformation(), iad);

  if ( this->ForcePointAndCellDataSelection &&
    !(has_vd || has_ed || has_rd ) )
    {
    //only force cell & data on DataSets
    has_pd = ( info->GetNumberOfPoints() > 0);
    has_cd = ( info->GetNumberOfCells() > 0);
    }

  if (this->DisableUpdateDomainEntries || has_pd )
    {
    this->AddEntry("Point Data", vtkDataObject::FIELD_ASSOCIATION_POINTS);
    }
  if (this->DisableUpdateDomainEntries || has_cd )
    {
    this->AddEntry("Cell Data",  vtkDataObject::FIELD_ASSOCIATION_CELLS);
    }

  if (this->DisableUpdateDomainEntries || has_vd)
    {
    this->AddEntry("Vertex Data", vtkDataObject::FIELD_ASSOCIATION_VERTICES);
    }

  if (this->DisableUpdateDomainEntries || has_ed)
    {
    this->AddEntry("Edge Data", vtkDataObject::FIELD_ASSOCIATION_EDGES);
    }

  if (this->DisableUpdateDomainEntries || has_rd)
    {
    this->AddEntry("Row Data", vtkDataObject::FIELD_ASSOCIATION_ROWS);
    }

  if (this->EnableFieldDataSelection)
    {
    this->AddEntry("Field Data", vtkDataObject::FIELD_ASSOCIATION_NONE);
    }

  this->DefaultValue = -1;
  if (has_pd)
    {
    this->DefaultValue = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }
  else if (has_cd)
    {
    this->DefaultValue = vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
  else if (has_vd)
    {
    this->DefaultValue = vtkDataObject::FIELD_ASSOCIATION_VERTICES;
    }
  else if (has_ed)
    {
    this->DefaultValue = vtkDataObject::FIELD_ASSOCIATION_EDGES;
    }
  else if (has_rd)
    {
    this->DefaultValue = vtkDataObject::FIELD_ASSOCIATION_ROWS;
    }
  else if (this->EnableFieldDataSelection)
    {
    this->DefaultValue = vtkDataObject::FIELD_ASSOCIATION_NONE;
    }


  this->InvokeModified();
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMProxyProperty* pp,
                                  vtkSMSourceProxy* sp,
                                  int outputport)
{
  vtkSmartPointer<vtkSMDomainIterator> di;
  di.TakeReference(pp->NewDomainIterator());
  di->Begin();
  while (!di->IsAtEnd())
    {
    vtkSMInputArrayDomain* iad = vtkSMInputArrayDomain::SafeDownCast(
      di->GetDomain());
    if (iad)
      {
      this->Update(sp, iad, outputport);
      return;
      }
    di->Next();
    }

  // No vtkSMInputArrayDomain present.
  this->Update(sp, NULL, outputport);
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
int vtkSMFieldDataDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp && this->DefaultValue != -1)
    {
    ivp->SetElement(0, this->DefaultValue);
    // update unchecked value for now as well. We really need a mechanism to
    // "clear" the unchecked values when a value is set.
    ivp->SetUncheckedElement(0, this->DefaultValue);
    return 1;
    }

  return this->Superclass::SetDefaultValues(prop);
}


//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
    {
    return 0;
    }

  int enable_field_data=0;
  if (element->GetScalarAttribute("enable_field_data", &enable_field_data))
    {
    this->EnableFieldDataSelection = (enable_field_data!=0)? true : false;
    }
  int disable_update_domain_entries=0;
  if (element->GetScalarAttribute("disable_update_domain_entries",
      &disable_update_domain_entries))
    {
    this->DisableUpdateDomainEntries =
      (disable_update_domain_entries!=0)? true : false;
    }

  int force_point_cell_data =0;
  if (element->GetScalarAttribute("force_point_cell_data",
    &force_point_cell_data))
    {
    this->ForcePointAndCellDataSelection =
      ( force_point_cell_data!=0) ? true : false;
    }


  if (this->DisableUpdateDomainEntries)
    {
    // this is a traditional enumeration. Fill it up with values.
    this->AddEntry("Point Data", vtkDataObject::FIELD_ASSOCIATION_POINTS);
    this->AddEntry("Cell Data",  vtkDataObject::FIELD_ASSOCIATION_CELLS);
    this->AddEntry("Vertex Data", vtkDataObject::FIELD_ASSOCIATION_VERTICES);
    this->AddEntry("Edge Data", vtkDataObject::FIELD_ASSOCIATION_EDGES);
    this->AddEntry("Row Data", vtkDataObject::FIELD_ASSOCIATION_ROWS);
    if (this->EnableFieldDataSelection)
      {
      this->AddEntry("Field Data", vtkDataObject::FIELD_ASSOCIATION_NONE);
      }
    this->DefaultValue = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
