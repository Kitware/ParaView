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
class vtkProcessModule;
class vtkPVOptions;
class vtkProcessModuleGUIHelper;

class VTK_EXPORT vtkPVMain : public vtkObject
{
public:
  static vtkPVMain* New();
  vtkTypeRevisionMacro(vtkPVMain,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  typedef void (*INITIALIZE_INTERPRETER_FUNCTION )(vtkProcessModule* pm);
  int Run(vtkPVOptions*, vtkProcessModuleGUIHelper* guihelp,
          INITIALIZE_INTERPRETER_FUNCTION func,
          int argc, char* argv[]);
  static void Initialize(int* argc, char** argv[]);
  static void Finalize();
protected:
  // Description:
  // Default constructor.
  vtkPVMain();

  // Description:
  // Destructor.
  virtual ~vtkPVMain();
private:
  vtkProcessModule* ProcessModule;
  vtkPVMain(const vtkPVMain&); // Not implemented
  void operator=(const vtkPVMain&); // Not implemented
};

#endif // #ifndef __vtkPVMain_h

