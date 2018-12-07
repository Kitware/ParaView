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

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVPythonOptions : public vtkPVOptions
{
public:
  static vtkPVPythonOptions* New();
  vtkTypeMacro(vtkPVPythonOptions, vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the python script name.
   */
  vtkGetStringMacro(PythonScriptName);
  //@}

protected:
  /**
   * Default constructor.
   */
  vtkPVPythonOptions();

  /**
   * Destructor.
   */
  ~vtkPVPythonOptions() override;

  /**
   * Synchronizes the options among root and satellites.
   */
  void Synchronize();

  /**
   * After parsing, process extra option dependencies.
   */
  int PostProcess(int argc, const char* const* argv) override;

  /**
   * This method is called when wrong argument is found. If it returns 0, then
   * the parsing will fail.
   */
  int WrongArgument(const char* argument) override;

  // Options:
  vtkSetStringMacro(PythonScriptName);
  char* PythonScriptName;

private:
  vtkPVPythonOptions(const vtkPVPythonOptions&) = delete;
  void operator=(const vtkPVPythonOptions&) = delete;
};

#endif // #ifndef vtkPVPythonOptions_h
