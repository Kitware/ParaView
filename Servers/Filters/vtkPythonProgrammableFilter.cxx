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

#include <vtkstd/map>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkPythonProgrammableFilter, "1.19");
vtkStandardNewMacro(vtkPythonProgrammableFilter);

//----------------------------------------------------------------------------

typedef vtkstd::map<vtkstd::string, vtkstd::string> ParametersT;

class vtkPythonProgrammableFilterImplementation
{
public:
  vtkPythonProgrammableFilterImplementation() :
    Running(0),
    Interpretor(NULL)
  {
  this->Parameters["foo"] = "['bar', 'baz']";
  this->Parameters["bleh"] = "'blah'";
  }

  //state used to get by a reference counting cyclic loop
  int Running;
  vtkPVPythonInterpretor* Interpretor;
  
  // Stores name-value parameters that will be passed to running scripts
  ParametersT Parameters;
};

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::vtkPythonProgrammableFilter() :
  Implementation(new vtkPythonProgrammableFilterImplementation())
{
  this->Script = NULL;
  this->InformationScript = NULL;
  this->OutputDataSetType = VTK_DATA_SET;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::UnRegister(vtkObjectBase *o)
{
  this->Superclass::UnRegister(o);
  if (this->GetReferenceCount() == 4 && 
      this->Implementation->Interpretor != NULL &&
      !this->Implementation->Running
    )
    {
    vtkPVPythonInterpretor *cpy = this->Implementation->Interpretor;
    vtkstd::string script;
    script  = "";
    script += "self = 0\n";
    cpy->MakeCurrent();
    cpy->RunSimpleString(script.c_str());
    cpy->ReleaseControl();
    this->Implementation->Interpretor = NULL;
    cpy->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::~vtkPythonProgrammableFilter()
{
  if (this->Script != NULL)
    {
    delete[] this->Script;
    }
  this->SetInformationScript(NULL);

  if (this->Implementation->Interpretor != NULL)
    {
    this->Implementation->Interpretor->Delete();
    }
    
  delete this->Implementation;
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
    this->Exec(this->InformationScript);    
    }
  return 1;
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
void vtkPythonProgrammableFilter::SetParameter(const char *raw_name, const char *raw_value)
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
  if (self != NULL)
    {
    self->Exec();
    }
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::Exec(const char* script)
{
  if (!script || !strlen(script))
    {
    return;
    }

  this->Implementation->Running = 1;
  if (this->Implementation->Interpretor == NULL)
    {
    this->Implementation->Interpretor = vtkPVPythonInterpretor::New();
    const char* argv0 = vtkProcessModule::GetProcessModule()->
      GetOptions()->GetArgv0();
    this->Implementation->Interpretor->InitializeSubInterpretor(1, (char**)&argv0);

    char addrofthis[1024];
    sprintf(addrofthis, "%p", this);    
    char *aplus = addrofthis; 
    if ((addrofthis[0] == '0') && ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
      {
      aplus += 2; //skip over "0x"
      }
    vtkstd::string initscript;
    initscript  = "";
    initscript += "import paraview;\n";
    initscript += "self = paraview.vtkProgrammableFilter('";
    initscript += aplus;
    initscript +=  "');\n";
    
    for(
      ParametersT::const_iterator parameter = this->Implementation->Parameters.begin();
      parameter != this->Implementation->Parameters.end();
      ++parameter)
      {
      initscript += parameter->first + " = " + parameter->second + "\n";
      } 
    
    this->Implementation->Interpretor->MakeCurrent();
    this->Implementation->Interpretor->RunSimpleString(initscript.c_str());
    }
  
  this->Implementation->Interpretor->MakeCurrent();
  this->Implementation->Interpretor->RunSimpleString(script);
  this->Implementation->Interpretor->ReleaseControl();
  this->Implementation->Running = 0;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::Exec()
{
  if (this->Script == NULL)
    {
    return;
    }
  this->Exec(this->Script);
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
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(),1);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "OutputDataSetType: " << this->OutputDataSetType << endl;
}
