/*=========================================================================

  Program:   ParaView
  Module:    vtkCPInputDataDescription.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPInputDataDescription.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

class vtkCPInputDataDescription::vtkInternals
{
public:
  typedef std::vector<std::string> FieldType;
  std::map<int, FieldType> Fields;
};

vtkStandardNewMacro(vtkCPInputDataDescription);
vtkCxxSetObjectMacro(vtkCPInputDataDescription, Grid, vtkDataObject);
//----------------------------------------------------------------------------
vtkCPInputDataDescription::vtkCPInputDataDescription()
{
  this->Grid = NULL;
  this->GenerateMesh = false;
  this->AllFields = false;
  this->Internals = new vtkCPInputDataDescription::vtkInternals();
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
vtkCPInputDataDescription::~vtkCPInputDataDescription()
{
  this->SetGrid(nullptr);
  if (this->Internals)
  {
    delete this->Internals;
    this->Internals = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::Reset()
{
  this->Internals->Fields.clear();
  this->AllFields = false;
  this->GenerateMesh = false;
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::AddField(const char* fieldName, int type)
{
  if (std::find(this->Internals->Fields[type].begin(), this->Internals->Fields[type].end(),
        fieldName) == this->Internals->Fields[type].end())
  {
    this->Internals->Fields[type].push_back(fieldName);
  }
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkCPInputDataDescription::AddPointField(const char* fieldName)
{
  VTK_LEGACY_BODY(vtkCPInputDataDescription::AddPointField, "ParaView 5.6");
  this->AddField(fieldName, vtkDataObject::POINT);
}
#endif

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkCPInputDataDescription::AddCellField(const char* fieldName)
{
  VTK_LEGACY_BODY(vtkCPInputDataDescription::AddCellField, "ParaView 5.6");
  this->AddField(fieldName, vtkDataObject::CELL);
}
#endif

//----------------------------------------------------------------------------
unsigned int vtkCPInputDataDescription::GetNumberOfFields()
{
  unsigned int count = 0;
  for (auto iter : this->Internals->Fields)
  {
    count += static_cast<unsigned int>(iter.second.size());
  }
  if (count > 2)
  {
    count = 0;
    for (auto iter : this->Internals->Fields)
    {
      count += static_cast<unsigned int>(iter.second.size());
    }
  }
  return count;
}

//----------------------------------------------------------------------------
const char* vtkCPInputDataDescription::GetFieldName(unsigned int fieldIndex)
{
  if (fieldIndex >= this->GetNumberOfFields())
  {
    vtkWarningMacro("Bad FieldIndex " << fieldIndex);
    return nullptr;
  }
  size_t count = 0;
  for (auto iter : this->Internals->Fields)
  {
    size_t size = iter.second.size();
    if (size + count > static_cast<size_t>(fieldIndex))
    {
      return iter.second[fieldIndex - count].c_str();
    }
    count += size;
  }
  vtkWarningMacro("Bad FieldIndex " << fieldIndex);
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkCPInputDataDescription::GetFieldType(unsigned int fieldIndex)
{
  if (fieldIndex >= this->GetNumberOfFields())
  {
    vtkWarningMacro("Bad FieldIndex " << fieldIndex);
    return -1;
  }
  size_t count = 0;
  for (auto iter : this->Internals->Fields)
  {
    size_t size = iter.second.size();
    if (size + count > static_cast<size_t>(fieldIndex))
    {
      return iter.first;
    }
    count += size;
  }
  vtkWarningMacro("Bad FieldIndex " << fieldIndex);
  return -1;
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsFieldNeeded(const char* fieldName)
{
  VTK_LEGACY_BODY(vtkCPInputDataDescription::IsFieldNeeded, "ParaView 5.6");

  return this->IsFieldNeeded(fieldName, 0) || this - IsFieldNeeded(fieldName, 1);
}
#endif

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsFieldNeeded(const char* fieldName, int type)
{
  if (!fieldName)
  {
    return false;
  }

  if (this->AllFields)
  {
    return true;
  }

  return std::find(this->Internals->Fields[type].begin(), this->Internals->Fields[type].end(),
           fieldName) != this->Internals->Fields[type].end();
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsFieldPointData(const char* fieldName)
{
  VTK_LEGACY_BODY(vtkCPInputDataDescription::IsFieldPointData, "ParaView 5.6");
  return std::find(this->Internals->Fields[0].begin(), this->Internals->Fields[0].end(),
           fieldName) != this->Internals->Fields[0].end();
}
#endif

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::GetIfGridIsNecessary()
{
  return (this->AllFields || this->GetNumberOfFields() > 0 || this->GenerateMesh);
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::ShallowCopy(vtkCPInputDataDescription* idd)
{
  if (idd == nullptr || this == idd)
  {
    return;
  }
  this->AllFields = idd->AllFields;
  this->GenerateMesh = idd->GenerateMesh;
  this->SetGrid(idd->Grid);
  memcpy(this->WholeExtent, idd->WholeExtent, 6 * sizeof(int));
  this->Internals->Fields = idd->Internals->Fields;
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AllFields: " << this->AllFields << "\n";
  os << indent << "GenerateMesh: " << this->GenerateMesh << "\n";
  if (this->Grid)
  {
    os << indent << "Grid: " << this->Grid << "\n";
  }
  else
  {
    os << indent << "Grid: (NULL)\n";
  }
  os << indent << "WholeExtent: " << this->WholeExtent[0] << " " << this->WholeExtent[1] << " "
     << this->WholeExtent[2] << " " << this->WholeExtent[3] << " " << this->WholeExtent[4] << " "
     << this->WholeExtent[5] << "\n";
}
