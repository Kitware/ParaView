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
// kwsys::CommandLineArguments

#ifndef __vtkPVGUIClientOptions_h
#define __vtkPVGUIClientOptions_h

#include "vtkPVBatchOptions.h"

class VTK_EXPORT vtkPVGUIClientOptions : public vtkPVBatchOptions
{
public:
  static vtkPVGUIClientOptions* New();
  vtkTypeRevisionMacro(vtkPVGUIClientOptions,vtkPVBatchOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetMacro(PlayDemoFlag, int);
  vtkGetMacro(DisableRegistry, int);
  vtkGetMacro(CrashOnErrors, int);
  vtkGetMacro(StartEmpty, int);
  vtkGetStringMacro(ParaViewScriptName);
  vtkGetStringMacro(ParaViewDataName);

  // Description:
  // Some variables need to be overwritten
  vtkSetStringMacro(HostName);
  vtkSetStringMacro(Username);
  vtkSetClampMacro(PortNumber, int, 0, 65535);
  vtkSetStringMacro(ParaViewScriptName);

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

  // Options:
  int PlayDemoFlag;
  int DisableRegistry;
  int CrashOnErrors;
  int StartEmpty;
  char* ParaViewScriptName;

  vtkSetStringMacro(ParaViewDataName);
  char* ParaViewDataName;
};

#endif // #ifndef __vtkPVGUIClientOptions_h

