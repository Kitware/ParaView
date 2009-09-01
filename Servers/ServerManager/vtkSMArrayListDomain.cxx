/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArrayListDomain.h"

#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
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

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>
#include "vtksys/ios/sstream"
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMArrayListDomain);
vtkCxxRevisionMacro(vtkSMArrayListDomain, "1.21.2.1");

struct vtkSMArrayListDomainInternals
{
  vtkstd::map<vtkStdString, int> PartialMap;
  vtkstd::vector<int> DataTypes;
};

//---------------------------------------------------------------------------
vtkSMArrayListDomain::vtkSMArrayListDomain()
{
  this->AttributeType = vtkDataSetAttributes::SCALARS;
  this->DefaultElement = 0;
  this->Association = 0;
  this->InputDomainName = 0;
  this->NoneString = 0;
  this->ALDInternals = new vtkSMArrayListDomainInternals;
}

//---------------------------------------------------------------------------
vtkSMArrayListDomain::~vtkSMArrayListDomain()
{
  this->SetInputDomainName(0);
  this->SetNoneString(0);
  delete this->ALDInternals;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::IsArrayPartial(unsigned int idx)
{
  const char* name = this->GetString(idx);
  return this->ALDInternals->PartialMap[name];
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::AddArrays(vtkSMSourceProxy* sp,
                                     int outputport,
                                     vtkPVDataSetAttributesInformation* info, 
                                     vtkSMInputArrayDomain* iad,
                                     int association)
{
  this->DefaultElement = 0;

  int attrIdx=-1;
  vtkPVArrayInformation* attrInfo = info->GetAttributeInformation(
    this->AttributeType);
  int num = info->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* arrayInfo = info->GetArrayInformation(idx);
    if ( iad->IsFieldValid(sp, outputport, info->GetArrayInformation(idx), 1) )
      {
      this->ALDInternals->PartialMap[arrayInfo->GetName()] = arrayInfo->GetIsPartial();
      int nAcceptedTypes=this->ALDInternals->DataTypes.size();
      if ( nAcceptedTypes==0 )
        {
        unsigned int newidx = this->AddString(arrayInfo->GetName());
        if (arrayInfo == attrInfo)
          {
          attrIdx = newidx;
          }
        }
      for (int i=0; i<nAcceptedTypes; ++i)
        {
        int thisDataType=this->ALDInternals->DataTypes[i];
        if (!thisDataType || (arrayInfo->GetDataType() == thisDataType))
          {
          unsigned int newidx = this->AddString(arrayInfo->GetName());
          if (arrayInfo == attrInfo)
            {
            attrIdx = newidx;
            }
          }
        }
      }
    }
  if (attrIdx >= 0)
    {
    this->SetDefaultElement(attrIdx);
    this->Association = association;
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMSourceProxy* sp, 
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

  if ( iad->GetAttributeType() == vtkSMInputArrayDomain::ANY )
    {
    this->AddArrays(sp, outputport, info->GetPointDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_POINTS);
    this->AddArrays(sp, outputport, info->GetCellDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_CELLS);
    this->AddArrays(sp, outputport, info->GetVertexDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_VERTICES);
    this->AddArrays(sp, outputport, info->GetEdgeDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_EDGES);
    this->AddArrays(sp, outputport, info->GetRowDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_ROWS);
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::POINT )
    {
    this->AddArrays(sp, outputport, info->GetPointDataInformation(), iad,
      vtkDataObject:: FIELD_ASSOCIATION_POINTS);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_POINTS;
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::CELL )
    {
    this->AddArrays(sp, outputport, info->GetCellDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_CELLS);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_CELLS;
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::VERTEX)
    {
    this->AddArrays(sp, outputport, info->GetVertexDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_VERTICES);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_VERTICES;
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::EDGE)
    {
    this->AddArrays(sp, outputport, info->GetEdgeDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_EDGES);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_EDGES;    
    }
  else if ( iad->GetAttributeType() == vtkSMInputArrayDomain::ROW)
    {
    this->AddArrays(sp, outputport, info->GetRowDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_ROWS);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_ROWS;
    }

  this->InvokeModified();
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp,
                                  vtkSMSourceProxy* sp,
                                  int outputport)
{
  vtkSMInputArrayDomain* iad = 0;
  if (this->InputDomainName)
    {
    iad = vtkSMInputArrayDomain::SafeDownCast(
      pp->GetDomain(this->InputDomainName));
    }
  else
    {
    vtkSMDomainIterator* di = pp->NewDomainIterator();
    di->Begin();
    while (!di->IsAtEnd())
      {
      iad = vtkSMInputArrayDomain::SafeDownCast(di->GetDomain());
      if (iad)
        {
        break;
        }
      di->Next();
      }
    di->Delete();
    }

  if (iad)
    {
    this->Update(sp, iad, outputport);
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProxyProperty* pp)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);

  unsigned int i;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      this->Update(pp, sp, 
        (ip? ip->GetUncheckedOutputPortForConnection(i) : 0));
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
        (ip? ip->GetOutputPortForConnection(i) : 0));
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProperty*)
{
  this->RemoveAllStrings();
  if (this->NoneString)
    {
    this->AddString(this->NoneString);
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    }
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* input_domain_name = 
    element->GetAttribute("input_domain_name");
  if (input_domain_name)
    {
    this->SetInputDomainName(input_domain_name);
    }

  // Search for attribute type with matching name.
  const char* none_string = element->GetAttribute("none_string");
  if (none_string)
    {
      this->SetNoneString(none_string);
    }

  // Search for attribute type with matching name.
  const char* attribute_type = element->GetAttribute("attribute_type");
  unsigned int i = vtkDataSetAttributes::NUM_ATTRIBUTES;
  if(attribute_type)
    {
    for(i=0; i<vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
      {
      if(strcmp(vtkDataSetAttributes::GetAttributeTypeAsString(i),
                attribute_type) == 0)
        {
        this->SetAttributeType(i);
        break;
        }
      }
    }

  if(i == vtkDataSetAttributes::NUM_ATTRIBUTES)
    {
    this->SetAttributeType(vtkDataSetAttributes::SCALARS);
    }

  const char* data_type = element->GetAttribute("data_type");

  if(data_type)
    {
    // data_type can be a space delimited list of types
    // vlaid for the domain
    vtksys_ios::istringstream dataTypeStream(data_type);

    while (dataTypeStream.good())
      {
      vtkstd::string thisType;
      dataTypeStream >> thisType;

      if ( thisType=="VTK_VOID" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_VOID);
        }
      else if ( thisType=="VTK_BIT" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_BIT);
        }
      else if ( thisType=="VTK_CHAR" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_CHAR);
        }
      else if ( thisType=="VTK_SIGNED_CHAR" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_SIGNED_CHAR);
        }
      else if ( thisType=="VTK_UNSIGNED_CHAR" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_CHAR);
        }
      else if ( thisType=="VTK_SHORT" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_SHORT);
        }
      else if ( thisType=="VTK_UNSIGNED_SHORT" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_SHORT);
        }
      else if ( thisType=="VTK_INT" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_INT);
        }
      else if ( thisType=="VTK_UNSIGNED_INT" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_INT);
        }
      else if ( thisType=="VTK_LONG" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_LONG);
        }
      else if ( thisType=="VTK_FLOAT" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_FLOAT);
        }
      else if ( thisType=="VTK_DOUBLE" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_DOUBLE);
        }
      else if ( thisType=="VTK_ID_TYPE" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_ID_TYPE);
        }
      else if ( thisType=="VTK_STRING" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_STRING);
        }
      else if ( thisType=="VTK_OPAQUE" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_OPAQUE);
        }
      else if ( thisType=="VTK_LONG_LONG" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_LONG_LONG);
        }
      else if ( thisType=="VTK_UNSIGNED_LONG_LONG" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_LONG_LONG);
        }
      else if ( thisType=="VTK___INT64" )
        {
        this->ALDInternals->DataTypes.push_back(VTK___INT64);
        }
      else if ( thisType=="VTK_UNSIGNED___INT64" )
        {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED___INT64);
        }
      else
        {
        int maybeType=atoi(thisType.c_str()); //?
        this->ALDInternals->DataTypes.push_back(maybeType);
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    return 0;
    }

  const char* array = 0;
  if (this->GetNumberOfStrings() > 0)
    {
    array = this->GetString(this->DefaultElement);
    const char* defaultValue = svp->GetDefaultValue(0);
    unsigned int temp;
    if (defaultValue && this->IsInDomain(defaultValue, temp))
      {
      array = defaultValue;
      }
    }

    if (svp->GetNumberOfElements() == 5)
      {
      vtksys_ios::ostringstream ass;
      ass << this->Association;
      svp->SetElement(3, ass.str().c_str());
      if (array)
        {
        svp->SetElement(4, array);
        return 1;
        }
      }
    else if (svp->GetNumberOfElements() == 1 && array)
      {
      svp->SetElement(0, array);
      return 1;
      }

  return this->Superclass::SetDefaultValues(prop);
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultElement: " << this->DefaultElement << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
  int nDataTypes=this->ALDInternals->DataTypes.size();
  for (int i=0; i<nDataTypes; ++i)
    {
    os << indent << "DataType: " << this->ALDInternals->DataTypes[i] << endl;
    }
}
