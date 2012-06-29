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
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <map>
#include <vector>
#include <string>
#include "vtksys/ios/sstream"
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMArrayListDomain);

struct vtkSMArrayListDomainInformationKey
{
  vtkStdString Location;
  vtkStdString Name;
  int Strategy;
};

struct vtkSMArrayListDomainInternals
{
  std::map<vtkStdString, int> PartialMap;
  std::vector<int> DataTypes;
  std::vector<int> FieldAssociation;
  std::map<int,int> DomainAssociation;
  std::vector<vtkSMArrayListDomainInformationKey> InformationKeys;

  void SetAssociations(int index, int field, int domain)
    {
    this->FieldAssociation[index] = field;
    this->DomainAssociation[index] =  domain == -1 ? field : domain;
    }
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
int vtkSMArrayListDomain::GetDomainAssociation(unsigned int idx )
{
if ( this->ALDInternals->DomainAssociation.find(idx) ==
  this->ALDInternals->DomainAssociation.end() )
  {
  return this->GetFieldAssociation(idx);
  }
  return this->ALDInternals->DomainAssociation.find(idx)->second;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::GetFieldAssociation(unsigned int idx)
{
  if (this->ALDInternals->FieldAssociation.size() > idx)
    {
    return this->ALDInternals->FieldAssociation[idx];
    }

  return -1;
}
//---------------------------------------------------------------------------
unsigned int vtkSMArrayListDomain::AddString(const char* string)
{
  // by default we don't assume any association.
  this->ALDInternals->FieldAssociation.push_back(
    vtkDataObject::NUMBER_OF_ASSOCIATIONS);

  return this->Superclass::AddString(string);
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::RemoveAllStrings()
{
  this->ALDInternals->FieldAssociation.clear();
  this->ALDInternals->PartialMap.clear();
  this->Superclass::RemoveAllStrings();
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::RemoveString(const char* string)
{
  int index = this->Superclass::RemoveString(string);
  if (index != -1)
    {
    int cc=0;
    std::vector<int>::iterator iter;
    for (iter=this->ALDInternals->FieldAssociation.begin();
      iter != this->ALDInternals->FieldAssociation.end();
      iter++, cc++)
      {
      if (cc==index)
        {
        this->ALDInternals->FieldAssociation.erase(iter);
        break;
        }
      }
    }
  return index;
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::AddArrays(vtkSMSourceProxy* sp,
                                     int outputport,
                                     vtkPVDataSetAttributesInformation* info,
                                     vtkSMInputArrayDomain* iad,
                                     int association, int domainAssociation)
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
      int nAcceptedTypes=static_cast<int>(this->ALDInternals->DataTypes.size());
      if ( nAcceptedTypes==0 )
        {
        if(this->CheckInformationKeys(arrayInfo))
        {
          unsigned int newidx = this->AddArray(arrayInfo, association,
            domainAssociation, iad);
          if (arrayInfo == attrInfo)
            {
            attrIdx = newidx;
            }
          }
        }
      for (int i=0; i<nAcceptedTypes; ++i)
        {
        int thisDataType=this->ALDInternals->DataTypes[i];
        if (!thisDataType || (arrayInfo->GetDataType() == thisDataType))
          {
          if(this->CheckInformationKeys(arrayInfo))
            {
            unsigned int newidx = this->AddArray(arrayInfo, association,
              domainAssociation, iad);
            if (arrayInfo == attrInfo)
              {
              attrIdx = newidx;
              }
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
unsigned int vtkSMArrayListDomain::AddArray(
  vtkPVArrayInformation* arrayInfo, int association,int domainAssociation,
  vtkSMInputArrayDomain* iad)
{
  if (iad->GetAutomaticPropertyConversion() &&
    iad->GetNumberOfComponents() == 1 &&
    arrayInfo->GetNumberOfComponents() > 1)
    {
    // add magnitude only for numeric arrays.
    unsigned int first_index = VTK_UNSIGNED_INT_MAX;
    if (arrayInfo->GetDataType() != VTK_STRING)
      {
      vtkStdString name = this->CreateMangledName(arrayInfo,
        arrayInfo->GetNumberOfComponents());
      first_index = this->AddString(name.c_str());
      this->ALDInternals->SetAssociations(first_index,association,domainAssociation);
      }
    for (int cc=0; cc < arrayInfo->GetNumberOfComponents(); cc++)
      {
      vtkStdString name = this->CreateMangledName(arrayInfo,cc);
      unsigned int newidx = this->AddString(name.c_str());
      if (first_index == VTK_UNSIGNED_INT_MAX)
        {
        first_index = newidx;
        }
      this->ALDInternals->SetAssociations(newidx,association,domainAssociation);
      }
    return first_index;
    }
  else
    {
    unsigned int newidx = this->AddString(arrayInfo->GetName());
    this->ALDInternals->SetAssociations(newidx,association,domainAssociation);
    return  newidx;
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

  int attribute_type = iad->GetAttributeType();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetRequiredProperty("FieldDataSelection"));
  if (ivp && ivp->GetNumberOfElements() == 1)
    {
    if (ivp->GetNumberOfUncheckedElements() == 1)
      {
      attribute_type =
        vtkSMInputArrayDomain::GetAttributeTypeFromFieldAssociation(
          ivp->GetUncheckedElement(0));
      }
    else
      {
      attribute_type =
        vtkSMInputArrayDomain::GetAttributeTypeFromFieldAssociation(
          ivp->GetElement(0));
      }
    }

  switch (attribute_type)
    {
  case vtkSMInputArrayDomain::ANY:
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
    break;

  case vtkSMInputArrayDomain::POINT:
    this->AddArrays(sp, outputport, info->GetPointDataInformation(), iad,
      vtkDataObject:: FIELD_ASSOCIATION_POINTS);
    if(vtkSMInputArrayDomain::GetAutomaticPropertyConversion())
     {
     this->AddArrays(sp, outputport, info->GetCellDataInformation(), iad,
       vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataObject::FIELD_ASSOCIATION_POINTS);
     }
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_POINTS;
    break;

  case vtkSMInputArrayDomain::CELL:
    this->AddArrays(sp, outputport, info->GetCellDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_CELLS);
    if(vtkSMInputArrayDomain::GetAutomaticPropertyConversion())
     {
     this->AddArrays(sp, outputport, info->GetPointDataInformation(), iad,
       vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataObject::FIELD_ASSOCIATION_CELLS);
     }
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_CELLS;
    break;

  case vtkSMInputArrayDomain::VERTEX:
    this->AddArrays(sp, outputport, info->GetVertexDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_VERTICES);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_VERTICES;
    break;

  case vtkSMInputArrayDomain::EDGE:
    this->AddArrays(sp, outputport, info->GetEdgeDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_EDGES);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_EDGES;
    break;

  case vtkSMInputArrayDomain::ROW:
    this->AddArrays(sp, outputport, info->GetRowDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_ROWS);
    this->Association = vtkDataObject:: FIELD_ASSOCIATION_ROWS;
    break;

  case  vtkSMInputArrayDomain::NONE:
    this->AddArrays(sp, outputport, info->GetFieldDataInformation(), iad,
      vtkDataObject::FIELD_ASSOCIATION_NONE);
    this->Association = vtkDataObject::FIELD_ASSOCIATION_NONE;
    break;
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
    this->ALDInternals->FieldAssociation[0] =
      vtkDataObject::NUMBER_OF_ASSOCIATIONS;
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

  const char* key_locations = element->GetAttribute("key_locations");
  const char* key_names = element->GetAttribute("key_names");
  const char* key_strategies = element->GetAttribute("key_strategies");
  if(key_locations == NULL)
    {
    // Default value : add a needed information key vtkAbstractArray::GUI_HIDE
    this->AddInformationKey("vtkAbstractArray", "GUI_HIDE", vtkSMArrayListDomain::REJECT_KEY);
    }
  else
    {
    std::stringstream locations(key_locations);
    std::stringstream names(key_names);
    std::stringstream strategies(key_strategies);
    std::string location, name, strategy, last_strategy;
    int strat;
    // default value for the strategy
    last_strategy = "need_key";
    while(locations >> location)
      {
      if(! (names >> name) )
        {
        vtkErrorMacro("The number of key names must be equal to the number of key locations");
        break;
        }
      if(! (strategies >> strategy))
        {
        strategy = last_strategy;
        }
      last_strategy = strategy;

      if(strategy == "need_key" || strategy == "NEED_KEY")
        {
        strat = vtkSMArrayListDomain::NEED_KEY;
        }
      else if(strategy == "reject_key" || strategy == "REJECT_KEY")
        {
        strat = vtkSMArrayListDomain::REJECT_KEY;
        }
      else
        {
        // maybe the strategy was coded as an integer.
        strat=atoi(strategy.c_str());
        }
      this->AddInformationKey(location.c_str(), name.c_str(), strat);
      }
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
      std::string thisType;
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
      else if ( thisType=="VTK_DATA_ARRAY" )
        {
          this->ALDInternals->DataTypes.push_back(VTK_BIT);
          this->ALDInternals->DataTypes.push_back(VTK_CHAR);
          this->ALDInternals->DataTypes.push_back(VTK_SIGNED_CHAR);
          this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_CHAR);
          this->ALDInternals->DataTypes.push_back(VTK_SHORT);
          this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_SHORT);
          this->ALDInternals->DataTypes.push_back(VTK_INT);
          this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_INT);
          this->ALDInternals->DataTypes.push_back(VTK_LONG);
          this->ALDInternals->DataTypes.push_back(VTK_FLOAT);
          this->ALDInternals->DataTypes.push_back(VTK_DOUBLE);
          this->ALDInternals->DataTypes.push_back(VTK_ID_TYPE);
          this->ALDInternals->DataTypes.push_back(VTK_LONG_LONG);
          this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_LONG);
          this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_LONG_LONG);
          this->ALDInternals->DataTypes.push_back(VTK___INT64);
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
int vtkSMArrayListDomain::CheckInformationKeys(vtkPVArrayInformation* arrayInfo)
{
  for (unsigned int i=0; i< this->GetNumberOfInformationKeys(); i++)
    {
    vtkSMArrayListDomainInformationKey& key = this->ALDInternals->InformationKeys[i];
    int hasInfo = arrayInfo->HasInformationKey(key.Location, key.Name);
    if(hasInfo && key.Strategy == vtkSMArrayListDomain::REJECT_KEY)
      {
      return 0;
      }
    if(!hasInfo && key.Strategy == vtkSMArrayListDomain::NEED_KEY)
      {
      return 0;
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
unsigned int vtkSMArrayListDomain::AddInformationKey(const char* location, const char *name, int strategy)
{
  vtkSMArrayListDomainInformationKey key;
  key.Location = location;
  key.Name = name;
  key.Strategy = strategy;
  this->ALDInternals->InformationKeys.push_back(key);
  return this->ALDInternals->InformationKeys.size() - 1;
}

//---------------------------------------------------------------------------
unsigned int vtkSMArrayListDomain::RemoveInformationKey(const char* location, const char *name)
{
  std::vector<vtkSMArrayListDomainInformationKey>::iterator it
    = this->ALDInternals->InformationKeys.begin();
  int index = 0;
  while(it != this->ALDInternals->InformationKeys.end())
    {
    vtkSMArrayListDomainInformationKey& key = *it;
    if(key.Location == location && key.Name == name)
      {
      this->ALDInternals->InformationKeys.erase(it);
      return index;
      }
    it++;
    index++;
    }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMArrayListDomain::GetNumberOfInformationKeys()
{
  return static_cast<unsigned int>(this->ALDInternals->InformationKeys.size());
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::RemoveAllInformationKeys()
{
  this->ALDInternals->InformationKeys.clear();
}

//---------------------------------------------------------------------------
const char* vtkSMArrayListDomain::GetInformationKeyLocation(unsigned int index)
{
  if(index < this->ALDInternals->InformationKeys.size())
    {
    return this->ALDInternals->InformationKeys[index].Location;
    }
  return NULL;
}

//---------------------------------------------------------------------------
const char* vtkSMArrayListDomain::GetInformationKeyName(unsigned int index)
{
  if(index < this->ALDInternals->InformationKeys.size())
    {
    return this->ALDInternals->InformationKeys[index].Name;
    }
  return NULL;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::GetInformationKeyStrategy(unsigned int index)
{
  if(index < this->ALDInternals->InformationKeys.size())
    {
    return this->ALDInternals->InformationKeys[index].Strategy;
    }
  return -1;
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultElement: " << this->DefaultElement << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
  int nDataTypes=static_cast<int>(this->ALDInternals->DataTypes.size());
  for (int i=0; i<nDataTypes; ++i)
    {
    os << indent << "DataType: " << this->ALDInternals->DataTypes[i] << endl;
    }

  for(unsigned int i=0; i<this->ALDInternals->InformationKeys.size(); i++)
    {
    vtkSMArrayListDomainInformationKey& key = this->ALDInternals->InformationKeys[i];
    os << key.Location << "::" << key.Name << " ";
    if(key.Strategy == vtkSMArrayListDomain::NEED_KEY)
      {
      os << "NEED_KEY";
      }
    else if(key.Strategy == vtkSMArrayListDomain::REJECT_KEY)
      {
      os << "REJECT_KEY";
      }
    else
      {
      os << "UNKNOWN KEY STRATEGY : " << key.Strategy;
      }
    os << endl;
    }
}


//---------------------------------------------------------------------------
vtkStdString vtkSMArrayListDomain::CreateMangledName(
  vtkPVArrayInformation *arrayInfo, int component)
{
  vtksys_ios::ostringstream stream;
  if ( component != arrayInfo->GetNumberOfComponents() )
    {
    stream << arrayInfo->GetName() << "_" <<
      arrayInfo->GetComponentName(component);
    }
  else
    {
    stream << arrayInfo->GetName() << "_Magnitude";
    }
  return stream.str();
}

//---------------------------------------------------------------------------
vtkStdString vtkSMArrayListDomain::ArrayNameFromMangledName(
  const char* name)
{
  vtkStdString extractedName = name;
  size_t pos = extractedName.rfind("_");
  if (pos == vtkStdString::npos)
    {
    return extractedName;
    }
  return extractedName.substr(0,pos);
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::ComponentIndexFromMangledName(
  vtkPVArrayInformation *info, const char* name)
{
  vtkStdString extractedName = name;
  size_t pos = extractedName.rfind("_");
  if (pos == vtkStdString::npos)
    {
    return -1;
    }
  vtkStdString compName = extractedName.substr(pos+1,extractedName.length()-pos);
  int numComps = info->GetNumberOfComponents();
  if ( compName == "Magnitude" )
    {
    return numComps;
    }
  for ( int i=0; i < numComps; ++i)
    {
    if ( compName == info->GetComponentName(i) )
      {
      return i;
      }
    }
  return -1;
}
