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

#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtkstd/algorithm>

class vtkCPInputDataDescription::vtkInternals
{
public:
  typedef vtkstd::vector<vtkstd::string> FieldType;
  FieldType PointFields;
  FieldType CellFields;
};

vtkStandardNewMacro(vtkCPInputDataDescription);
vtkCxxRevisionMacro(vtkCPInputDataDescription, "1.1");
vtkCxxSetObjectMacro(vtkCPInputDataDescription, Grid, vtkDataObject);
//----------------------------------------------------------------------------
vtkCPInputDataDescription::vtkCPInputDataDescription()
{
  this->Grid = 0;
  this->GenerateMesh = false;
  this->AllFields = false;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkCPInputDataDescription::~vtkCPInputDataDescription()
{
  this->SetGrid(0);
  delete this->Internals;
  this->Internals = 0;
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
    this->Internals->PointFields.size() +
    this->Internals->CellFields.size());
}

//----------------------------------------------------------------------------
const char* vtkCPInputDataDescription::GetFieldName(unsigned int fieldIndex)
{
  if (fieldIndex >= this->GetNumberOfFields())
    {
    vtkWarningMacro("Bad FieldIndex " << fieldIndex);
    return 0;
    }

  if (fieldIndex >= 
    static_cast<unsigned int>(this->Internals->PointFields.size()))
    {
    fieldIndex -=
      static_cast<unsigned int>(this->Internals->PointFields.size());
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

  if (vtkstd::find(this->Internals->PointFields.begin(),
    this->Internals->PointFields.end(),
    fieldName) != this->Internals->PointFields.end())
    {
    return true;
    }

  if (vtkstd::find(this->Internals->CellFields.begin(),
    this->Internals->CellFields.end(),
    fieldName) != this->Internals->CellFields.end())
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsFieldPointData(const char* fieldName)
{
  if (vtkstd::find(this->Internals->PointFields.begin(),
    this->Internals->PointFields.end(),
    fieldName) != this->Internals->PointFields.end())
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCPInputDataDescription::GetGrid()
{
  return this->Grid;
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::IsInputSufficient()
{
  if (this->Grid == 0)
    {
    return false;
    }

  vtkDataSet* DataSet = vtkDataSet::SafeDownCast(this->Grid);
  if(DataSet)
    {
    return this->DoesGridContainNeededFields(DataSet);
    }
  vtkCompositeDataSet* Composite = 
    vtkCompositeDataSet::SafeDownCast(this->Grid);
  if(Composite)
    {
    vtkCompositeDataIterator* Iter = Composite->NewIterator();
    Iter->VisitOnlyLeavesOn();
    Iter->TraverseSubTreeOn();
    Iter->SkipEmptyNodesOn();
    for(Iter->GoToFirstItem();!Iter->IsDoneWithTraversal();Iter->GoToNextItem())
      {
      DataSet = vtkDataSet::SafeDownCast(Iter->GetDataSet());
      if(DataSet)
        {
        if(!this->DoesGridContainNeededFields(DataSet))
          {
          Iter->Delete();
          return false;
          }
        }
      }
    Iter->Delete();
    return true;
    }
  
  return false; // false because of unknown grid type
}

//----------------------------------------------------------------------------
bool vtkCPInputDataDescription::DoesGridContainNeededFields(vtkDataSet* dataSet)
{
  vtkInternals::FieldType::iterator iter;
  for (iter = this->Internals->PointFields.begin();
    iter != this->Internals->PointFields.end();
    ++iter)
    {
    vtkstd::string fieldName = *iter;
    if (dataSet->GetPointData()->GetArray(fieldName.c_str()) == 0)
      {
      return false;
      }
    }
  
  for (iter = this->Internals->CellFields.begin();
    iter != this->Internals->CellFields.end();
    ++iter)
    {
    vtkstd::string fieldName = *iter;
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
  return (this->AllFields ||
    this->GetNumberOfFields() > 0 || this->GenerateMesh);
}

//----------------------------------------------------------------------------
void vtkCPInputDataDescription::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AllFields: " << this->AllFields << "\n";
  os << indent << "GenerateMesh: " << this->GenerateMesh << "\n";
  os << indent << "Grid: " << this->Grid << "\n";
}

