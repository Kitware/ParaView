/*=========================================================================

   Program: ParaView
   Module:  pqCoreConfiguration.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqCoreConfiguration_h
#define pqCoreConfiguration_h

#include "pqCoreModule.h" // for exports
#include <QObject>
#include <QVector>
#include <string>
#include <vector>

class vtkCLIOptions;

/**
 * @class pqCoreConfiguration
 * @brief runtime configuration options for ParaView Qt client
 *
 * pqCoreConfiguration is a singleton that maintains runtime configuration
 * options for the ParaView Qt client.
 *
 * @sa vtkCLIOptions, vtkRemotingCoreConfiguration,
 * vtkProcessModuleConfiguration
 */
class PQCORE_EXPORT pqCoreConfiguration
{
public:
  /**
   * Provides access to the singleton.
   */
  static pqCoreConfiguration* instance();

  /**
   * Returns state file to load on startup, if any.
   */
  const std::string& stateFileName() const { return this->StateFileName; }

  /**
   * Returns data file to load on startup, if any.
   */
  const std::vector<std::string>& dataFileNames() const { return this->DataFileNames; }

  /**
   * Returns the Python script to load on startup, if any.
   */
  const std::string& pythonScript() const { return this->PythonScript; }

  /**
   * Returns directory where to output test results and temporary files, if any
   * else an empty string is returned.
   */
  const std::string& baselineDirectory() const { return this->BaselineDirectory; }

  /**
   * Returns directory where to output test results and temporary files, if any
   * else an empty string is returned.
   */
  const std::string& testDirectory() const { return this->TestDirectory; }

  /**
   * Returns directory containing test data files, if any, else an empty string
   * is returned.
   */
  const std::string& dataDirectory() const { return this->DataDirectory; }

  ///@{
  /**
   * Returns information about tests scripts.
   */
  int testScriptCount() const { return this->TestScripts.size(); }
  const std::string& testScript(int index) const { return this->TestScripts.at(index).FileName; }
  const std::string& testBaseline(int index) const { return this->TestScripts.at(index).Baseline; }
  int testThreshold(int index) const { return this->TestScripts.at(index).Threshold; }
  ///@}

  /**
   * Returns true if the application should exit after test playback is
   * complete.
   */
  bool exitApplicationWhenTestsDone() const { return this->ExitAppWhenTestsDone; }

  /**
   * When specified, ParaView will attempt to connect a Catalyst Live session at
   * the given port.
   */
  int catalystLivePort() const { return this->CatalystLivePort; }

  /**
   * Populate vtkCLIOptions instance with command line options to control the
   * configurable options provided by this class.
   */
  bool populateOptions(vtkCLIOptions* options);

  ///@{
  /**
   * A little bit of hack to activate a particular test script.
   */
  void setActiveTestIndex(int index) { this->ActiveTestIndex = index; }
  const std::string& testScript() const { return this->testScript(this->ActiveTestIndex); }
  const std::string& testBaseline() const { return this->testBaseline(this->ActiveTestIndex); }
  int testThreshold() const { return this->testThreshold(this->ActiveTestIndex); }
  ///@}

  ///@{
  /**
   * Collaboration testing related flags.
   */
  bool testMaster() const { return this->TestMaster; }
  bool testSlave() const { return this->TestSlave; }
  ///@}

protected:
  pqCoreConfiguration();
  ~pqCoreConfiguration();

  ///@{
  // not entirely sure why this is being done, but I'll let it be fore now.
  // pqPVApplicationCore uses this.
  friend class pqPVApplicationCore;
  void addDataFile(const std::string& data) { this->DataFileNames.push_back(data); }
  ///@}

private:
  Q_DISABLE_COPY(pqCoreConfiguration);

  std::string StateFileName;
  std::vector<std::string> DataFileNames;
  std::vector<std::string> PositionalFileNames;
  std::string PythonScript;
  std::string BaselineDirectory;
  std::string TestDirectory;
  std::string DataDirectory;
  bool ExitAppWhenTestsDone;
  int ActiveTestIndex = 0;
  int CatalystLivePort = -1;
  bool TestMaster = false;
  bool TestSlave = false;

  struct TestScriptInfo
  {
    std::string FileName;
    std::string Baseline;
    int Threshold = 12;
  };
  QVector<TestScriptInfo> TestScripts;
};

#endif
