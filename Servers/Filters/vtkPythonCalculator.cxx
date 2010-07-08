/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonCalculator.h"

#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPointData.h"
#include "vtkPythonProgrammableFilter.h"
#include "vtkCellData.h"
#include "vtkProcessModule.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/string>

vtkStandardNewMacro(vtkPythonCalculator);

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkPythonCalculator::vtkPythonCalculator()
{
  this->Expression = NULL;
  this->ArrayName = NULL;
  this->SetArrayName("result");
  this->SetExecuteMethod(vtkPythonCalculator::ExecuteScript, this);
  this->ArrayAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->CopyArrays = true;
}

//----------------------------------------------------------------------------
vtkPythonCalculator::~vtkPythonCalculator()
{
  this->SetExpression(NULL);
  this->SetArrayName(NULL);
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::RequestDataObject(
  vtkInformation* vtkNotUsed(request), 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  // Output type is same as input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
      
      if (!output || !output->IsA(input->GetClassName())) 
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::ExecuteScript(void *arg)
{  
  vtkPythonCalculator *self = 
    static_cast<vtkPythonCalculator*>(arg);
  if (self)
    {
    self->Exec(self->GetExpression(), "RequestData");
    }
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::Exec(const char* expression,
                               const char* funcname)
{
  if (!expression)
    {
    return;
    }

  vtkDataObject* firstInput = this->GetInputDataObject(0, 0);
  vtkFieldData* fd = 0;
  if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(firstInput);
    if (ds)
      {
      fd = ds->GetPointData();
      }    
    }
  else if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(firstInput);
    if (ds)
      {
      fd = ds->GetCellData();
      }        
    }
  if (!fd)
    {
    vtkErrorMacro("Unexpected association value.");
    return;
    }

  // Replace tabs with two spaces
  vtkstd::string orgscript;
  size_t len = strlen(expression);
  for(size_t i=0; i< len; i++)
    {
    if (expression[i] == '\t')
      {
      orgscript += "  ";
      }
    else
      {
      orgscript.push_back(expression[i]);
      }
    }
    
  //size_t pos = orgscript.rfind("\n");
    
  // Construct a script that defines a function
  vtkstd::string fscript;
  fscript  = "def ";
  fscript += funcname;

  fscript += "(self, inputs):\n";
  fscript += "  arrays = {}\n";
  int narrays = fd->GetNumberOfArrays();
  for (int i=0; i<narrays; i++)
    {
    const char* aname = fd->GetArray(i)->GetName();
    if (aname)
      {
      fscript += "  import paraview\n";
      fscript += "  name = paraview.make_name_valid(\"";
      fscript += aname;
      fscript += "\")\n";
      fscript += "  if name:\n";
      fscript += "    try:\n";
      fscript += "      exec \"%s = inputs[0].";
      if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        {
        fscript += "PointData['";
        }
      else if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
        {
        fscript += "CellData['";
        }
      fscript += aname;
      fscript += "']\" % (name)\n";
      fscript += "    except: pass\n";
      fscript += "  arrays['";
      fscript += aname;
      fscript += "'] = inputs[0].";
      if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        {
        fscript += "PointData['";
        }
      else if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
        {
        fscript += "CellData['";
        }
      fscript += aname;
      fscript += "']\n";
      }
    }
  fscript += "  try:\n";
  fscript += "    points = inputs[0].Points\n";
  fscript += "  except: pass\n";
  
  if (expression && strlen(expression) > 0)  
    {
    fscript += "  retVal = ";
    fscript += orgscript;
    fscript += "\n";
    fscript += "  if not isinstance(retVal, ndarray):\n";
    fscript += "    retVal = retVal * ones((inputs[0].GetNumberOf";
    if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      fscript += "Points(), 1))\n";
      }
    else if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      {
      fscript += "Cells(), 1))\n";
      }
    fscript += "  return retVal\n";
    }
  else
    {
    fscript += "  return None\n";
    }
  
  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->RunSimpleString(fscript.c_str());

  vtkstd::string runscript;
  runscript += "import paraview\n";
  runscript += "paraview.fromFilter = True\n";
  runscript += "from paraview import vtk\n";
  runscript += "from paraview.vtk import dataset_adapter\n";
  runscript += "from numpy import *\n";
  runscript += "from paraview.vtk.algorithms import *\n";
  runscript += "from paraview import servermanager\n";
  runscript += "if servermanager.progressObserverTag:\n";
  runscript += "  servermanager.ToggleProgressPrinting()\n";

  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", this);    
  char *aplus = addrofthis; 
  if ((addrofthis[0] == '0') && 
      ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
    {
    aplus += 2; //skip over "0x"
    }
  
  // Call the function
  runscript += "myarg = ";
  runscript += "vtk.vtkProgrammableFilter('";
  runscript += aplus;
  runscript += "')\n";
  runscript += "inputs = []\n";
  runscript += "index = 0\n";
  int numinps = this->GetNumberOfInputConnections(0);
  for (int i=0; i<numinps; i++)
    {
    runscript += 
      "inputs.append(dataset_adapter.WrapDataObject(myarg.GetInputDataObject(0, index)))\n";
    runscript += "index += 1\n";
    }
  runscript += "output = dataset_adapter.WrapDataObject(myarg.GetOutputDataObject(0))\n";
  if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    runscript +=  "fd = output.PointData\n";
    }
  else if (this->ArrayAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    runscript += "fd = output.CellData\n";
    }
  if (this->CopyArrays)
    {
    runscript += "output.GetPointData().PassData(inputs[0].GetPointData().VTKObject)\n";
    runscript += "output.GetCellData().PassData(inputs[0].GetCellData().VTKObject)\n";
    }
  runscript += "retVal = ";
  runscript += funcname;
  runscript += "(vtk.vtkProgrammableFilter('";
  runscript += aplus;
  runscript += "'), inputs)\n";
  runscript += "if retVal is not None:\n";
  runscript += "  fd.append(retVal, '";
  runscript += this->GetArrayName();
  runscript += "')\n";
  runscript += "del myarg\n";
  runscript += "del inputs\n";
  runscript += "del fd\n";
  runscript += "del retVal\n";
  runscript += "del output\n";
  
  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->RunSimpleString(runscript.c_str());
  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->FlushMessages();
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  if(port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
