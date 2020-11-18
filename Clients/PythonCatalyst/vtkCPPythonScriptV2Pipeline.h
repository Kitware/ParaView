/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonScriptV2Pipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCPPythonScriptV2Pipeline
 * @brief vtkCPPipeline for Catalyst Python script / package version 2.0
 *
 * vtkCPPythonScriptV2Pipeline is intended to use with ParaView Python scripts
 * introduced in ParaView version 5.9. These Python scripts typically
 * use extract generators to define the extracts to be generated.
 * This class is used to use such packages in Catalyst.
 *
 * For details on the supported Python scripts and how to use them to write in
 * situ analysis, refer to
 * ['Anatomy of Catalyst Python Module (Version 2.0)'](@ref CatalystPythonScriptsV2).
 */

#ifndef vtkCPPythonScriptV2Pipeline_h
#define vtkCPPythonScriptV2Pipeline_h

#include "vtkCPPythonPipeline.h"
#include "vtkNew.h"                    // for vtkNew
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries

class vtkCPPythonScriptV2Helper;

class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonScriptV2Pipeline : public vtkCPPythonPipeline
{
public:
  static vtkCPPythonScriptV2Pipeline* New();
  vtkTypeMacro(vtkCPPythonScriptV2Pipeline, vtkCPPythonPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize from a zip archive. If the packagename is not specified, then
   * the it is assumed to be the same as the filename for the zip archive
   * without the extension.
   *
   * For HPC use-cases, this is the preferred approach since we ensure that only
   * the root node makes disk access thus avoid thrashing the IO subsystem and
   * improving load times.
   */
  bool Initialize(const char* filename);

  //@{
  /**
   * Implementation for vtkCPPipeline API
   */
  int RequestDataDescription(vtkCPDataDescription* dataDescription) override;
  int CoProcess(vtkCPDataDescription* dataDescription) override;
  int Finalize() override;
  //@}

protected:
  vtkCPPythonScriptV2Pipeline();
  ~vtkCPPythonScriptV2Pipeline();

private:
  vtkCPPythonScriptV2Pipeline(const vtkCPPythonScriptV2Pipeline&) = delete;
  void operator=(const vtkCPPythonScriptV2Pipeline&) = delete;

  vtkNew<vtkCPPythonScriptV2Helper> Helper;
  bool CoProcessHasBeenCalled;
};

#endif
