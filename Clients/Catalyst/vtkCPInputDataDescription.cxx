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
#include "vtkSMSourceProxy.h"

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
vtkCxxSetObjectMacro(vtkCPInputDataDescription, TemporalCache, vtkSMSourceProxy);
//----------------------------------------------------------------------------
vtkCPInputDataDescription::vtkCPInputDataDescription()
{
  this->Grid = NULL;
  this->GenerateMesh = false;
  this->AllFields = false;
  this->Internals = new vtkCPInputDataDescription::vtkInternals();
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;
  this->TemporalCache = nullptr;
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
  this->SetTemporalCache(nullptr);
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
  this->SetTemporalCache(idd->TemporalCache);
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
