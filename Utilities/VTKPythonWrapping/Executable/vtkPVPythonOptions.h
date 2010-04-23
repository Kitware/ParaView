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
// .NAME vtkPVPythonOptions - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
// 
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.

#ifndef __vtkPVPythonOptions_h
#define __vtkPVPythonOptions_h

#include "vtkPVOptions.h"

class VTK_EXPORT vtkPVPythonOptions : public vtkPVOptions
{
public:
  static vtkPVPythonOptions* New();
  vtkTypeMacro(vtkPVPythonOptions,vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When set to true, the python script is run on satellites as well, otherwise
  // only the root node processes the python script. Disabled by default.
  vtkSetMacro(EnableSymmetricScripting, int);
  vtkGetMacro(EnableSymmetricScripting, int);

  // Description:
  // Called to create the primary self-connection. Default implementation
  // handles all paraview cases. Makes it possible for new paraview-based
  // applications to provide new primary connection types.
  virtual vtkSelfConnection* NewSelfConnection();

  // Description:
  // Get the python script name.
  vtkGetStringMacro(PythonScriptName);

protected:
  // Description:
  // Default constructor.
  vtkPVPythonOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVPythonOptions();

  // Description:
  // Synchronizes the options among root and satellites.
  void Synchronize();

  // Description:
  // Initialize arguments.
  virtual void Initialize();

  // Description:
  // After parsing, process extra option dependencies.
  virtual int PostProcess(int argc, const char* const* argv);

  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int WrongArgument(const char* argument);

  // Options:
  vtkSetStringMacro(PythonScriptName);
  char* PythonScriptName;

  int EnableSymmetricScripting;
private:
  vtkPVPythonOptions(const vtkPVPythonOptions&); // Not implemented
  void operator=(const vtkPVPythonOptions&); // Not implemented
};

#endif // #ifndef __vtkPVPythonOptions_h


