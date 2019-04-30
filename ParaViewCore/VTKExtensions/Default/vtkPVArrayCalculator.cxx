/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArrayCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVArrayCalculator.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFunctionParser.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilter.h"
#include "vtkPointData.h"
#include "vtkTable.h"

#include <algorithm>
#include <assert.h>
#include <set>
#include <sstream>
#include <string>

namespace
{
template <class A, class B>
std::string vtkJoinToString(const A& a, const B& b)
{
  std::ostringstream stream;
  stream << a << "_" << b;
  return stream.str();
}

std::string vtkQuoteString(const std::string& s)
{
  std::ostringstream stream;
  stream << '\"' << s << '\"';
  return stream.str();
}

class add_scalar_variables
{
  vtkPVArrayCalculator* Calc;
  const char* ArrayName;
  int Component;

public:
  add_scalar_variables(vtkPVArrayCalculator* calc, const char* array_name, int component_num)
    : Calc(calc)
    , ArrayName(array_name)
    , Component(component_num)
  {
  }
  void operator()(const std::string& name)
  {
    this->Calc->AddScalarVariable(name.c_str(), this->ArrayName, this->Component);
  }
};
}

vtkStandardNewMacro(vtkPVArrayCalculator);
// ----------------------------------------------------------------------------
vtkPVArrayCalculator::vtkPVArrayCalculator()
{
  // We'll tell the superclass about all arrays (partial and full) and have it
  // ignore missing arrays when evaluating the calculator.
  this->IgnoreMissingArrays = true;
}

// ----------------------------------------------------------------------------
vtkPVArrayCalculator::~vtkPVArrayCalculator()
{
}

// ----------------------------------------------------------------------------
int vtkPVArrayCalculator::GetAttributeTypeFromInput(vtkDataObject* input)
{
  int attributeType = this->AttributeType;
  if (attributeType == vtkArrayCalculator::DEFAULT_ATTRIBUTE_TYPE)
  {
    vtkGraph* graphInput = vtkGraph::SafeDownCast(input);
    vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
    vtkTable* tableInput = vtkTable::SafeDownCast(input);
    if (graphInput)
    {
      attributeType = vtkDataObject::VERTEX;
    }
    if (dsInput)
    {
      attributeType = vtkDataObject::POINT;
    }
    if (tableInput)
    {
      attributeType = vtkDataObject::ROW;
    }
  }

  return attributeType;
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::ResetArrayAndVariableNames()
{
  // Make sure we reparse the function based on the current array order
  this->FunctionParser->InvalidateFunction();

  // Look at the data-arrays available in the input and register them as
  // variables with the superclass.
  // It's safe to call these methods in RequestData() since they don't call
  // this->Modified().
  this->RemoveAllVariables();
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::AddCoordinateVariableNames()
{
  // Add coordinate scalar and vector variables
  this->AddCoordinateScalarVariable("coordsX", 0);
  this->AddCoordinateScalarVariable("coordsY", 1);
  this->AddCoordinateScalarVariable("coordsZ", 2);
  this->AddCoordinateVectorVariable("coords", 0, 1, 2);
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::AddArrayAndVariableNames(
  vtkDataObject* vtkNotUsed(theInputObj), vtkDataSetAttributes* inDataAttrs)
{
  // add non-coordinate scalar and vector variables
  int numberArays = inDataAttrs->GetNumberOfArrays(); // the input
  for (int j = 0; j < numberArays; j++)
  {
    vtkAbstractArray* array = inDataAttrs->GetAbstractArray(j);
    const char* array_name = array->GetName();

    int numberComps = array->GetNumberOfComponents();

    if (numberComps == 1)
    {
      this->AddScalarVariable(array_name, array_name, 0);
      this->AddScalarVariable(vtkQuoteString(array_name).c_str(), array_name);
    }
    else
    {
      for (int i = 0; i < numberComps; i++)
      {
        std::set<std::string> possible_names;

        if (array->GetComponentName(i))
        {
          std::string name(vtkJoinToString(array_name, array->GetComponentName(i)));
          possible_names.insert(name);
          possible_names.insert(vtkQuoteString(name).c_str());
        }
        std::string name(
          vtkJoinToString(array_name, vtkPVPostFilter::DefaultComponentName(i, numberComps)));
        possible_names.insert(name);
        possible_names.insert(vtkQuoteString(name).c_str());

        // also put a <ArrayName>_<ComponentNumber> to handle past versions of
        // vtkPVArrayCalculator when component names were not used and index was
        // used e.g. state files prior to fixing of BUG #12951.
        std::string default_name = vtkJoinToString(array_name, i);

        possible_names.insert(default_name);
        possible_names.insert(vtkQuoteString(default_name).c_str());

        std::for_each(
          possible_names.begin(), possible_names.end(), add_scalar_variables(this, array_name, i));
      }

      if (numberComps == 3)
      {
        this->AddVectorArrayName(array_name, 0, 1, 2);
        this->AddVectorVariable(vtkQuoteString(array_name).c_str(), array_name, 0, 1, 2);
      }
    }
  }
}

// ----------------------------------------------------------------------------
int vtkPVArrayCalculator::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);

  // Ensure the mtime is not modified in RequestData.
  vtkMTimeType mtime = this->GetMTime();

  // Reset the array and variable names to start out
  this->ResetArrayAndVariableNames();

  // Add coordinate variable names just once per multiblock dataset
  this->AddCoordinateVariableNames();

  // Add arrays to the calculator
  auto inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (inputCD)
  {
    vtkSmartPointer<vtkCompositeDataIterator> cdIter;
    cdIter.TakeReference(inputCD->NewIterator());
    cdIter->SkipEmptyNodesOn();
    for (cdIter->InitTraversal(); !cdIter->IsDoneWithTraversal(); cdIter->GoToNextItem())
    {
      vtkDataObject* dataObject = cdIter->GetCurrentDataObject();
      if (dataObject)
      {
        int attributeType = this->GetAttributeTypeFromInput(dataObject);
        vtkDataSetAttributes* dataAttrs = dataObject->GetAttributes(attributeType);
        vtkIdType numTuples = dataAttrs->GetNumberOfTuples();
        if (numTuples > 0)
        {
          this->AddArrayAndVariableNames(input, dataAttrs);
        }
      }
    }
  }
  else
  {
    int attributeType = this->GetAttributeTypeFromInput(input);
    vtkDataSetAttributes* dataAttrs = input->GetAttributes(attributeType);
    vtkIdType numTuples = dataAttrs->GetNumberOfTuples();
    if (numTuples > 0)
    {
      this->AddArrayAndVariableNames(input, dataAttrs);
    }
  }

  assert(this->GetMTime() == mtime && "post: mtime cannot be changed in RequestData()");
  (void)mtime;

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
