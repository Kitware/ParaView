/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVPythonOptions
 * @brief   ParaView options storage
 *
 * An object of this class represents a storage for ParaView options
 *
 * These options can be retrieved during run-time, set using configuration file
 * or using Command Line Arguments.
*/

#ifndef vtkPVPythonOptions_h
#define vtkPVPythonOptions_h

#include "vtkPVOptions.h"

class VTKREMOTINGCORE_EXPORT vtkPVPythonOptions : public vtkPVOptions
{
public:
  static vtkPVPythonOptions* New();
  vtkTypeMacro(vtkPVPythonOptions, vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVPythonOptions();
  ~vtkPVPythonOptions() override;

  /**
   * This method is called when wrong argument is found. We record the first
   * unknown argument and then stop argument parsing by simply returning 1.
   */
  int WrongArgument(const char* argument) override;

private:
  vtkPVPythonOptions(const vtkPVPythonOptions&) = delete;
  void operator=(const vtkPVPythonOptions&) = delete;
};

#endif
