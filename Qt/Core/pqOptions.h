/*=========================================================================

   Program: ParaView
   Module:    pqOptions.h

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

#ifndef pqOptions_h
#define pqOptions_h

#include "pqCoreModule.h"
#include <QStringList>
#include <vtkPVOptions.h>

/** \brief Command line options for pqClient.
 *
 * pqOptions extends vtkPVOptions to handle pqClient specific command line
 * options.
 */
class PQCORE_EXPORT pqOptions : public vtkPVOptions
{
public:
  static pqOptions* New();
  vtkTypeMacro(pqOptions, vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetStringMacro(BaselineDirectory);
  vtkGetStringMacro(TestDirectory);
  vtkGetStringMacro(DataDirectory);

  //@{
  /**
   * State file to load on startup.
   */
  // See Bug #5711
  vtkGetStringMacro(StateFileName);
  //@}

  vtkGetMacro(ExitAppWhenTestsDone, int);

  // Returns the test scripts as a list.
  QStringList GetTestScripts();

  /**
  * Returns the server resource name specified
  * to load.
  */
  vtkGetStringMacro(ServerResourceName);

  vtkSetStringMacro(BaselineDirectory);
  vtkSetStringMacro(TestDirectory);
  vtkSetStringMacro(DataDirectory);

  int GetNumberOfTestScripts() { return this->TestScripts.size(); }
  QString GetTestScript(int cc) { return this->TestScripts[cc].TestFile; }
  QString GetTestBaseline(int cc) { return this->TestScripts[cc].TestBaseline; }
  int GetTestImageThreshold(int cc) { return this->TestScripts[cc].ImageThreshold; }

  /**
  * HACK: When playing back tests, this variable is set to make it easier to locate
  * the test image threshold for the current test. This is updated by the
  * test playback code.
  */
  vtkSetMacro(CurrentImageThreshold, int);
  vtkGetMacro(CurrentImageThreshold, int);

  // Description:
  // These flags are used for testing multi-clients configurations.
  vtkGetMacro(TestMaster, int);
  vtkGetMacro(TestSlave, int);

  // Description:
  // Using --script option, user can specify a python script to be run on
  // startup. This have any effect only when ParaView is built with Python
  // support.
  vtkGetStringMacro(PythonScript);

  // DO NOT CALL. Public for internal callbacks.
  int AddTestScript(const char*);
  int SetLastTestBaseline(const char*);
  int SetLastTestImageThreshold(int);

protected:
  pqOptions();
  ~pqOptions() override;

  void Initialize() override;
  int PostProcess(int argc, const char* const* argv) override;

  char* BaselineDirectory;
  char* TestDirectory;
  char* DataDirectory;
  char* ServerResourceName;
  char* StateFileName; // loading state file(Bug #5711)

  int ExitAppWhenTestsDone;
  int DisableRegistry;
  int CurrentImageThreshold;
  int TestMaster;
  int TestSlave;
  char* PythonScript;

  vtkSetStringMacro(PythonScript);
  vtkSetStringMacro(ServerResourceName);
  vtkSetStringMacro(StateFileName);

  struct TestInfo
  {
    QString TestFile;
    QString TestBaseline;
    int ImageThreshold;
    TestInfo()
      : ImageThreshold(12)
    {
    }
  };

  QList<TestInfo> TestScripts;

  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  int WrongArgument(const char* argument) override;

private:
  pqOptions(const pqOptions&);
  void operator=(const pqOptions&);
};

#endif // pqOptions_h
