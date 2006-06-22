/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVGUIClientOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGUIClientOptions - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
// 
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.
// 
// .SECTION See Also
// vtksys::CommandLineArguments

#ifndef __vtkPVGUIClientOptions_h
#define __vtkPVGUIClientOptions_h

#include "vtkPVOptions.h"

class VTK_EXPORT vtkPVGUIClientOptions : public vtkPVOptions
{
public:
  static vtkPVGUIClientOptions* New();
  vtkTypeRevisionMacro(vtkPVGUIClientOptions,vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetMacro(PlayDemoFlag, int);
  vtkGetMacro(DisableRegistry, int);
  vtkGetMacro(CrashOnErrors, int);
  vtkGetMacro(StartEmpty, int);
  vtkGetMacro(ClientServerConnectionTimeout, int);
  vtkGetStringMacro(ParaViewScriptName);
  vtkSetStringMacro(ParaViewScriptName);
  vtkGetStringMacro(InternalScriptName);

protected:
  // Description:
  // Default constructor.
  vtkPVGUIClientOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVGUIClientOptions();

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

private:
  vtkPVGUIClientOptions(const vtkPVGUIClientOptions&); // Not implemented
  void operator=(const vtkPVGUIClientOptions&); // Not implemented

  vtkSetStringMacro(InternalScriptName);

  // Options:
  int PlayDemoFlag;
  int DisableRegistry;
  int CrashOnErrors;
  int StartEmpty;
  int ClientServerConnectionTimeout;
  char* ParaViewScriptName;
  char* InternalScriptName;
};

#endif // #ifndef __vtkPVGUIClientOptions_h
