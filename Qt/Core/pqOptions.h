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
#include "vtkLegacy.h" // for VTK_LEGACY
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

  VTK_LEGACY(const char* GetBaselineDirectory());
  VTK_LEGACY(const char* GetTestDirectory());
  VTK_LEGACY(const char* GetDataDirectory());
  VTK_LEGACY(const char* GetStateFileName());
  VTK_LEGACY(const char* GetPythonScript());
  VTK_LEGACY(int GetExitAppWhenTestsDone());
  VTK_LEGACY(const char* GetServerResourceName());
  VTK_LEGACY(QStringList GetTestScripts());
  VTK_LEGACY(int GetNumberOfTestScripts());
  VTK_LEGACY(QString GetTestScript(int cc));
  VTK_LEGACY(QString GetTestBaseline(int cc));
  VTK_LEGACY(int GetTestImageThreshold(int cc));
  VTK_LEGACY(int GetCurrentImageThreshold());
  VTK_LEGACY(int GetTestMaster());
  VTK_LEGACY(int GetTestSlave());

protected:
  pqOptions();
  ~pqOptions() override;

private:
  pqOptions(const pqOptions&);
  void operator=(const pqOptions&);
};

#endif // pqOptions_h
