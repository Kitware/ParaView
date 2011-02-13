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
#include "vtkPythonProgrammableFilter.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPVOptions.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkProcessModule.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/string>

vtkStandardNewMacro(vtkPythonProgrammableFilter);

//----------------------------------------------------------------------------

typedef vtkstd::map<vtkstd::string, vtkstd::string> ParametersT;

class vtkPythonProgrammableFilterImplementation
{
public:
  // Stores name-value parameters that will be passed to running scripts
  ParametersT Parameters;
};

vtkPVPythonInterpretor* vtkPythonProgrammableFilter::GlobalPipelineInterpretor = 0;

// Upon disconnect, remove the global interp
class vtkPythonProgrammableFilterObserver : public vtkCommand
{
public:
  virtual void Execute(vtkObject*, unsigned long, void*)
    {
    if (vtkPythonProgrammableFilter::GlobalPipelineInterpretor)
      {
      vtkPythonProgrammableFilter::GlobalPipelineInterpretor->Delete();
      vtkPythonProgrammableFilter::GlobalPipelineInterpretor = 0;
      }
    }
};

//----------------------------------------------------------------------------
vtkPVPythonInterpretor* vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()
{
  if (!GlobalPipelineInterpretor)
    {
    vtkPythonProgrammableFilter::GlobalPipelineInterpretor = vtkPVPythonInterpretor::New();
    vtkPythonProgrammableFilter::GlobalPipelineInterpretor->SetCaptureStreams(true);
    const char* argv0 = vtkProcessModule::GetProcessModule()->
      GetOptions()->GetArgv0();
    vtkPythonProgrammableFilter::GlobalPipelineInterpretor->InitializeSubInterpretor(
      1, (char**)&argv0);
    vtkPythonProgrammableFilterObserver* obs = new vtkPythonProgrammableFilterObserver;
    vtkProcessModule::GetProcessModule()->AddObserver(vtkCommand::ExitEvent, obs);
    obs->UnRegister(0);
    }
  return vtkPythonProgrammableFilter::GlobalPipelineInterpretor;
}

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::vtkPythonProgrammableFilter() :
  Implementation(new vtkPythonProgrammableFilterImplementation())
{
  this->Script = NULL;
  this->InformationScript = NULL;
  this->UpdateExtentScript = NULL;
  this->PythonPath = 0;
  this->SetExecuteMethod(vtkPythonProgrammableFilter::ExecuteScript, this);
  this->OutputDataSetType = VTK_POLY_DATA;
}

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::~vtkPythonProgrammableFilter()
{
  this->SetScript(NULL);
  this->SetInformationScript(NULL);
  this->SetUpdateExtentScript(NULL);
  this->SetPythonPath(0);

  delete this->Implementation;
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  if (this->OutputDataSetType == VTK_DATA_SET)
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

  const char *outTypeStr =
    vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType);

  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA(outTypeStr))
      {
      vtkDataObject* newOutput =
        vtkDataObjectTypes::NewDataObject(this->OutputDataSetType);
      if (!newOutput)
        {
        vtkErrorMacro("Could not create chosen output data type: "
                      << outTypeStr);
        return 0;
        }
      newOutput->SetPipelineInformation(info);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::RequestInformation(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by the histogram filter.
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (strcmp(
      sddp->GetExtentTranslator(outInfo)->GetClassName(),
      "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    sddp->SetExtentTranslator(outInfo, et);
    et->Delete();
    }

  if (this->InformationScript)
    {
    this->Exec(this->InformationScript, "RequestInformation");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::RequestUpdateExtent(
  vtkInformation*request,
  vtkInformationVector**inputVector,
  vtkInformationVector* outputVector)
{
  if (this->UpdateExtentScript)
    {
    this->Exec(this->UpdateExtentScript, "RequestUpdateExtent");
    return 1;
    }

  return this->Superclass::RequestUpdateExtent(request,
                                              inputVector,outputVector);
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::SetParameter(const char *raw_name,
                                               const char *raw_value)
{
  const vtkstd::string name = raw_name ? raw_name : "";
  const vtkstd::string value = raw_value ? raw_value : "";

  if(name.empty())
    {
    vtkErrorMacro(<< "cannot set parameter with empty name");
    return;
    }

  this->Implementation->Parameters[name] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::ClearParameters()
{
  this->Implementation->Parameters.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::ExecuteScript(void *arg)
{
  vtkPythonProgrammableFilter *self =
    static_cast<vtkPythonProgrammableFilter*>(arg);
  if (self)
    {
    self->Exec(self->GetScript(), "RequestData");
    }
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::Exec(const char* script,
                                       const char* funcname)
{
  if (!script || !strlen(script))
    {
    return;
    }


  // Prepend the paths defined in PythonPath to sys.path
  if (this->PythonPath)
    {
    vtkstd::string pathscript;
    pathscript += "import sys\n";
    vtkstd::vector<vtksys::String> paths = vtksys::SystemTools::SplitString(
      this->PythonPath, ';');
    for (unsigned int cc=0; cc < static_cast<unsigned int>(paths.size()); cc++)
      {
      if (!paths[cc].empty())
        {
        pathscript += "if not ";
        pathscript += paths[cc];
        pathscript += " in sys.path:\n";
        pathscript += "  sys.path.insert(0, ";
        pathscript += paths[cc];
        pathscript += ")\n";
        vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->RunSimpleString(pathscript.c_str());
        }
      }
    }

  // Construct a script that defines a function
  vtkstd::string fscript;
  fscript  = "def ";
  fscript += funcname;

  // Set the parameters defined by user.
  fscript += "(self, inputs = None, output = None):\n";
  for(ParametersT::const_iterator parameter =
        this->Implementation->Parameters.begin();
      parameter != this->Implementation->Parameters.end();
      ++parameter)
    {
    fscript += "  " + parameter->first + " = " + parameter->second + "\n";
    }

  // Indent user script
  fscript += "  ";

  // Replace tabs with two spaces
  vtkstd::string orgscript;
  size_t len = strlen(script);
  for(size_t i=0; i< len; i++)
    {
    if (script[i] == '\t')
      {
      orgscript += "  ";
      }
    else
      {
      orgscript.push_back(script[i]);
      }
    }
  // Remove DOS line endings. They confuse the indentation code below.
  orgscript.erase(
    vtkstd::remove(orgscript.begin(), orgscript.end(), '\r'), orgscript.end());

  vtkstd::string::iterator it = orgscript.begin();
  for(; it != orgscript.end(); it++)
    {
    fscript += *it;
    // indent new lines
    if (*it == '\n')
      {
      fscript += "  ";
      }
    }
  fscript += "\n";
  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->RunSimpleString(fscript.c_str());

  vtkstd::string runscript;

  runscript += "import paraview\n";
  runscript += "paraview.fromFilter = True\n";
  runscript += "from paraview import vtk\n";
  runscript += "from paraview import vtk\n";
  runscript += "from paraview import servermanager\n";
  runscript += "if servermanager.progressObserverTag:\n";
  runscript += "  servermanager.ToggleProgressPrinting()\n";
  runscript += "hasnumpy = True\n";
  runscript += "try:\n";
  runscript += "  from numpy import *\n";
  runscript += "except ImportError:\n";
  runscript += "  hasnumpy = False\n";
  runscript += "if hasnumpy:\n";
  runscript += "  from paraview.vtk import dataset_adapter\n";
  runscript += "  from paraview.vtk.algorithms import *\n";

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
  runscript += "if hasnumpy:\n";
  runscript += "  inputs = []\n";
  runscript += "  index = 0\n";
  int numinps = this->GetNumberOfInputConnections(0);
  for (int i=0; i<numinps; i++)
    {
    runscript +=
      "  inputs.append(dataset_adapter.WrapDataObject(myarg.GetInputDataObject(0, index)))\n";
    runscript += "  index += 1\n";
    }
  runscript += "  output = dataset_adapter.WrapDataObject(myarg.GetOutputDataObject(0))\n";
  runscript += "else:\n";
  runscript += "  inputs = None\n";
  runscript += "  output = None\n";

  // Call the function
  runscript += funcname;
  runscript += "(myarg, inputs, output)\n";
  runscript += "del inputs\n";
  runscript += "del output\n";
  runscript += "del myarg\n";
  runscript += "import gc\n";
  runscript += "gc.collect()\n";

  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->RunSimpleString(runscript.c_str());
  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->FlushMessages();
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  if(port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "OutputDataSetType: " << this->OutputDataSetType << endl;
  os << indent << "PythonPath: "
    << (this->PythonPath? this->PythonPath : "(none)") << endl;
}
