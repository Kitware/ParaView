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

// Hide PARAVIEW_DEPRECATED_IN_5_10_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "pqOptions.h"

#include "pqCoreConfiguration.h"
#include "vtkLegacy.h"
#include "vtkRemotingCoreConfiguration.h"
#include <string>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(pqOptions);
//-----------------------------------------------------------------------------
pqOptions::pqOptions() = default;

//-----------------------------------------------------------------------------
pqOptions::~pqOptions() = default;

//-----------------------------------------------------------------------------
const char* pqOptions::GetBaselineDirectory()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetBaselineDirectory, "ParaView 5.10", pqCoreConfiguration::baselineDirectory);
  const auto& dir = pqCoreConfiguration::instance()->baselineDirectory();
  return dir.empty() ? nullptr : dir.c_str();
}

//-----------------------------------------------------------------------------
const char* pqOptions::GetTestDirectory()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetTestDirectory, "ParaView 5.10", pqCoreConfiguration::testDirectory);
  const auto& dir = pqCoreConfiguration::instance()->testDirectory();
  return dir.empty() ? nullptr : dir.c_str();
}

//-----------------------------------------------------------------------------
const char* pqOptions::GetDataDirectory()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetDataDirectory, "ParaView 5.10", pqCoreConfiguration::dataDirectory);
  const auto& dir = pqCoreConfiguration::instance()->dataDirectory();
  return dir.empty() ? nullptr : dir.c_str();
}

//-----------------------------------------------------------------------------
const char* pqOptions::GetStateFileName()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetStateFileName, "ParaView 5.10", pqCoreConfiguration::stateFileName);
  const auto& dir = pqCoreConfiguration::instance()->stateFileName();
  return dir.empty() ? nullptr : dir.c_str();
}

//-----------------------------------------------------------------------------
const char* pqOptions::GetPythonScript()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetPythonScript, "ParaView 5.10", pqCoreConfiguration::pythonScript);
  const auto& dir = pqCoreConfiguration::instance()->pythonScript();
  return dir.empty() ? nullptr : dir.c_str();
}

//-----------------------------------------------------------------------------
const char* pqOptions::GetServerResourceName()
{
  VTK_LEGACY_REPLACED_BODY(pqOptions::GetServerResourceName, "ParaView 5.10",
    vtkRemotingCoreConfiguration::GetServerResourceName);
  const auto& dir = vtkRemotingCoreConfiguration::GetInstance()->GetServerResourceName();
  return dir.empty() ? nullptr : dir.c_str();
}

//-----------------------------------------------------------------------------
int pqOptions::GetExitAppWhenTestsDone()
{
  VTK_LEGACY_REPLACED_BODY(pqOptions::GetExitAppWhenTestsDone, "ParaView 5.10",
    pqCoreConfiguration::exitApplicationWhenTestsDone);
  return pqCoreConfiguration::instance()->exitApplicationWhenTestsDone() ? 1 : 0;
}

//-----------------------------------------------------------------------------
QStringList pqOptions::GetTestScripts()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetTestScripts, "ParaView 5.10", pqCoreConfiguration::testScript);

  auto config = pqCoreConfiguration::instance();
  QStringList result;
  for (int cc = 0, max = config->testScriptCount(); cc < max; ++cc)
  {
    result.push_back(config->testScript(cc).c_str());
  }
  return result;
}

//-----------------------------------------------------------------------------
int pqOptions::GetNumberOfTestScripts()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetNumberOfTestScripts, "ParaView 5.10", pqCoreConfiguration::testScriptCount);
  return pqCoreConfiguration::instance()->testScriptCount();
}

//-----------------------------------------------------------------------------
QString pqOptions::GetTestScript(int cc)
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetTestScript, "ParaView 5.10", pqCoreConfiguration::testScript);
  return pqCoreConfiguration::instance()->testScript(cc).c_str();
}

//-----------------------------------------------------------------------------
QString pqOptions::GetTestBaseline(int cc)
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetTestBaseline, "ParaView 5.10", pqCoreConfiguration::testBaseline);
  return pqCoreConfiguration::instance()->testBaseline(cc).c_str();
}

//-----------------------------------------------------------------------------
int pqOptions::GetTestImageThreshold(int cc)
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetTestImageThreshold, "ParaView 5.10", pqCoreConfiguration::testThreshold);
  return pqCoreConfiguration::instance()->testThreshold(cc);
}

//-----------------------------------------------------------------------------
int pqOptions::GetCurrentImageThreshold()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetCurrentImageThreshold, "ParaView 5.10", pqCoreConfiguration::testThreshold);
  return pqCoreConfiguration::instance()->testThreshold();
}

//-----------------------------------------------------------------------------
int pqOptions::GetTestMaster()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetTestMaster, "ParaView 5.10", pqCoreConfiguration::testMaster);
  return pqCoreConfiguration::instance()->testMaster() ? 1 : 0;
}

//-----------------------------------------------------------------------------
int pqOptions::GetTestSlave()
{
  VTK_LEGACY_REPLACED_BODY(
    pqOptions::GetTestSlave, "ParaView 5.10", pqCoreConfiguration::testSlave);
  return pqCoreConfiguration::instance()->testSlave() ? 1 : 0;
}

//-----------------------------------------------------------------------------
void pqOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
