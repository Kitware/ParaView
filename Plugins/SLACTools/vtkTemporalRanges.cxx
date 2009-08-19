// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalRanges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkTemporalRanges.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTypeTraits.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtkstd/algorithm>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#include <math.h>

//=============================================================================
// This is not super portable.  We really should be using vtkMath::IsNan.  The
// only reason we are not is so that we can copy this code to the 3.6 branch of
// ParaView where that method does not yet exist.  Once we move past that, this
// section should go away and all instances of isnan should be replaced with
// vtkMath::IsNan.
#ifndef isnan
// This is compiler specific not platform specific: MinGW doesn't need that.
# if defined(_MSC_VER) || defined(__BORLANDC__)
#  include <float.h>
#  define isnan(x) _isnan(x)
# endif
#endif

//=============================================================================
namespace vtkTemporalRangesNamespace{
  const int AVERAGE_ROW = vtkTemporalRanges::AVERAGE_ROW;
  const int MINIMUM_ROW = vtkTemporalRanges::MINIMUM_ROW;
  const int MAXIMUM_ROW = vtkTemporalRanges::MAXIMUM_ROW;
  const int COUNT_ROW   = vtkTemporalRanges::COUNT_ROW;
  const int NUMBER_OF_ROWS = vtkTemporalRanges::NUMBER_OF_ROWS;

  inline void InitializeColumn(vtkDoubleArray *column)
  {
    column->SetNumberOfComponents(1);
    column->SetNumberOfTuples(NUMBER_OF_ROWS);

    column->SetValue(AVERAGE_ROW, 0.0);
    column->SetValue(MINIMUM_ROW, vtkTypeTraits<double>::Max());
    column->SetValue(MAXIMUM_ROW, vtkTypeTraits<double>::Min());
    column->SetValue(COUNT_ROW,   0.0);
  }

  inline void AccumulateValue(vtkDoubleArray *column, double value)
  {
    if (!isnan(value))
      {
      column->SetValue(AVERAGE_ROW, value + column->GetValue(AVERAGE_ROW));
      column->SetValue(MINIMUM_ROW, vtkstd::min(column->GetValue(MINIMUM_ROW),
                                                value));
      column->SetValue(MAXIMUM_ROW, vtkstd::max(column->GetValue(MAXIMUM_ROW),
                                                value));
      column->SetValue(COUNT_ROW, column->GetValue(COUNT_ROW) + 1);
      }
  }

  inline void AccumulateColumns(vtkDoubleArray *target,
                                vtkDoubleArray *source)
  {
    double targetCount = target->GetValue(COUNT_ROW);
    double sourceCount = source->GetValue(COUNT_ROW);
    double totalCount = targetCount + sourceCount;
    double targetTotal = targetCount*target->GetValue(AVERAGE_ROW);
    double sourceTotal = sourceCount*source->GetValue(AVERAGE_ROW);
    target->SetValue(AVERAGE_ROW, (targetTotal + sourceTotal)/totalCount);
    target->SetValue(MINIMUM_ROW, vtkstd::min(source->GetValue(MINIMUM_ROW),
                                              target->GetValue(MINIMUM_ROW)));
    target->SetValue(MAXIMUM_ROW, vtkstd::max(source->GetValue(MAXIMUM_ROW),
                                              target->GetValue(MAXIMUM_ROW)));
    target->SetValue(COUNT_ROW, totalCount);
  }
};
using namespace vtkTemporalRangesNamespace;

//=============================================================================
vtkCxxRevisionMacro(vtkTemporalRanges, "1.2");
vtkStandardNewMacro(vtkTemporalRanges);

//-----------------------------------------------------------------------------
vtkTemporalRanges::vtkTemporalRanges()
{
}

vtkTemporalRanges::~vtkTemporalRanges()
{
}

void vtkTemporalRanges::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkTemporalRanges::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
    {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    return 1;
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkTemporalRanges::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  vtkTable *output = vtkTable::GetData(outputVector);

  this->InitializeTable(output);

  vtkCompositeDataSet *compositeInput
    = vtkCompositeDataSet::GetData(inputVector[0]);
  if (compositeInput)
    {
    this->AccumulateCompositeData(compositeInput, output);
    return 1;
    }

  vtkDataSet *dsInput = vtkDataSet::GetData(inputVector[0]);
  if (dsInput)
    {
    this->AccumulateDataSet(dsInput, output);
    return 1;
    }

  vtkWarningMacro(<< "Unknown data type : "
                  << vtkDataObject::GetData(inputVector[0])->GetClassName());
  return 0;
}

//-----------------------------------------------------------------------------
void vtkTemporalRanges::InitializeTable(vtkTable *output)
{
  output->Initialize();

  VTK_CREATE(vtkStringArray, rangeName);
  rangeName->SetName("Range Name");
  rangeName->SetNumberOfComponents(1);
  rangeName->SetNumberOfTuples(NUMBER_OF_ROWS);

  rangeName->SetValue(AVERAGE_ROW, "Average");
  rangeName->SetValue(MINIMUM_ROW, "Minimum");
  rangeName->SetValue(MAXIMUM_ROW, "Maximum");
  rangeName->SetValue(COUNT_ROW, "Count");

  output->AddColumn(rangeName);
}

//-----------------------------------------------------------------------------
void vtkTemporalRanges::AccumulateCompositeData(vtkCompositeDataSet *input,
                                                vtkTable *output)
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
       iter->GoToNextItem())
    {
    vtkDataObject *dataobj = iter->GetCurrentDataObject();
    vtkDataSet *dataset = vtkDataSet::SafeDownCast(dataobj);
    if (dataset)
      {
      this->AccumulateDataSet(dataset, output);
      }
    else if (dataobj)
      {
      vtkWarningMacro(<< "Unknown data type : " << dataobj->GetClassName());
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTemporalRanges::AccumulateDataSet(vtkDataSet *input, vtkTable *output)
{
  this->AccumulateFields(input->GetPointData(), output);
  this->AccumulateFields(input->GetCellData(), output);
  this->AccumulateFields(input->GetFieldData(), output);
}

//-----------------------------------------------------------------------------
void vtkTemporalRanges::AccumulateFields(vtkFieldData *fields, vtkTable *output)
{
  for (int i = 0; i < fields->GetNumberOfArrays(); i++)
    {
    vtkDataArray *array = fields->GetArray(i);
    if (array)
      {
      this->AccumulateArray(array, output);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTemporalRanges::AccumulateArray(vtkDataArray *field, vtkTable *output)
{
  int numComponents = field->GetNumberOfComponents();
  vtkIdType numTuples = field->GetNumberOfTuples();
  vtkDoubleArray *magnitudeColumn = NULL;
  vtkstd::vector<vtkDoubleArray *> componentColumns(numComponents);
  VTK_CREATE(vtkDoubleArray, magnitudeAccumulate);
  vtkstd::vector<vtkSmartPointer<vtkDoubleArray> > componentAccumulate(numComponents);
  if (numComponents > 1)
    {
    magnitudeColumn = this->GetColumn(output, field->GetName(), -1);
    InitializeColumn(magnitudeAccumulate);
    for (int i = 0; i < numComponents; i++)
      {
      componentColumns[i] = this->GetColumn(output, field->GetName(), i);
      componentAccumulate[i] = vtkSmartPointer<vtkDoubleArray>::New();
      InitializeColumn(componentAccumulate[i]);
      }
    }
  else
    {
    componentColumns[0] = this->GetColumn(output, field->GetName());
    componentAccumulate[0] = vtkSmartPointer<vtkDoubleArray>::New();
    InitializeColumn(componentAccumulate[0]);
    }

  for (vtkIdType i = 0; i < numTuples; i++)
    {
    double mag = 0.0;
    for (int j = 0; j < numComponents; j++)
      {
      double value = field->GetComponent(i, j);
      mag += value*value;
      AccumulateValue(componentAccumulate[j], value);
      }
    if (magnitudeColumn)
      {
      mag = sqrt(mag);
      AccumulateValue(magnitudeAccumulate, mag);
      }
    }

  for (int j = 0; j < numComponents; j++)
    {
    componentAccumulate[j]->SetValue(AVERAGE_ROW,
                              (  componentAccumulate[j]->GetValue(AVERAGE_ROW)
                               / componentAccumulate[j]->GetValue(COUNT_ROW) ));
    AccumulateColumns(componentColumns[j], componentAccumulate[j]);
    }
  if (magnitudeColumn)
    {
    magnitudeAccumulate->SetValue(AVERAGE_ROW,
                                  (  magnitudeAccumulate->GetValue(AVERAGE_ROW)
                                   / magnitudeAccumulate->GetValue(COUNT_ROW)));
    AccumulateColumns(magnitudeColumn, magnitudeAccumulate);
    }
}

//-----------------------------------------------------------------------------
vtkDoubleArray *vtkTemporalRanges::GetColumn(vtkTable *table, const char *name,
                                             int component)
{
  vtksys_ios::ostringstream fullname;
  fullname << name << "_";
  if (component < 0)
    {
    fullname << "M";
    }
  else
    {
    fullname << component;
    }
  vtkstd::string fullnamestring = fullname.str();
  return this->GetColumn(table, fullnamestring.c_str());
}

//-----------------------------------------------------------------------------
vtkDoubleArray *vtkTemporalRanges::GetColumn(vtkTable *table, const char *name)
{
  vtkAbstractArray *abstractArray = table->GetColumnByName(name);
  vtkDoubleArray *array = vtkDoubleArray::SafeDownCast(abstractArray);
  if (!array)
    {
    if (abstractArray)
      {
      table->RemoveColumnByName(name);
      }
    array = vtkDoubleArray::New();
    array->SetName(name);
    InitializeColumn(array);
    table->AddColumn(array);
    array->Delete();    //Reference held by table.
    }

  return array;
}
