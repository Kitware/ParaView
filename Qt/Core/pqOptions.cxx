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

#include <string>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(pqOptions);

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
  this->BaselineDirectory = 0;
  this->TestDirectory = 0;
  this->DataDirectory = 0;
  this->ExitAppWhenTestsDone = 0;
  this->ServerResourceName = 0;
  this->CurrentImageThreshold = 12;
  this->PythonScript = 0;
  this->TestMaster = 0;
  this->TestSlave = 0;
  this->TileImagePath = 0;
}

//-----------------------------------------------------------------------------
pqOptions::~pqOptions()
{
  this->SetBaselineDirectory(0);
  this->SetTestDirectory(0);
  this->SetDataDirectory(0);
  this->SetServerResourceName(0);
  this->SetPythonScript(0);
  this->SetTileImagePath(0);
}

//-----------------------------------------------------------------------------
void pqOptions::Initialize()
{
  this->Superclass::Initialize();

  this->AddArgument("--baseline-directory", NULL, &this->TestDirectory,
    "Set the baseline directory where test recorder will store baseline images.");

  this->AddArgument("--test-directory", NULL, &this->TestDirectory,
    "Set the temporary directory where test-case output will be stored.");

  this->AddArgument("--tile-image-prefix", NULL, &this->TileImagePath,
    "Set the temporary directory with file name prefix for the tile display image dump.");

  this->AddArgument("--data-directory", NULL, &this->DataDirectory,
    "Set the data directory where test-case data are located.");

  this->AddBooleanArgument("--exit", NULL, &this->ExitAppWhenTestsDone,
    "Exit application when testing is done. Use for testing.");

  this->AddArgument("--server", "-s", &this->ServerResourceName,
    "Set the name of the server resource to connect to when the client starts.");

  this->AddCallback("--test-script", NULL, &::AddTestScript, this,
    "Add test script. Can be used multiple times to "
    "specify multiple tests.");
  this->AddCallback("--test-baseline", NULL, &::AddTestBaseline, this,
    "Add test baseline. Can be used multiple times to specify "
    "multiple baselines for multiple tests, in order.");
  this->AddCallback("--test-threshold", NULL, &::AddTestImageThreshold, this,
    "Add test image threshold. "
    "Can be used multiple times to specify multiple image thresholds for "
    "multiple tests in order. When recording test can be a directory to record image in");

  this->AddArgument(
    "--script", NULL, &this->PythonScript, "Set a python script to be evaluated on startup.");

  this->AddBooleanArgument("--test-master", 0, &this->TestMaster,
    "(For testing) When present, tests master configuration.");

  this->AddBooleanArgument(
    "--test-slave", 0, &this->TestSlave, "(For testing) When present, tests slave configuration.");
}

//-----------------------------------------------------------------------------
QStringList pqOptions::GetTestScripts()
{
  QStringList list;
  for (int cc = 0; cc < this->GetNumberOfTestScripts(); cc++)
  {
    list << this->GetTestScript(cc);
  }
  return list;
}

//-----------------------------------------------------------------------------
int pqOptions::PostProcess(int argc, const char* const* argv)
{
  return this->Superclass::PostProcess(argc, argv);
}

//-----------------------------------------------------------------------------
int pqOptions::WrongArgument(const char* arg)
{
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
  if (this->TestScripts.size() == 0)
  {
    this->AddTestScript("-not-specified");
  }
  this->TestScripts.last().TestBaseline = image;
  return 1;
}

//-----------------------------------------------------------------------------
int pqOptions::SetLastTestImageThreshold(int threshold)
{
  if (this->TestScripts.size() == 0)
  {
    this->AddTestScript("-not-specified");
  }
  this->TestScripts.last().ImageThreshold = threshold;
  return 1;
}

//-----------------------------------------------------------------------------
void pqOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TestDirectory: " << (this->TestDirectory ? this->TestDirectory : "(none)")
     << endl;
  os << indent << "DataDirectory: " << (this->DataDirectory ? this->DataDirectory : "(none)")
     << endl;

  os << indent
     << "ServerResourceName: " << (this->ServerResourceName ? this->ServerResourceName : "(none)")
     << endl;

  os << indent << "PythonScript: " << (this->PythonScript ? this->PythonScript : "(none)") << endl;
}
