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
#include "vtkNew.h" //  for vtkNew.
#include <string>   // for std::string
#include <vector>   // for std::vector

class vtkCPPythonScriptV2Helper;

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
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set the arguments. The vector is deep copied. Note, changing arguments
   * after `Initialize` has no effect.
   */
  void SetArguments(const std::vector<std::string>& args);
  const std::vector<std::string>& GetArguments() const { return this->Arguments; }
  //@}

  //@{
  /**
   * Get/Set the execute parameters. The vector is deep copied.
   */
  void SetParameters(const std::vector<std::string>& params);
  const std::vector<std::string>& GetParameters() const { return this->Parameters; }
  //@}

  //@{
  /**
   * vtkInSituPipeline API implementation.
   */
  bool Initialize() override;
  bool Execute(int, double) override;
  bool Finalize() override;
  //@}

protected:
  vtkInSituPipelinePython();
  ~vtkInSituPipelinePython();

private:
  vtkInSituPipelinePython(const vtkInSituPipelinePython&) = delete;
  void operator=(const vtkInSituPipelinePython&) = delete;

  vtkNew<vtkCPPythonScriptV2Helper> Helper;
  char* FileName;
  std::vector<std::string> Arguments;
  std::vector<std::string> Parameters;
};

#endif
