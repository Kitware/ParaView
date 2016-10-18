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
#include <string>
#include <vector>

class vtkCPInputDataDescription::vtkInternals
{
public:
  typedef std::vector<std::string> FieldType;
  FieldType PointFields;
  FieldType CellFields;
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
  this->SetGrid(0);
  if (this->Internals)
  {
    delete this->Internals;
    this->Internals = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::Reset()
{
  this->Internals->PointFields.clear();
  this->Internals->CellFields.clear();
  this->AllFields = false;
  this->GenerateMesh = false;
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::AddPointField(const char* fieldName)
{
  this->Internals->PointFields.push_back(fieldName);
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::AddCellField(const char* fieldName)
{
  this->Internals->CellFields.push_back(fieldName);
}

//----------------------------------------------------------------------------
unsigned int vtkCPInputDataDescription::GetNumberOfFields()
{
  return static_cast<unsigned int>(
    this->Internals->PointFields.size() + this->Internals->CellFields.size());
}

//----------------------------------------------------------------------------
const char* vtkCPInputDataDescription::GetFieldName(unsigned int fieldIndex)
{
  if (fieldIndex >= this->GetNumberOfFields())
  {
    vtkWarningMacro("Bad FieldIndex " << fieldIndex);
    return 0;
  }

  if (fieldIndex >= static_cast<unsigned int>(this->Internals->PointFields.size()))
  {
    fieldIndex -= static_cast<unsigned int>(this->Internals->PointFields.size());
    return this->Internals->CellFields[fieldIndex].c_str();
  }
  return this->Internals->PointFields[fieldIndex].c_str();
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsFieldNeeded(const char* fieldName)
{
  if (!fieldName)
  {
    return false;
  }

  if (this->AllFields)
  {
    return true;
  }

  if (std::find(this->Internals->PointFields.begin(), this->Internals->PointFields.end(),
        fieldName) != this->Internals->PointFields.end())
  {
    return true;
  }

  if (std::find(this->Internals->CellFields.begin(), this->Internals->CellFields.end(),
        fieldName) != this->Internals->CellFields.end())
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsFieldPointData(const char* fieldName)
{
  if (std::find(this->Internals->PointFields.begin(), this->Internals->PointFields.end(),
        fieldName) != this->Internals->PointFields.end())
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsInputSufficient()
{
  if (this->Grid == 0)
  {
    return false;
  }

  vtkDataSet* dataSet = vtkDataSet::SafeDownCast(this->Grid);
  if (dataSet)
  {
    return this->DoesGridContainNeededFields(dataSet);
  }
  vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(this->Grid);
  if (composite)
  {
    vtkCompositeDataIterator* iter = composite->NewIterator();
    iter->SkipEmptyNodesOn();
    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      dataSet = vtkDataSet::SafeDownCast(iter->GetDataSet());
      if (dataSet)
      {
        if (!this->DoesGridContainNeededFields(dataSet))
        {
          iter->Delete();
          return false;
        }
      }
    }
    iter->Delete();
    return true;
  }

  return false; // false because of unknown grid type
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::DoesGridContainNeededFields(vtkDataSet* dataSet)
{
  vtkInternals::FieldType::iterator iter;
  for (iter = this->Internals->PointFields.begin(); iter != this->Internals->PointFields.end();
       ++iter)
  {
    std::string fieldName = *iter;
    if (dataSet->GetPointData()->GetArray(fieldName.c_str()) == 0)
    {
      return false;
    }
  }

  for (iter = this->Internals->CellFields.begin(); iter != this->Internals->CellFields.end();
       ++iter)
  {
    std::string fieldName = *iter;
    if (dataSet->GetCellData()->GetArray(fieldName.c_str()) == 0)
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::GetIfGridIsNecessary()
{
  return (this->AllFields || this->GetNumberOfFields() > 0 || this->GenerateMesh);
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
