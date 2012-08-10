/*=========================================================================
  
  Program:   ParaView
  Module:    vtkTestingOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestingOptions.h"

#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTestingOptions);

//----------------------------------------------------------------------------
vtkTestingOptions::vtkTestingOptions()
{
  this->SetProcessType(vtkPVOptions::PVBATCH);
  this->ServerMode = 0;
  this->DataDir = 0;
  this->TempDir = 0;
  this->SMStateXMLName = 0;
  this->BaselineImage = 0;
  this->Threshold = 10.0;
}

//----------------------------------------------------------------------------
vtkTestingOptions::~vtkTestingOptions()
{
  this->SetSMStateXMLName(0);
  this->SetDataDir(0);
  this->SetTempDir(0);
  this->SetBaselineImage(0);
}

//----------------------------------------------------------------------------
void vtkTestingOptions::Initialize()
{
  this->Superclass::Initialize();
  this->AddArgument("--data-dir", "-D", &this->DataDir,
    "Path to the data dir.");
  this->AddArgument("--temp-dir", "-T", &this->TempDir,
    "Path to temporary directory.");
  this->AddArgument("--baseline-image", "-V", &this->BaselineImage,
    "Path to baseline image");
  //this->AddArgument("--threshold", 0, &this->Threshold,
  //  "Threshold for regression testing.");
}

//----------------------------------------------------------------------------
int vtkTestingOptions::PostProcess(int argc, const char* const* argv)
{
  if ( this->SMStateXMLName && 
    vtksys::SystemTools::GetFilenameLastExtension(this->SMStateXMLName) != ".pvsm")
    {
    vtksys_ios::ostringstream str;
    str << "Wrong state xml name: " << this->SMStateXMLName << ends;
    this->SetErrorMessage(str.str().c_str());
    return 0;
    }
  return this->Superclass::PostProcess(argc, argv);
}

//----------------------------------------------------------------------------
int vtkTestingOptions::WrongArgument(const char* argument)
{
  if ( vtksys::SystemTools::FileExists(argument) &&
    vtksys::SystemTools::GetFilenameLastExtension(argument) == ".pvsm")
    {
    this->SetSMStateXMLName(argument);
    return 1;
    }

  return this->Superclass::WrongArgument(argument);
}

//----------------------------------------------------------------------------
void vtkTestingOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

