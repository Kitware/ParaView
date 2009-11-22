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

#ifndef __pqOptions_h
#define __pqOptions_h

#include "pqCoreExport.h"
#include <vtkPVOptions.h>
#include <QStringList>

/*! \brief Command line options for pqClient.
 *
 * pqOptions extends vtkPVOptions to handle pqClient specific command line 
 * options.
 */
class PQCORE_EXPORT pqOptions : public vtkPVOptions
{
public:
  static pqOptions *New();
  vtkTypeRevisionMacro(pqOptions, vtkPVOptions);
  void PrintSelf(ostream &os, vtkIndent indent);

  vtkGetStringMacro(TestDirectory);
  vtkGetStringMacro(DataDirectory);

  /// DEPRECATED.
  /// @deprecated Use GetTestScript(int)/GetTestBaseline(int)/
  /// GetTestImageThreshold(int) instead which allows for providing
  /// multiples tests/baselines/thresholds on  the command line.
  VTK_LEGACY(vtkGetStringMacro(BaselineImage));
  VTK_LEGACY(vtkGetMacro(ImageThreshold, int));
  VTK_LEGACY(vtkSetMacro(ImageThreshold, int));

  vtkGetMacro(ExitAppWhenTestsDone, int);
  vtkGetMacro(DisableRegistry, int);
 
  /// DEPRECATED.
  /// @deprecated Use GetTestScript(int)/GetTestBaseline(int)/
  /// GetTestImageThreshold(int) instead which allows for providing
  /// multiples tests/baselines/thresholds on  the command line.
  const QStringList& GetTestFiles() 
    { return this->TestFiles; }

  /// Returns the server resource name specified
  /// to load.
  vtkGetStringMacro(ServerResourceName);

  vtkSetStringMacro(TestDirectory);
  vtkSetStringMacro(DataDirectory);
  vtkSetStringMacro(BaselineImage);
  vtkSetStringMacro(TestFileName);
  vtkSetStringMacro(TestInitFileName);
  vtkSetStringMacro(ServerResourceName);

  int GetNumberOfTestScripts()
    { return this->TestScripts.size(); }
  QString GetTestScript(int cc)
    { return this->TestScripts[cc].TestFile; }
  QString GetTestBaseline(int cc)
    { return this->TestScripts[cc].TestBaseline; }
  int GetTestImageThreshold(int cc)
    { return this->TestScripts[cc].ImageThreshold; }

  // Description
  // Get/Set whether lightkit is disabled by default. This is useful for
  // testing.
  vtkGetMacro(DisableLightKit, int);
  vtkSetMacro(DisableLightKit, int);

  // DO NOT CALL. Public for internal callbacks.
  int AddTestScript(const char*);
  int SetLastTestBaseline(const char*);
  int SetLastTestImageThreshold(int);

protected:
  pqOptions();
  virtual ~pqOptions();

  virtual void Initialize();
  virtual int PostProcess(int argc, const char * const *argv);

  char* TestDirectory;
  char* DataDirectory;
  char* BaselineImage;
  char* TestFileName;
  char* TestInitFileName;
  char* ServerResourceName;
  int ImageThreshold;
  int ExitAppWhenTestsDone;
  int DisableRegistry;
  int DisableLightKit;

  struct TestInfo
    {
    QString TestFile;
    QString TestBaseline;
    int ImageThreshold;
    TestInfo():ImageThreshold(12) { }
    };

  QList<TestInfo> TestScripts;

  QStringList TestFiles;
    
 
  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int WrongArgument(const char* argument);
private:
  pqOptions(const pqOptions &);
  void operator=(const pqOptions &);
};

#endif //__pqOptions_h

