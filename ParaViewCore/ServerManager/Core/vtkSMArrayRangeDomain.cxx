/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArrayRangeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMArrayRangeDomain);

//---------------------------------------------------------------------------
vtkSMArrayRangeDomain::vtkSMArrayRangeDomain()
{
}

//---------------------------------------------------------------------------
vtkSMArrayRangeDomain::~vtkSMArrayRangeDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(vtkSMProperty*)
{
  this->RemoveAllMinima();
  this->RemoveAllMaxima();

  vtkSMProxyProperty* ip = 0;
  vtkSMStringVectorProperty* array = 0;

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    ip = pp;
    }

  // Get the array name for the array selection
  vtkSMStringVectorProperty* sp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetRequiredProperty("ArraySelection"));
  if (sp)
    {
    array = sp;
    }

  if (!ip || !array)
    {
    return;
    }

  if (array->GetNumberOfUncheckedElements() < 5)
    {
    return;
    }
  const char* arrayName = array->GetUncheckedElement(4);
  if (!arrayName || arrayName[0] == '\0')
    {
    if (array->GetNumberOfElements() < 5)
      {
      return;
      }
    arrayName = array->GetElement(4);
    }

  if (!arrayName || arrayName[0] == '\0')
    {
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
      this->Update(arrayName, ip, source,
        (inputProp? inputProp->GetUncheckedOutputPortForConnection(i): 0));
      this->InvokeModified();
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
      this->Update(arrayName, ip, source,
        (inputProp? inputProp->GetOutputPortForConnection(i): 0));
      this->InvokeModified();
      return;
      }
    }


}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(const char* arrayName,
                                   vtkSMProxyProperty* ip,
                                   vtkSMSourceProxy* sp,
                                   int outputport)
{
  vtkSMDomainIterator* di = ip->NewDomainIterator();
  di->Begin();
  while (!di->IsAtEnd())
    {
    // We have to figure out whether we are working with cell data,
    // point data or both.
    vtkSMInputArrayDomain* iad = vtkSMInputArrayDomain::SafeDownCast(
      di->GetDomain());
    if (iad)
      {
      this->Update(arrayName, sp, iad, outputport);
      break;
      }
    di->Next();
    }
  di->Delete();
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::Update(const char* arrayName,
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

  bool valid = true;
  if ( iad->GetAttributeType() == vtkSMInputArrayDomain::ANY )
    {
    this->SetArrayRange(info->GetPointDataInformation(), arrayName);
    this->SetArrayRange(info->GetCellDataInformation(), arrayName);
    this->SetArrayRange(info->GetVertexDataInformation(), arrayName);
    this->SetArrayRange(info->GetEdgeDataInformation(), arrayName);
    this->SetArrayRange(info->GetRowDataInformation(), arrayName);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::POINT )
    {
    valid = this->SetArrayRange(info->GetPointDataInformation(), arrayName);
    if (!valid)
      {
      this->SetArrayRangeForAutoConvertProperty(
        info->GetCellDataInformation(),arrayName);
      }
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::CELL )
    {
    valid = this->SetArrayRange(info->GetCellDataInformation(), arrayName);
    if (!valid)
      {
      this->SetArrayRangeForAutoConvertProperty(
        info->GetPointDataInformation(),arrayName);
      }
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::VERTEX)
    {
    this->SetArrayRange(info->GetVertexDataInformation(), arrayName);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::EDGE )
    {
    this->SetArrayRange(info->GetEdgeDataInformation(), arrayName);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::ROW)
    {
    this->SetArrayRange(info->GetRowDataInformation(), arrayName);
    }
}

//---------------------------------------------------------------------------
bool vtkSMArrayRangeDomain::SetArrayRange(
  vtkPVDataSetAttributesInformation* info, const char* arrayName)
{
  vtkPVArrayInformation* ai = info->GetArrayInformation(arrayName);
  if (!ai)
    {    
    return false;
    }

  int numArrComp = ai->GetNumberOfComponents();

  // This creates numMinMax entries but leaves them unset (no min or max)
  this->SetNumberOfEntries(numArrComp);

  for (int i=0; i<numArrComp; i++)
    {
    this->AddMinimum(i, ai->GetComponentRange(i)[0]);
    this->AddMaximum(i, ai->GetComponentRange(i)[1]);
    }
  if (numArrComp > 1) // vector magnitude range
    {
    this->AddMinimum(numArrComp, ai->GetComponentRange(-1)[0]);
    this->AddMaximum(numArrComp, ai->GetComponentRange(-1)[1]);
    }
  return true;
}

//---------------------------------------------------------------------------
bool vtkSMArrayRangeDomain::SetArrayRangeForAutoConvertProperty(
  vtkPVDataSetAttributesInformation* info, const char* arrayName)
{
  vtkStdString name =
      vtkSMArrayListDomain::ArrayNameFromMangledName(arrayName);
  if ( name.length() == 0 )
    {
    //failed to extract the name from the mangled name
    return false;
    }
  if ( name == vtkStdString(arrayName) )
    {
    return this->SetArrayRange(info,arrayName);
    }

  vtkPVArrayInformation* ai = info->GetArrayInformation(name.c_str());
  if (!ai)
    {
    return false;
    }
  int numArrComp = ai->GetNumberOfComponents();
  int comp =
    vtkSMArrayListDomain::ComponentIndexFromMangledName(ai,arrayName);
  if ( comp == -1 )
    {
    return false;
    }

  // This creates numMinMax entries but leaves them unset (no min or max)
  this->SetNumberOfEntries(1);

  if ( comp != numArrComp)
    {
    this->AddMinimum(0, ai->GetComponentRange(comp)[0]);
    this->AddMaximum(0, ai->GetComponentRange(comp)[1]);
    }
  else
    {
    // vector magnitude range
    this->AddMinimum(0, ai->GetComponentRange(-1)[0]);
    this->AddMaximum(0, ai->GetComponentRange(-1)[1]);
    }

  return true;
}

//---------------------------------------------------------------------------
int vtkSMArrayRangeDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMDoubleVectorProperty* dvp =
    vtkSMDoubleVectorProperty::SafeDownCast(prop);
  if (!dvp)
    {
    vtkErrorMacro(
      "vtkSMArrayRangeDomain only works with vtkSMDoubleVectorProperty.");
    return 0;
    }
  if (this->GetMinimumExists(0) && this->GetMaximumExists(0))
    {
    if (dvp->GetRepeatCommand())
      {
      // This is the case when this domain is added to properties
      // like "ContourValues".
      dvp->SetNumberOfElements(1);
      double value = (this->GetMinimum(0)+this->GetMaximum(0))/2.0;
      dvp->SetElement(0, value);
      return 1;
      }
    else if(dvp->GetNumberOfElements() == 2)
      {
      // min,max case
      dvp->SetElements2(this->GetMinimum(0), this->GetMaximum(0));
      return 1;
      }
    }

  return this->Superclass::SetDefaultValues(prop);
}

//---------------------------------------------------------------------------
void vtkSMArrayRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
