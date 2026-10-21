// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Tests how vtkTCPNetworkAccessManager analyzes a client handshake that differs
// from the server handshake. A subclass exposes the protected analysis, and the
// test checks the returned code and the reported warnings or errors. Versions that
// differ only in the nightly (third) version component are accepted with a
// warning; other mismatches (major.minor, connect id, rendering backend,
// unparsable handshake) are rejected.

#include "vtkObjectFactory.h"
#include "vtkTCPNetworkAccessManager.h"
#include "vtkTestErrorObserver.h"

#include <iostream>
#include <string>

namespace
{
class TestManager : public vtkTCPNetworkAccessManager
{
public:
  static TestManager* New();
  vtkTypeMacro(TestManager, vtkTCPNetworkAccessManager);

  using vtkTCPNetworkAccessManager::AnalyzeHandshakeAndGetErrorCode;
  using vtkTCPNetworkAccessManager::HANDSHAKE_DIFFERENT_BUILD_VERSIONS;
  using vtkTCPNetworkAccessManager::HANDSHAKE_DIFFERENT_CONNECTION_IDS;
  using vtkTCPNetworkAccessManager::HANDSHAKE_DIFFERENT_PV_VERSIONS;
  using vtkTCPNetworkAccessManager::HANDSHAKE_DIFFERENT_RENDERING_BACKENDS;
  using vtkTCPNetworkAccessManager::HANDSHAKE_UNKNOWN_ERROR;
};
vtkStandardNewMacro(TestManager);

bool Contains(const std::string& text, const std::string& expected)
{
  return expected.empty() || text.find(expected) != std::string::npos;
}

struct Case
{
  const char* Client;
  const char* Server;
  int ErrorCode;
  // Substrings expected in the output; empty means not checked.
  const char* Warning;
  const char* Error;
};
const Case cases[] = {
  // Nightly client, release server: accepted with a warning showing both versions.
  { "paraview-6.2.20261020.renderingbackend.opengl2", "paraview-6.2.renderingbackend.opengl2",
    TestManager::HANDSHAKE_DIFFERENT_BUILD_VERSIONS, "(client: 6.2.20261020, server: 6.2)", "" },
  // Release client, nightly server: same as above.
  { "paraview-6.2.renderingbackend.opengl2", "paraview-6.2.20261020.renderingbackend.opengl2",
    TestManager::HANDSHAKE_DIFFERENT_BUILD_VERSIONS, "(client: 6.2, server: 6.2.20261020)", "" },
  // Nightlies from different days: same as above.
  { "paraview-6.2.20261020.renderingbackend.opengl2",
    "paraview-6.2.20261019.renderingbackend.opengl2",
    TestManager::HANDSHAKE_DIFFERENT_BUILD_VERSIONS, "(client: 6.2.20261020, server: 6.2.20261019)",
    "" },
  // Different major.minor: rejected; the error shows both versions.
  { "paraview-6.3.20261020.renderingbackend.opengl2", "paraview-6.2.renderingbackend.opengl2",
    TestManager::HANDSHAKE_DIFFERENT_PV_VERSIONS, "",
    "Client reports version: 6.3.20261020 but server is version 6.2" },
  // Connect id mismatch checked before the nightly difference: rejected.
  { "paraview-6.2.20261020.connect_id.3.renderingbackend.opengl2",
    "paraview-6.2.renderingbackend.opengl2", TestManager::HANDSHAKE_DIFFERENT_CONNECTION_IDS, "",
    "connection ids do not match" },
  // Rendering backend mismatch checked before the nightly difference: rejected.
  { "paraview-6.2.20261020.renderingbackend.other", "paraview-6.2.renderingbackend.opengl2",
    TestManager::HANDSHAKE_DIFFERENT_RENDERING_BACKENDS, "", "different rendering backends" },
  // Unparsable client handshake: rejected; the error shows the raw handshakes.
  { "paraview-6.2.bad", "paraview-6.2.renderingbackend.opengl2",
    TestManager::HANDSHAKE_UNKNOWN_ERROR, "", "Client handshake: \"paraview-6.2.bad\"" },
};
}

extern int TestHandshake(int, char*[])
{
  vtkNew<TestManager> manager;
  vtkNew<vtkTest::ErrorObserver> observer;
  manager->AddObserver(vtkCommand::WarningEvent, observer);
  manager->AddObserver(vtkCommand::ErrorEvent, observer);

  bool success = true;
  for (const Case& c : cases)
  {
    observer->Clear();
    int errorCode = manager->AnalyzeHandshakeAndGetErrorCode(c.Client, c.Server);
    std::string warnings = observer->GetWarningMessage();
    std::string errors = observer->GetErrorMessage();
    if (errorCode != c.ErrorCode || !Contains(warnings, c.Warning) || !Contains(errors, c.Error))
    {
      std::cerr << "FAILED client " << c.Client << " server " << c.Server << ": code=" << errorCode
                << " (expected " << c.ErrorCode << ")\n  warnings: " << warnings
                << "\n  errors: " << errors << "\n  expected warning: \"" << c.Warning
                << "\"\n  expected error: \"" << c.Error << "\"" << std::endl;
      success = false;
    }
  }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
