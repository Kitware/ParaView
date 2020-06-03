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
 * introduced in ParaView version 5.9. These Python scripts use extract
 * generators to define the extracts to be generated. The scripts that define
 * the analysis pipeline(s) are bundled in a Python package. This class is used
 * to use such packages in Catalyst.
 *
 */

#ifndef vtkCPPythonScriptV2Pipeline_h
#define vtkCPPythonScriptV2Pipeline_h

#include "vtkCPPythonPipeline.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries

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
  bool InitializeFromZIP(const char* zipfilename, const char* packagename = nullptr);

  /**
   * Use this method to initialize from a package that is not in a zip archive.
   * This is primarily intended for debugging purposes. `InitializeFromZIP` is
   * the preferred way.
   *
   * `path` is an absolute path to the Python package.
   */
  bool InitializeFromDirectory(const char* path);

  /**
   * Use this method to load a .py file. These files are generally used only for
   * very simple analysis scripts which do not any analysis pipeline put just
   * want to support Live, for example.
   */
  bool InitializeFromScript(const char* pyfilename);

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

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
