/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonPipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPPythonPipeline_h
#define vtkCPPythonPipeline_h

#include "vtkCPPipeline.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries
#include "vtkSmartPointer.h"           // for vtkSmartPointer
#include <string>                      // For member function use

class vtkCPDataDescription;

/// @ingroup CoProcessing
/// Abstract class that takes care of initializing Catalyst Python
/// pipelines for all concrete implementations and adds in some
/// useful helper methods.
class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonPipeline : public vtkCPPipeline
{
public:
  vtkTypeMacro(vtkCPPythonPipeline, vtkCPPipeline);

  /**
   * Starting with ParaView 5.9, there are two versions of Python scripts
   * that ParaView supports. Use this method to detect which version it is.
   * Returns 1, 2 to indicate script version or 0 on failure.
   */
  static int DetectScriptVersion(const char* fname);

  /**
   * Detects the script version, if possible and created appropriate subclass.
   * If the version cannot be determined, the `default_version` is assumed.
   *
   * @sa `CreateAndInitializePipeline`.
   */
  static vtkSmartPointer<vtkCPPythonPipeline> CreatePipeline(
    const char* fname, int default_version = 1);

  /**
   * Same as `CreatePipeline`, except that if the pipeline instance is successfully
   * created also calls appropriate `Initialize` method on it.
   *
   * If the Initialize failed, this will return nullptr.
   */
  static vtkSmartPointer<vtkCPPythonPipeline> CreateAndInitializePipeline(
    const char* fname, int default_version = 1);

  //@{
  /**
   * These overloads are provided for Python wrapping since `vtkSmartPointer`
   * doesn't seem to be wrapped correctly. C++ code should avoid using these.
   * Use the `Create*` variants instead.
   */
  VTK_NEWINSTANCE
  static vtkCPPythonPipeline* NewPipeline(const char* fname, int default_version = 1);
  VTK_NEWINSTANCE
  static vtkCPPythonPipeline* NewAndInitializePipeline(const char* fname, int default_version = 1);
  //@}
protected:
  /// For things like programmable filters that have a '\n' in their strings,
  /// we need to fix them to have \\n so that everything works smoothly
  void FixEOL(std::string&);

  /// Return the address of Pointer for the python script.
  std::string GetPythonAddress(void* pointer);

  vtkCPPythonPipeline();
  ~vtkCPPythonPipeline() override;

private:
  vtkCPPythonPipeline(const vtkCPPythonPipeline&) = delete;
  void operator=(const vtkCPPythonPipeline&) = delete;
};
#endif
