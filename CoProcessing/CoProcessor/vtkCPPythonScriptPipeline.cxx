/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonScriptPipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPPythonScriptPipeline.h"

#include "CPSystemInformation.h"
#include "vtkCPDataDescription.h"
#include "vtkCPPythonHelper.h"
#include "vtkDataObject.h"
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVMain.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkSMObject.h"
#include "vtkSMXMLParser.h"

#include <vtkstd/string>
using vtkstd::string;
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>
using vtksys_ios::ostringstream;

vtkCPPythonHelper* vtkCPPythonScriptPipeline::PythonHelper = 0;

vtkCxxRevisionMacro(vtkCPPythonScriptPipeline, "1.1");
vtkStandardNewMacro(vtkCPPythonScriptPipeline);

//----------------------------------------------------------------------------
vtkCPPythonScriptPipeline::vtkCPPythonScriptPipeline()
{
  if(!vtkCPPythonScriptPipeline::PythonHelper)
    {
    this->PythonHelper = vtkCPPythonHelper::New();
    this->PythonHelper->Register(this);
    this->PythonHelper->Delete();
    }
  else
    {
    this->PythonHelper->Register(this);
    }
  this->PythonScriptName = 0;
}

//----------------------------------------------------------------------------
vtkCPPythonScriptPipeline::~vtkCPPythonScriptPipeline()
{
  this->PythonHelper->UnRegister(this);
  this->SetPythonScriptName(0);
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::Initialize(const char* FileName)
{
  if(vtksys::SystemTools::FileExists(FileName) == 0)
    {
    vtkErrorMacro("Could not find file " << FileName);
    return 0;
    }

  // for now do not check on filename extension:
  //vtksys::SystemTools::GetFilenameLastExtension(FileName) == ".py" == 0)

  string FilenamePath = vtksys::SystemTools::GetFilenamePath(FileName);
  string FilenameName = vtksys::SystemTools::GetFilenameWithoutExtension(
    vtksys::SystemTools::GetFilenameName(FileName));
  // need to save the script name as it is used as the name of the module
  this->SetPythonScriptName(FilenameName.c_str());

  ostringstream LoadPythonModules;
  LoadPythonModules
    << "sys.path.append('" << FilenamePath << "')\n"
    << "import " << FilenameName << "\n";

  this->PythonHelper->GetPythonInterpretor()->RunSimpleString(
    LoadPythonModules.str().c_str());
  this->PythonHelper->GetPythonInterpretor()->FlushMessages();
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::RequestDataDescription(
  vtkCPDataDescription* DataDescription)
{
  if(!DataDescription)
    {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
    }
  // check the script to see if it should be run...
  vtkStdString DataDescriptionString = this->GetPythonAddress(DataDescription);

  ostringstream PythonInput;
#ifndef COPROCESSOR_WIN32_BUILD
  // Not on Windows.
  PythonInput << "DataDescription = libvtkCoProcessorPython.vtkCPDataDescription('"
              << DataDescriptionString << "')\n"
              << this->PythonScriptName << ".RequestDataDescription(DataDescription)\n";
#else
  PythonInput << "DataDescription = vtkCoProcessorPython.vtkCPDataDescription('"
              << DataDescriptionString << "')\n"
              << this->PythonScriptName << ".RequestDataDescription(DataDescription)\n";
#endif

  this->PythonHelper->GetPythonInterpretor()->RunSimpleString(PythonInput.str().c_str());
  this->PythonHelper->GetPythonInterpretor()->FlushMessages();
  return DataDescription->GetIfAnyGridNecessary()? 1: 0;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptPipeline::CoProcess(
  vtkCPDataDescription* DataDescription)
{
  if(!DataDescription)
    {
    vtkWarningMacro("DataDescription is NULL.");
    return 0;
    }
  vtkStdString DataDescriptionString = this->GetPythonAddress(DataDescription);

  ostringstream PythonInput;
  PythonInput 
#ifndef COPROCESSOR_WIN32_BUILD
    // Not on Windows
    << "DataDescription = libvtkCoProcessorPython.vtkCPDataDescription('"
#else
    << "DataDescription = vtkCoProcessorPython.vtkCPDataDescription('"
#endif
    << DataDescriptionString << "')\n"
    << this->PythonScriptName << ".DoCoProcessing(DataDescription)\n";

  this->PythonHelper->GetPythonInterpretor()->RunSimpleString(
    PythonInput.str().c_str());
  this->PythonHelper->GetPythonInterpretor()->FlushMessages();
  return 1;  
}

//----------------------------------------------------------------------------
vtkStdString vtkCPPythonScriptPipeline::GetPythonAddress(void* Pointer)
{
  /*
  ostringstream ss;
  ss.setf(ios::hex,ios::basefield);
  ss.unsetf(ios::showbase);
  // below may be 32/64 bit dependent
  ss << reinterpret_cast<unsigned long long>(Pointer);
  return ss.str();
  */

  char AddressOfPointer[1024];
#ifdef COPROCESSOR_WIN32_BUILD
  sprintf_s(AddressOfPointer, "%p", Pointer);
#else
  sprintf(AddressOfPointer, "%p", Pointer);
#endif
  char *aplus = AddressOfPointer;
  if ((AddressOfPointer[0] == '0') && 
      ((AddressOfPointer[1] == 'x') || AddressOfPointer[1] == 'X'))
    {
    aplus += 2; //skip over "0x"
    }

  vtkstd::string Value = aplus;
  return Value;
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PythonHelper: " << this->PythonHelper << "\n";
  os << indent << "PythonScriptName: " << this->PythonScriptName << "\n";
}



