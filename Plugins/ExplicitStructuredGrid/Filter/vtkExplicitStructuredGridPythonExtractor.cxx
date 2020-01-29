/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridPythonExtractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitStructuredGridPythonExtractor.h"

#include <vtkAlgorithmOutput.h>
#include <vtkCellData.h>
#include <vtkExplicitStructuredGrid.h>
#include <vtkIdList.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPython.h>
#include <vtkPythonCompatibility.h>
#include <vtkPythonInterpreter.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <sstream>

vtkStandardNewMacro(vtkExplicitStructuredGridPythonExtractor);

//-----------------------------------------------------------------------------
vtkExplicitStructuredGridPythonExtractor::vtkExplicitStructuredGridPythonExtractor()
{
  this->SetPythonExpression("ret=0");
}

//-----------------------------------------------------------------------------
vtkExplicitStructuredGridPythonExtractor::~vtkExplicitStructuredGridPythonExtractor()
{
  delete[] this->PythonExpression;
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridPythonExtractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "PythonExpression: " << this->PythonExpression << std::endl;
  os << "PassDataToScript: " << this->PassDataToScript << std::endl;
}

//----------------------------------------------------------------------------
// Change the WholeExtent
int vtkExplicitStructuredGridPythonExtractor::RequestInformation(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int extent[6];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridPythonExtractor::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // We can handle anything.
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridPythonExtractor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkExplicitStructuredGrid* input = vtkExplicitStructuredGrid::GetData(inputVector[0], 0);
  vtkExplicitStructuredGrid* output = vtkExplicitStructuredGrid::GetData(outputVector, 0);

  std::vector<vtkDataArray*> cellArrays;
  std::vector<vtkDataArray*> pointArrays;

  vtkPythonInterpreter::Initialize();
  if (this->PassDataToScript)
  {
    // Get input data arrays
    for (vtkIdType i = 0; i < input->GetCellData()->GetNumberOfArrays(); i++)
    {
      vtkDataArray* arr = input->GetCellData()->GetArray(i);
      if (arr)
      {
        cellArrays.push_back(arr);
      }
    }

    // Get input point arrays
    for (vtkIdType i = 0; i < input->GetPointData()->GetNumberOfArrays(); i++)
    {
      vtkDataArray* arr = input->GetPointData()->GetArray(i);
      if (arr)
      {
        pointArrays.push_back(arr);
      }
    }
  }

  int extent[6];
  input->GetExtent(extent);
  output->DeepCopy(input);

  // Browse input data and copy cell attributes to output
  for (int k = extent[4]; k < extent[5]; ++k)
  {
    for (int j = extent[2]; j < extent[3]; ++j)
    {
      for (int i = extent[0]; i < extent[1]; ++i)
      {
        int cellId = input->ComputeCellId(i, j, k, true);
        if (!input->IsCellVisible(cellId))
        {
          continue;
        }

        vtkNew<vtkIdList> ptIds;
        input->GetCellPoints(cellId, ptIds);
        int keepCell =
          this->EvaluatePythonExpression(cellId, ptIds, i, j, k, cellArrays, pointArrays);

        if (keepCell == 0)
        {
          output->BlankCell(cellId);
        }
      }
    }
  }
  output->ComputeFacesConnectivityFlagsArray();
  this->UpdateProgress(1.);
  return 1;
}

//----------------------------------------------------------------------------
// This method computes if the current cell must be extracted or not comparing
// the expression to the current values of every expression's variables
bool vtkExplicitStructuredGridPythonExtractor::EvaluatePythonExpression(vtkIdType cellId,
  vtkIdList* ptIds, int i, int j, int k, std::vector<vtkDataArray*>& cellArrays,
  std::vector<vtkDataArray*>& pointArrays)
{
  std::stringstream ss;
  // Add (i,j,k) values into ss
  ss << "ret=0" << endl;
  ss << "i=" << i << endl;
  ss << "j=" << j << endl;
  ss << "k=" << k << endl;

  if (this->PassDataToScript)
  {
    // Add cellArrays values into ss
    ss << "CellArray = dict()" << endl;
    for (unsigned int idx = 0; idx < cellArrays.size(); idx++)
    {
      int nbComp = cellArrays[idx]->GetNumberOfComponents();
      ss << "CellArray['";
      ss << cellArrays[idx]->GetName() << "'] = ";
      double* tuple = new double[nbComp];
      cellArrays[idx]->GetTuple(cellId, tuple);
      if (nbComp == 1)
      {
        ss << tuple[0] << endl;
      }
      else
      {
        ss << " [";
        ss << tuple[0];
        for (vtkIdType l = 1; l < nbComp; l++)
        {
          ss << "," << tuple[l];
        }
        ss << "]" << endl;
      }
      delete[] tuple;
    }
    // Add pointArrays values into ss
    ss << "PointArray = dict()" << endl;
    for (unsigned int idx = 0; idx < pointArrays.size(); idx++)
    {
      int nbComp = pointArrays[idx]->GetNumberOfComponents();
      ss << "PointArray['";
      ss << pointArrays[idx]->GetName() << "'] = [";

      if (nbComp > 1)
      {
        double* tuple = new double[nbComp];
        pointArrays[idx]->GetTuple(ptIds->GetId(0), tuple);
        ss << "[" << tuple[0];
        for (vtkIdType icomp = 1; icomp < nbComp; icomp++)
        {
          ss << "," << tuple[icomp];
        }
        ss << "]";

        for (vtkIdType l = 1; l < ptIds->GetNumberOfIds(); l++)
        {
          pointArrays[idx]->GetTuple(ptIds->GetId(l), tuple);
          ss << ",[" << tuple[0];
          for (vtkIdType icomp = 1; icomp < nbComp; icomp++)
          {
            ss << "," << tuple[icomp];
          }
          ss << "]";
        }
        delete[] tuple;
      }
      else
      {
        double* tuple = new double[nbComp];
        pointArrays[idx]->GetTuple(ptIds->GetId(0), tuple);
        ss << tuple[0];
        for (vtkIdType l = 1; l < ptIds->GetNumberOfIds(); l++)
        {
          pointArrays[idx]->GetTuple(ptIds->GetId(l), tuple);
          ss << "," << tuple[0];
        }
        delete[] tuple;
      }

      ss << "]" << endl;
    }
  }
  ss << this->PythonExpression;

  vtkPythonInterpreter::RunSimpleString(ss.str().c_str());

  // Fetch the result value
  bool ret = false;
  PyObject* main_module = PyImport_ImportModule("__main__");
  PyObject* global_dict = PyModule_GetDict(main_module);
  PyObject* iobj = PyDict_GetItemString(global_dict, "ret");
  ret = PyInt_AsLong(iobj) != 0;
  return ret;
}
