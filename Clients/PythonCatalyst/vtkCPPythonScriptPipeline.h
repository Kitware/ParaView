/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonScriptPipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPPythonScriptPipeline_h
#define vtkCPPythonScriptPipeline_h

#include "vtkCPPythonPipeline.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries
#include "vtkStdString.h"              // for the string

class vtkCPDataDescription;

/// @ingroup CoProcessing
/// Class that creates a coprocessing pipeline starting from a coprocessing
/// script.  This class only does operations with respect to the script
/// and uses the name of the script as the module to hide its definitions
/// from other python modules.
class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonScriptPipeline : public vtkCPPythonPipeline
{
public:
  static vtkCPPythonScriptPipeline* New();
  vtkTypeMacro(vtkCPPythonScriptPipeline, vtkCPPythonPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Initialize this pipeline from given the file name of a
  /// python script. Returns 1 for success and 0 for failure.
  int Initialize(const char* fileName);

  /// Configuration Step:
  /// The coprocessor first determines if any coprocessing needs to be done
  /// at this TimeStep/Time combination returning 1 if it does and 0
  /// otherwise.  If coprocessing does need to be performed this time step
  /// it fills in the FieldNames array that the coprocessor requires
  /// in order to fulfill all the coprocessing requests for this
  /// TimeStep/Time combination.
  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription) override;

  /// Execute the pipeline. Returns 1 for success and 0 for failure.
  virtual int CoProcess(vtkCPDataDescription* dataDescription) override;

  /// Finalize the pipeline before deleting it. A default no-op implementation
  /// is given. Returns 1 for success and 0 for failure.
  virtual int Finalize() override;

protected:
  vtkCPPythonScriptPipeline();
  virtual ~vtkCPPythonScriptPipeline();

  /// Set/get macro functinos for setting PythonScriptName.
  vtkSetStringMacro(PythonScriptName);
  vtkGetStringMacro(PythonScriptName);

private:
  vtkCPPythonScriptPipeline(const vtkCPPythonScriptPipeline&) = delete;
  void operator=(const vtkCPPythonScriptPipeline&) = delete;

  /// The name of the python script (without the path or extension)
  /// that is used as the namespace of the functions of the script.
  char* PythonScriptName;
};

#endif
