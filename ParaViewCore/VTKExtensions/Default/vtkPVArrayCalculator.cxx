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
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFunctionParser.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPostFilter.h"
#include "vtkPointData.h"

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
}

// ----------------------------------------------------------------------------
vtkPVArrayCalculator::~vtkPVArrayCalculator()
{
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::UpdateArrayAndVariableNames(
  vtkDataObject* vtkNotUsed(theInputObj), vtkDataSetAttributes* inDataAttrs)
{
  vtkMTimeType mtime = this->GetMTime();

  // Make sure we reparse the function based on the current array order
  this->FunctionParser->InvalidateFunction();

  // Look at the data-arrays available in the input and register them as
  // variables with the superclass.
  // It's safe to call these methods in RequestData() since they don't call
  // this->Modified().
  this->RemoveAllVariables();

  // Add coordinate scalar and vector variables
  this->AddCoordinateScalarVariable("coordsX", 0);
  this->AddCoordinateScalarVariable("coordsY", 1);
  this->AddCoordinateScalarVariable("coordsZ", 2);
  this->AddCoordinateVectorVariable("coords", 0, 1, 2);

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

  assert(this->GetMTime() == mtime && "post: mtime cannot be changed in RequestData()");
  static_cast<void>(mtime); // added so compiler won't complain im release mode.
}

// ----------------------------------------------------------------------------
int vtkPVArrayCalculator::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());

  vtkIdType numTuples = 0;
  vtkGraph* graphInput = vtkGraph::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  vtkDataSetAttributes* dataAttrs = NULL;

  if (dsInput)
  {
    if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT ||
      this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
    {
      dataAttrs = dsInput->GetPointData();
      numTuples = dsInput->GetNumberOfPoints();
    }
    else
    {
      dataAttrs = dsInput->GetCellData();
      numTuples = dsInput->GetNumberOfCells();
    }
  }
  else if (graphInput)
  {
    if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT ||
      this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_VERTEX_DATA)
    {
      dataAttrs = graphInput->GetVertexData();
      numTuples = graphInput->GetNumberOfVertices();
    }
    else
    {
      dataAttrs = graphInput->GetEdgeData();
      numTuples = graphInput->GetNumberOfEdges();
    }
  }

  if (numTuples > 0)
  {
    // Let's update the (scalar and vector arrays / variables) names  to make
    // them consistent with those of the upstream calculator(s). This addresses
    // the scenarios where the user modifies the name of a calculator whose out-
    // put is the input of a (some) subsequent calculator(s) or the user changes
    // the input of a downstream calculator.
    this->UpdateArrayAndVariableNames(input, dataAttrs);
  }

  input = NULL;
  dsInput = NULL;
  dataAttrs = NULL;
  graphInput = NULL;

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
