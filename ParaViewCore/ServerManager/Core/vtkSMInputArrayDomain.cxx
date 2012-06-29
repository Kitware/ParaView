/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputArrayDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputArrayDomain.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMInputArrayDomain);

bool vtkSMInputArrayDomain::AutomaticPropertyConversion = false;

//---------------------------------------------------------------------------
static const char* const vtkSMInputArrayDomainAttributeTypes[] = {
  "point",
  "cell",
  "any",
  "vertex",
  "edge",
  "row",
  "none"
};

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::vtkSMInputArrayDomain()
{
  this->AttributeType = vtkSMInputArrayDomain::ANY;
  this->NumberOfComponents = 0;
}

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::~vtkSMInputArrayDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }

  unsigned int i;

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(property);
  if (pp)
    {
    unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
    for (i=0; i<numProxs; i++)
      {
      if (!this->IsInDomain(
            vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i)),
            (ip? ip->GetUncheckedOutputPortForConnection(i):0)) )
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMSourceProxy* proxy,
                                      int outputport/*=0*/)
{
  if (!proxy)
    {
    return 0;
    }

  // Make sure the outputs are created.
  proxy->CreateOutputPorts();
  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info)
    {
    return 0;
    }

  if (this->AttributeType == vtkSMInputArrayDomain::POINT ||
      this->AttributeType == vtkSMInputArrayDomain::ANY ||
      (this->AutomaticPropertyConversion &&
      this->AttributeType == vtkSMInputArrayDomain::CELL))
    {
    if (this->AttributeInfoContainsArray(proxy, outputport,
        info->GetPointDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::CELL||
      this->AttributeType == vtkSMInputArrayDomain::ANY ||
      (this->AutomaticPropertyConversion &&
      this->AttributeType == vtkSMInputArrayDomain::POINT))
    {
    if (this->AttributeInfoContainsArray(proxy, outputport,
        info->GetCellDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::VERTEX||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(proxy, outputport,
        info->GetVertexDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::EDGE||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(proxy, outputport,
        info->GetEdgeDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::ROW||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(proxy, outputport,
        info->GetRowDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::NONE||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(proxy, outputport,
                                         info->GetFieldDataInformation()))
      {
      return 1;
      }
    }


  return 0;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::CheckForArray(
  vtkPVArrayInformation* arrayInfo, vtkPVDataSetAttributesInformation* attrInfo)
{
  if (!attrInfo || !arrayInfo)
    {
    return 0;
    }

  int num = attrInfo->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* curInfo = attrInfo->GetArrayInformation(idx);
    if (curInfo == arrayInfo)
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsFieldValid(
  vtkSMSourceProxy* proxy, int outputport, vtkPVArrayInformation* arrayInfo)
{
  return this->IsFieldValid(proxy, outputport, arrayInfo, 0);
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::GetAttributeTypeFromFieldAssociation(
  int dsaAssociation)
{
  switch (dsaAssociation)
    {
     case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        return vtkSMInputArrayDomain::POINT;
     case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        return vtkSMInputArrayDomain::CELL;
     case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
        // TODO: Handle this case.
        return vtkSMInputArrayDomain::POINT;
     case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
        return vtkSMInputArrayDomain::VERTEX;
     case vtkDataObject::FIELD_ASSOCIATION_EDGES:
        return vtkSMInputArrayDomain::EDGE;
     case vtkDataObject::FIELD_ASSOCIATION_ROWS:
        return vtkSMInputArrayDomain::ROW;
     case vtkDataObject::FIELD_ASSOCIATION_NONE:
        return vtkSMInputArrayDomain::NONE;
      }
  return vtkDataObject::FIELD_ASSOCIATION_POINTS;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsFieldValid(
  vtkSMSourceProxy* proxy, int outputport,
  vtkPVArrayInformation* arrayInfo, int bypass)
{
  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info)
    {
    return 0;
    }

  int attributeType = this->AttributeType;
  if (!bypass)
    {
    // FieldDataSelection typically is a SelectInputScalars kind of property
    // in which case the attribute type is at index 3 or is a simple
    // type choosing property in which case it's a vtkSMIntVectorProperty.
    vtkSMProperty* pfds = this->GetRequiredProperty("FieldDataSelection");
    vtkSMStringVectorProperty* fds = vtkSMStringVectorProperty::SafeDownCast(
      pfds);
    vtkSMIntVectorProperty* ifds = vtkSMIntVectorProperty::SafeDownCast(pfds);
    if (fds || ifds)
      {
      int val = (fds)? atoi(fds->GetUncheckedElement(3)) :
        ifds->GetUncheckedElement(0);
      attributeType =
        vtkSMInputArrayDomain::GetAttributeTypeFromFieldAssociation(val);
      }
    }

  int isField = 0;
  if ( this->AutomaticPropertyConversion &&
      (attributeType == vtkSMInputArrayDomain::POINT ||
       attributeType == vtkSMInputArrayDomain::CELL ||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetPointDataInformation());
    if (!isField)
      {
      isField = this->CheckForArray(arrayInfo, info->GetCellDataInformation());
      }
    }
  if (!isField &&
      (attributeType == vtkSMInputArrayDomain::POINT ||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetPointDataInformation());
    }

   if (!isField &&
     (attributeType == vtkSMInputArrayDomain::CELL ||
      attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetCellDataInformation());
    }

  if (!isField &&
      (attributeType == vtkSMInputArrayDomain::VERTEX||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetVertexDataInformation());
    }

  if (!isField &&
      (attributeType == vtkSMInputArrayDomain::EDGE||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetEdgeDataInformation());
    }

  if (!isField &&
      (attributeType == vtkSMInputArrayDomain::ROW||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetRowDataInformation());
    }

  if (!isField &&
      (attributeType == vtkSMInputArrayDomain::NONE||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetFieldDataInformation());
    }

  if (!isField)
    {
    return 0;
    }

  if (this->AutomaticPropertyConversion)
    {
    // when using automatic property conversion, we support automatic extraction
    // of a single component from multi-component arrays. However, we still
    // don't support automatic extraction of multiple components, so if the
    // filter needs more than 1 component, then the number of components must
    // match.
    if (this->NumberOfComponents > 1 &&
      this->NumberOfComponents != arrayInfo->GetNumberOfComponents())
      {
      return 0;
      }
    }
  else
    {
    if (this->NumberOfComponents > 0 &&
      this->NumberOfComponents != arrayInfo->GetNumberOfComponents())
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::AttributeInfoContainsArray(
  vtkSMSourceProxy* proxy,
  int outputport,
  vtkPVDataSetAttributesInformation* attrInfo)
{
  if (!attrInfo)
    {
    return 0;
    }

  int num = attrInfo->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(idx);
    if (this->IsFieldValid(proxy, outputport, arrayInfo, 1))
      {
      return 1;
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  vtkPVXMLElement* inputArrayElem = vtkPVXMLElement::New();
  inputArrayElem->SetName("InputArray");
  inputArrayElem->AddAttribute("attribute_type",
                               this->GetAttributeTypeAsString());
  inputArrayElem->AddAttribute("number_of_components",
                               this->GetNumberOfComponents());
  domainElement->AddNestedElement(inputArrayElem);
  inputArrayElem->Delete();

}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* attribute_type = element->GetAttribute("attribute_type");
  if (attribute_type)
    {
    this->SetAttributeType(attribute_type);
    }

  int numComponents;
  if (element->GetScalarAttribute("number_of_components", &numComponents))
    {
    this->SetNumberOfComponents(numComponents);
    }

  return 1;
}

//---------------------------------------------------------------------------
const char* vtkSMInputArrayDomain::GetAttributeTypeAsString()
{
  if (this->AttributeType < vtkSMInputArrayDomain::LAST_ATTRIBUTE_TYPE)
    {
    return vtkSMInputArrayDomainAttributeTypes[this->AttributeType];
    }

  return "(invalid)";
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SetAttributeType(const char* type)
{
  if ( ! type )
    {
    vtkErrorMacro("No type specified");
    return;
    }
  unsigned char cc;
  for ( cc = 0; cc < vtkSMInputArrayDomain::LAST_ATTRIBUTE_TYPE; cc ++ )
    {
    if ( strcmp(type, vtkSMInputArrayDomainAttributeTypes[cc]) == 0 )
      {
      this->SetAttributeType(cc);
      return;
      }
    }
  vtkErrorMacro("Unrecognized attribute type: " << type);
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SetAutomaticPropertyConversion(bool convert)
{
  if ( vtkSMInputArrayDomain::AutomaticPropertyConversion != convert )
    {
    vtkSMInputArrayDomain::AutomaticPropertyConversion = convert;
    }
}

//---------------------------------------------------------------------------
bool vtkSMInputArrayDomain::GetAutomaticPropertyConversion()
{
  return vtkSMInputArrayDomain::AutomaticPropertyConversion;
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "AttributeType: " << this->AttributeType
    << " (" << this->GetAttributeTypeAsString() << ")" << endl;
}
