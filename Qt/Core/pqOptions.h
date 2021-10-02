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
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_10_0
#include <QStringList>
#include <vtkPVOptions.h>
/** \brief Command line options for pqClient.
 *
 * @deprecated in ParaView 5.10. See `pqCoreConfiguration`, `vtkCLIOptions`
 * instead.
 */
class PQCORE_EXPORT pqOptions : public vtkPVOptions
{
public:
  static pqOptions* New();
  vtkTypeMacro(pqOptions, vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetBaselineDirectory();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetTestDirectory();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetDataDirectory();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetStateFileName();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetPythonScript();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetExitAppWhenTestsDone();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetServerResourceName();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  QStringList GetTestScripts();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetNumberOfTestScripts();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  QString GetTestScript(int cc);
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  QString GetTestBaseline(int cc);
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetTestImageThreshold(int cc);
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetCurrentImageThreshold();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetTestMaster();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetTestSlave();

protected:
  pqOptions();
  ~pqOptions() override;

private:
  pqOptions(const pqOptions&);
  void operator=(const pqOptions&);
};

#endif // pqOptions_h
