/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonProgrammableFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkPython.h> // python first

#include "vtkPythonProgrammableFilter.h"
#include "vtkObjectFactory.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkCommonInstantiator.h"

#include <vtkstd/string>

vtkCxxRevisionMacro(vtkPythonProgrammableFilter, "1.6");
vtkStandardNewMacro(vtkPythonProgrammableFilter);

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::vtkPythonProgrammableFilter()
{
  this->Script = NULL;
  this->Interpretor = NULL;
  this->OutputDataSetType = VTK_DATA_SET;
  }

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::~vtkPythonProgrammableFilter()
{
  if (this->Script != NULL)
    {
    delete[] this->Script;
    }

  if (this->Interpretor != NULL)
    {
    this->Interpretor->Delete();
    }
}

//----------------------------------------------------------------------------
const char * vtkPythonProgrammableFilter::IntToDataSetTypeString(int i)
{
  switch (i)
    {
    case VTK_POLY_DATA: return "vtkPolyData";
    case VTK_STRUCTURED_POINTS: return "vtkStructuredPoints";
    case VTK_STRUCTURED_GRID: return "vtkStructuredGrid";
    case VTK_RECTILINEAR_GRID: return "vtkRectilinearGrid";
    case VTK_UNSTRUCTURED_GRID: return "vtkUnstructuredGrid";
    case VTK_PIECEWISE_FUNCTION: return "vtkPiecewiseFunction";
    case VTK_IMAGE_DATA: return "vtkImageData";
    case VTK_DATA_OBJECT: return "vtkDataObject";
    case VTK_DATA_SET: return "vtkDataSet";
    case VTK_POINT_SET: return "vtkPointSet";
    case VTK_UNIFORM_GRID: return "vtkUniformGrid";
    case VTK_COMPOSITE_DATA_SET: return "vtkCompositeDataSet";
    case VTK_MULTIGROUP_DATA_SET: return "vtkMultigroupDataSet";
    case VTK_MULTIBLOCK_DATA_SET: return "vtkMultiblockDataSet";
    case VTK_HIERARCHICAL_DATA_SET: return "vtkHierarchicalDataSet";
    case VTK_HIERARCHICAL_BOX_DATA_SET: return "vtkHierarchical_BoxDataSet";
    case VTK_GENERIC_DATA_SET: return "vtkGenericDataSet";
    case VTK_HYPER_OCTREE: return "vtkHyperOctree";
    case VTK_TEMPORAL_DATA_SET: return "vtkTemporalDataSet";
    case VTK_TABLE: return "vtkTable";
    case VTK_GRAPH: return "vtkGraph";
    case VTK_TREE: return "vtkTree";
    default: return "unknown";
    }
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::RequestDataObject(
  vtkInformation* inInfo, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  if (this->OutputDataSetType == VTK_DATA_SET)
    {
    return this->Superclass::RequestDataObject(
      inInfo, inputVector, outputVector
      );
    }

  const char *outTypeStr = 
    vtkPythonProgrammableFilter::IntToDataSetTypeString(
      this->OutputDataSetType
      );


  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataSet *output = vtkDataSet::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));
    
    if (!output || !output->IsA(outTypeStr)) 
      {
      vtkObject* anObj = vtkInstantiator::CreateInstance(outTypeStr);
      if (!anObj || !anObj->IsA(outTypeStr))
        {
        vtkErrorMacro("Could not create chosen output data type.");
        return 0;
        }
      vtkDataSet* newOutput = vtkDataSet::SafeDownCast(anObj);
      newOutput->SetPipelineInformation(info);
      newOutput->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::SetScript(const char *script)
{  
  if (script == NULL)
    {
    return;
    }

  if (this->Script != NULL)
    {
    delete[] this->Script;
    }
  
  int len = strlen(script) + 1;
  this->Script = new char[len];
  memcpy(this->Script, script, len-1);   
  this->Script[len-1] = 0;
  this->SetExecuteMethod(vtkPythonProgrammableFilter::ExecuteScript, this);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::ExecuteScript(void *arg)
{  
  vtkPythonProgrammableFilter *self = 
    static_cast<vtkPythonProgrammableFilter*>(arg);
  if (self != NULL)
    {
    self->Exec();
    }
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::Exec()
{
  if (this->Script == NULL)
    {
    return;
    }

  if (this->Interpretor == NULL)
    {
    this->Interpretor = vtkPVPythonInterpretor::New();
    const char* argv0 = vtkProcessModule::GetProcessModule()->
      GetOptions()->GetArgv0();
    this->Interpretor->InitializeSubInterpretor(1, (char**)&argv0);

    char addrofthis[1024];
    char *aplus = addrofthis+2; //skip over "0x"
    sprintf(addrofthis, "%p", this);    
    vtkstd::string initscript;
    initscript  = "";
    initscript += "import paraview;\n";
    initscript += "self = paraview.vtkProgrammableFilter('";
    initscript += aplus;
    initscript +=  "');\n";
    cerr << initscript << endl;
    this->Interpretor->MakeCurrent();
    this->Interpretor->RunSimpleString(initscript.c_str());
    }
  
  this->Interpretor->MakeCurrent();
  this->Interpretor->RunSimpleString(this->Script);
}


//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "OutputDataSetType: " << this->OutputDataSetType << endl;
}
