/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonStringPipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPPythonStringPipeline_h
#define vtkCPPythonStringPipeline_h

#include "vtkCPPythonPipeline.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries
#include <string>                      // For data member

class vtkCPDataDescription;

/// @ingroup CoProcessing
/// Class that creates a coprocessing pipeline from a coprocessing
/// string.
class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonStringPipeline : public vtkCPPythonPipeline
{
public:
  static vtkCPPythonStringPipeline* New();
  vtkTypeMacro(vtkCPPythonStringPipeline, vtkCPPythonPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Initialize this pipeline from a given Python string
  /// script. The string is stored in PythonString.
  /// Returns 1 for success and 0 for failure.
  int Initialize(const char* pythonString);

  /// Configuration Step:
  /// The coprocessor first determines if any coprocessing needs to be done
  /// at this TimeStep/Time combination returning 1 if it does and 0
  /// otherwise.  If coprocessing does need to be performed this time step
  /// it fills in the FieldNames array that the coprocessor requires
  /// in order to fulfill all the coprocessing requests for this
  /// TimeStep/Time combination.
  int RequestDataDescription(vtkCPDataDescription* dataDescription) override;

  /// Execute the pipeline. Returns 1 for success and 0 for failure.
  int CoProcess(vtkCPDataDescription* dataDescription) override;

  /// Finalize the pipeline before deleting it. A default no-op implementation
  /// is given. Returns 1 for success and 0 for failure.
  int Finalize() override;

protected:
  vtkCPPythonStringPipeline();
  ~vtkCPPythonStringPipeline() override;

private:
  vtkCPPythonStringPipeline(const vtkCPPythonStringPipeline&) = delete;
  void operator=(const vtkCPPythonStringPipeline&) = delete;

  /// The internally stored name of the module for the Python string.
  std::string ModuleName;
};
#endif
