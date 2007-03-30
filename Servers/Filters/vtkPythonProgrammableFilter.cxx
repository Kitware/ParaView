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
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"

#include <vtkstd/string>

vtkCxxRevisionMacro(vtkPythonProgrammableFilter, "1.11");
vtkStandardNewMacro(vtkPythonProgrammableFilter);

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::vtkPythonProgrammableFilter()
{
  this->Script = NULL;
  this->InformationScript = NULL;
  this->Interpretor = NULL;
  this->OutputDataSetType = VTK_DATA_SET;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::UnRegister(vtkObjectBase *o)
{
  if (this->GetReferenceCount() == 4 && this->Interpretor != NULL)
    {
    this->Interpretor->MakeCurrent();
    this->Interpretor->RunSimpleString("self = 0");
    }
  this->Superclass::UnRegister(o);
}

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::~vtkPythonProgrammableFilter()
{
  if (this->Script != NULL)
    {
    delete[] this->Script;
    }
  this->SetInformationScript(NULL);

  if (this->Interpretor != NULL)
    {
    this->Interpretor->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::RequestInformation(
  vtkInformation*, 
  vtkInformationVector** , 
  vtkInformationVector*)
{
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
  if (this->Interpretor == NULL)
    {
    this->Interpretor = vtkPVPythonInterpretor::New();
    const char* argv0 = vtkProcessModule::GetProcessModule()->
      GetOptions()->GetArgv0();
    this->Interpretor->InitializeSubInterpretor(1, (char**)&argv0);

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
    this->Interpretor->MakeCurrent();
    this->Interpretor->RunSimpleString(initscript.c_str());
    }
  
  this->Interpretor->MakeCurrent();
  this->Interpretor->RunSimpleString(script);
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
