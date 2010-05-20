/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVMain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMain - encapaulate common functions for running a paraview program.
// .SECTION Description
// An object that has all the common stuff for running a paraview application.
// 

#ifndef __vtkPVMain_h
#define __vtkPVMain_h

#include "vtkObject.h"
class vtkProcessModule;
class vtkPVOptions;
class vtkProcessModuleGUIHelper;

class VTK_EXPORT vtkPVMain : public vtkObject
{
public:
  static vtkPVMain* New();
  vtkTypeMacro(vtkPVMain,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  typedef void (*INITIALIZE_INTERPRETER_FUNCTION )(vtkProcessModule* pm);

  // Description:
  // Initializes ProcessModule/Interpreter etc.
  int Initialize(vtkPVOptions*, vtkProcessModuleGUIHelper* guihelp,
          INITIALIZE_INTERPRETER_FUNCTION func,
          int argc, char* argv[]);
  
  // Description:
  // Initializes MPI (if needed). This must be called before any
  // VTK objects are created.
  static void Initialize(int* argc, char** argv[]);

  // Description:
  // Finalize.
  static void Finalize();

  // Description:
  // Typically one would call vtkProcessModuleGUIHelper::Run() to 
  // start the application event loop.
  // However, if helper is not present, eg. renderserver, then 
  // we call this method to call this->ProcessModule->Start().
  int Run(vtkPVOptions*);

  // Description:
  // This flag is set by default to 1. If cleared/set to 0, 
  // MPI is not initialized. This flag is ignored if VTK_USE_MPI 
  // is not defined, i.e. VTK was not built with MPI support.
  static void SetUseMPI(int s);
  static int GetUseMPI();
//BTX
protected:
  // Description:
  // Default constructor.
  vtkPVMain();

  // Description:
  // Destructor.
  virtual ~vtkPVMain();
  static int UseMPI;

private:
  vtkProcessModule* ProcessModule;
  vtkPVMain(const vtkPVMain&); // Not implemented
  void operator=(const vtkPVMain&); // Not implemented

  // Description
  // Mark whether or not this class needs to call MPI_Finalize().
  // This should only be done if this class called MPI_Init().
  static int FinalizeMPI;
//ETX
};

#endif // #ifndef __vtkPVMain_h

