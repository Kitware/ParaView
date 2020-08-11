/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituPipelinePython.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkInSituPipelinePython
 * @brief subclass for Python in situ scripts.
 * @ingroup Insitu
 *
 * vtkInSituPipelinePython supports loading ParaView Python scripts for in situ
 * analysis.
 */

#ifndef vtkInSituPipelinePython_h
#define vtkInSituPipelinePython_h

#include "vtkInSituPipeline.h"

#include <memory> // for std::unique_ptr

class vtkSMProxy;

class VTKPVINSITU_EXPORT vtkInSituPipelinePython : public vtkInSituPipeline
{
public:
  static vtkInSituPipelinePython* New();
  vtkTypeMacro(vtkInSituPipelinePython, vtkInSituPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the path to either a .py file or a .zip file or a Python package
   * directory.
   */
  vtkSetStringMacro(PipelinePath);
  vtkGetStringMacro(PipelinePath);
  //@}

  //@{
  /**
   * vtkInSituPipeline API implementation.
   */
  bool Initialize() override;
  bool Execute(int, double) override;
  bool Finalize() override;
  //@}

  //@{
  /**
   * Internal methods. These are called by Python modules internal to ParaView
   * and may change without notice. Should not be considered as part of ParaView
   * API.
   */
  void SetOptions(vtkSMProxy* catalystOptions);
  vtkGetObjectMacro(Options, vtkSMProxy);
  static void RegisterExtractGenerator(vtkSMProxy* generator);
  //@}
protected:
  vtkInSituPipelinePython();
  ~vtkInSituPipelinePython();

  bool IsLiveActivated();
  void DoLive(int, double);

private:
  vtkInSituPipelinePython(const vtkInSituPipelinePython&) = delete;
  void operator=(const vtkInSituPipelinePython&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  char* PipelinePath;

  vtkSMProxy* Options;
};

#endif
