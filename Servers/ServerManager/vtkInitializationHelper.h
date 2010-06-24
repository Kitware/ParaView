
/*=========================================================================

  Program:   ParaView
  Module:    vtkInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInitializationHelper - help class for python modules
// .SECTION Description
// This class is used by the python modules when they are loaded from
// python (as opposed to pvpython). It simply initializes the server
// manager so that it can be used.

#ifndef __vtkInitializationHelper_h
#define __vtkInitializationHelper_h

#include "vtkObject.h"

class vtkProcessModuleGUIHelper;
class vtkPVMain;
class vtkPVOptions;
class vtkSMApplication;
class vtkProcessModule;

class VTK_EXPORT vtkInitializationHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkInitializationHelper,vtkObject);

  // Description:
  // Initializes the server manager. Do not use the server manager
  // before calling this.
  static void Initialize(const char* executable);
  static void Initialize(const char* executable, vtkPVOptions* options);

  // Description:
  // Alternative API to initialize the server manager. This takes in  the
  // command line arguments and the vtkPVOptions instance to use to process the
  // command line options.
  static void Initialize(int argc, char**argv, vtkPVOptions* options);

  // Description:
  // Finalizes the server manager. Do not use the server manager
  // after calling this.
  static void Finalize();

  // Description:
  // To be used by executables. Call this method to initialize the client-server
  // interpretor.
  static void InitializeInterpretor(vtkProcessModule*);
protected:
  vtkInitializationHelper() {};
  virtual ~vtkInitializationHelper() {};

  static vtkPVMain* PVMain;
  static vtkSMApplication* Application;
  static vtkPVOptions* Options;
  static vtkProcessModuleGUIHelper* Helper;

private:

  vtkInitializationHelper(const vtkInitializationHelper&); // Not implemented
  void operator=(const vtkInitializationHelper&); // Not implemented
};

#endif
