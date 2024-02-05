// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVArrayCalculator.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilter.h"
#include "vtkPointData.h"
#include "vtkTable.h"

#include <algorithm>
#include <cassert>
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

bool vtkInQuotes(const char* s)
{
  return s[0] == '\"' && s[strlen(s) - 1] == '\"';
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
vtkPVArrayCalculator::~vtkPVArrayCalculator() = default;

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
  int numberOfArrays = inDataAttrs->GetNumberOfArrays(); // the input
  for (int j = 0; j < numberOfArrays; j++)
  {
    vtkAbstractArray* array = inDataAttrs->GetAbstractArray(j);
    const char* arrayName = array->GetName();
    if (!arrayName)
    {
      vtkWarningMacro("Skipping unnamed array at index " << j);
      continue;
    }

    int numberComps = array->GetNumberOfComponents();

    if (numberComps == 1)
    {
      std::string validVariableName = vtkArrayCalculator::CheckValidVariableName(arrayName);
      this->AddScalarVariable(validVariableName.c_str(), arrayName);
      if (validVariableName == arrayName && !vtkInQuotes(arrayName))
      {
        this->AddScalarVariable(vtkQuoteString(arrayName).c_str(), arrayName);
      }
    }
    else
    {
      for (int i = 0; i < numberComps; i++)
      {
        std::set<std::string> possibleNames;

        if (array->GetComponentName(i))
        {
          std::string name = vtkJoinToString(arrayName, array->GetComponentName(i));
          std::string validVariableName = vtkArrayCalculator::CheckValidVariableName(name.c_str());
          possibleNames.insert(validVariableName);
          if (validVariableName == name && !vtkInQuotes(name.c_str()))
          {
            possibleNames.insert(vtkQuoteString(name));
          }
        }
        std::string name =
          vtkJoinToString(arrayName, vtkPVPostFilter::DefaultComponentName(i, numberComps));
        std::string validVariableName = vtkArrayCalculator::CheckValidVariableName(name.c_str());
        possibleNames.insert(validVariableName);
        if (validVariableName == name && !vtkInQuotes(name.c_str()))
        {
          possibleNames.insert(vtkQuoteString(name));
        }

        // also put a <ArrayName>_<ComponentNumber> to handle past versions of
        // vtkPVArrayCalculator when component names were not used and index was
        // used e.g. state files prior to fixing of BUG #12951.
        std::string defaultName = vtkJoinToString(arrayName, i);

        std::string defaultValidVariableName =
          vtkArrayCalculator::CheckValidVariableName(defaultName.c_str());
        possibleNames.insert(defaultValidVariableName);
        if (defaultValidVariableName == defaultName && !vtkInQuotes(defaultName.c_str()))
        {
          possibleNames.insert(vtkQuoteString(defaultName));
        }

        std::for_each(
          possibleNames.begin(), possibleNames.end(), add_scalar_variables(this, arrayName, i));
      }

      if (numberComps == 3)
      {
        std::string validVariableName = vtkArrayCalculator::CheckValidVariableName(arrayName);
        this->AddVectorVariable(validVariableName.c_str(), arrayName);
        if (validVariableName == arrayName && !vtkInQuotes(arrayName))
        {
          this->AddVectorVariable(vtkQuoteString(arrayName).c_str(), arrayName);
        }
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
    bool supportGivenAttribute = true;
    for (cdIter->InitTraversal(); !cdIter->IsDoneWithTraversal(); cdIter->GoToNextItem())
    {
      vtkDataObject* dataObject = cdIter->GetCurrentDataObject();
      if (dataObject)
      {
        int attributeType = this->GetAttributeTypeFromInput(dataObject);
        vtkDataSetAttributes* dataAttrs = dataObject->GetAttributes(attributeType);
        if (dataAttrs)
        {
          vtkIdType numTuples = dataAttrs->GetNumberOfTuples();
          if (numTuples > 0)
          {
            this->AddArrayAndVariableNames(input, dataAttrs);
          }
        }
        else
        {
          supportGivenAttribute = false;
        }
      }
    }
    if (!supportGivenAttribute)
    {
      vtkWarningMacro("Some blocks do not support the given attribue type. Resulting array will be "
                      "partial or inexistant.");
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
