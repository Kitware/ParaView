// -*- c++ -*-
/*=========================================================================

   Program: ParaView
   Module:    pqOptions.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqOptions.h"

#include <vtkObjectFactory.h>
#include <vtkstd/string>

vtkStandardNewMacro(pqOptions);
vtkCxxRevisionMacro(pqOptions, "1.9");

static int AddTestScript(const char*, const char* value, void* call_data)
{
  pqOptions* self = reinterpret_cast<pqOptions*>(call_data);
  if (self)
    {
    return self->AddTestScript(value);
    }
  return 0;
}

static int AddTestBaseline(const char*, const char* value, void* call_data)
{
  pqOptions* self = reinterpret_cast<pqOptions*>(call_data);
  if (self)
    {
    return self->SetLastTestBaseline(value);
    }
  return 0;
}

static int AddTestImageThreshold(const char*, const char* value, void* call_data)
{
  pqOptions* self = reinterpret_cast<pqOptions*>(call_data);
  if (self)
    {
    return self->SetLastTestImageThreshold(QString(value).toInt());
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqOptions::pqOptions()
{
  this->BaselineImage = 0;
  this->TestDirectory = 0;
  this->DataDirectory = 0;
  this->ImageThreshold = 12;
  this->ExitAppWhenTestsDone = 0;
  this->DisableRegistry = 0;
  this->TestFileName = 0;
  this->TestInitFileName = 0;
  this->ServerResourceName = 0;
  this->DisableLightKit = 0;
}

//-----------------------------------------------------------------------------
pqOptions::~pqOptions()
{
  this->SetBaselineImage(0);
  this->SetTestDirectory(0);
  this->SetDataDirectory(0);
  this->SetTestFileName(0);
  this->SetTestInitFileName(0);
  this->SetServerResourceName(0);
}

//-----------------------------------------------------------------------------
void pqOptions::Initialize()
{
  this->Superclass::Initialize();
  
  this->AddArgument("--compare-view", NULL, 
    &this->BaselineImage,
    "Compare the viewport to a reference image, and exit.");
  
  this->AddArgument("--test-directory", NULL,
    &this->TestDirectory,
    "Set the temporary directory where test-case output will be stored.");
  
  this->AddArgument("--data-directory", NULL,
    &this->DataDirectory,
    "Set the data directory where test-case data are.");
 
  this->AddArgument("--run-test", NULL,
    &this->TestFileName,  "Run a recorded test case.");

  this->AddArgument("--run-test-init", NULL,
    &this->TestInitFileName,  "Run a recorded test initialization case.");
  
  this->AddArgument("--image-threshold", NULL, &this->ImageThreshold,
    "Set the threshold beyond which viewport-image comparisons fail.");

  this->AddBooleanArgument("--exit", NULL, &this->ExitAppWhenTestsDone,
    "Exit application when testing is done. Use for testing.");

  this->AddBooleanArgument("--disable-registry", "-dr", &this->DisableRegistry,
    "Do not use registry when running ParaView (for testing).");

  this->AddArgument("--server", "-s",
    &this->ServerResourceName,
    "Set the name of the server resource to connect with when the client starts.");

  this->AddBooleanArgument("--disable-light-kit", 0,
    &this->DisableLightKit,
    "When present, disables light kit by default. Useful for dashboard tests.");

  this->AddCallback("--test-script", NULL,
    &::AddTestScript, this, "Add test script. Can be used multiple times to "
    "specify multiple tests.");
  this->AddCallback("--test-baseline", NULL,
    &::AddTestBaseline, this,
    "Add test baseline. Can be used multiple times to specify "
    "multiple baselines for multiple tests, in order.");
  this->AddCallback("--test-threshold", NULL,
    &::AddTestImageThreshold, this,
    "Add test image threshold. "
    "Can be used multiple times to specify multiple image thresholds for "
    "multiple tests in order.");
}

//-----------------------------------------------------------------------------
int pqOptions::PostProcess(int argc, const char * const *argv)
{
  this->TestFiles.clear();
  if (this->TestInitFileName)
    {
    this->TestFiles << QString(this->TestInitFileName);
    }
  if (this->TestFileName)
    {
    this->TestFiles << QString(this->TestFileName);
    }
  return this->Superclass::PostProcess(argc, argv);
}

//-----------------------------------------------------------------------------
int pqOptions::WrongArgument(const char* arg)
{
  vtkstd::string argument = arg;
  int index = argument.find('=');
  if ( index != -1)
    {
    vtkstd::string key = argument.substr(0, index);
    vtkstd::string value = argument.substr(index+1);
    if (key == "--run-test")
      {
      this->TestFiles.push_back(value.c_str());
      return 1;
      }
    }
  return this->Superclass::WrongArgument(arg);
}

//-----------------------------------------------------------------------------
int pqOptions::AddTestScript(const char* script)
{
  TestInfo info;
  info.TestFile = script;
  this->TestScripts.push_back(info);
  return 1;
}
//-----------------------------------------------------------------------------
int pqOptions::SetLastTestBaseline(const char* image)
{
  if (this->TestScripts.size() > 0)
    {
    this->TestScripts.last().TestBaseline = image;
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int pqOptions::SetLastTestImageThreshold(int threshold)
{
  if (this->TestScripts.size() > 0)
    {
    this->TestScripts.last().ImageThreshold = threshold;
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ImageThreshold: " << this->ImageThreshold
    << endl;
  os << indent << "BaselineImage: " << (this->BaselineImage?
    this->BaselineImage : "(none)") << endl;
  os << indent << "TestDirectory: " << (this->TestDirectory?
    this->TestDirectory : "(none)") << endl;
  os << indent << "DataDirectory: " << (this->DataDirectory?
    this->DataDirectory : "(none)") << endl;

  os << indent << "ServerResourceName: " 
    << (this->ServerResourceName? this->ServerResourceName : "(none)") << endl;
}
