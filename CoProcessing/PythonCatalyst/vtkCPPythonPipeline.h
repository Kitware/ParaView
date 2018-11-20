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
