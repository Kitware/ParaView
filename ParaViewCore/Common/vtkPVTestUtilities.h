/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTestUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTestUtilities - A class to facilitate common test operations
// .SECTION Description
//

#ifndef vtkPVTestUtilities_h
#define vtkPVTestUtilities_h

#include "vtkObject.h"

class vtkPolyData;
class vtkDataArray;

class VTK_EXPORT vtkPVTestUtilities : public vtkObject
{
public:
  // the usual vtk stuff
  vtkTypeMacro(vtkPVTestUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVTestUtilities *New();

  // Description:
  // Initialize the object from command tail arguments.
  void Initialize(int argc, char **argv);
  // Description:
  // Given a path relative to the Data root (provided
  // in argv by -D option), construct a OS independent path
  // to the file specified by "name". "name" should not start
  // with a path seperator and if path seperators are needed
  // use '/'. Be sure to delete [] the return when you are
  // finished.
  char *GetDataFilePath(const char *name) {
    return this->GetFilePath(this->DataRoot,name); }
  // Description:
  // Given a path relative to the working directory (provided
  // in argv by -T option), construct a OS independent path
  // to the file specified by "name". "name" should not start
  // with a path seperator and if path seperators are needed
  // use '/'. Be sure to delete [] the return when you are
  // finished.
  char *GetTempFilePath(const char *name) {
    return this->GetFilePath(this->TempRoot,name); }

protected:
  vtkPVTestUtilities(){ this->Initialize(0,0); }
  ~vtkPVTestUtilities(){ this->Initialize(0,0); }

private:
  vtkPVTestUtilities(const vtkPVTestUtilities &); // Not implemented
  void operator=(const vtkPVTestUtilities &); // Not implemented
  ///
  char GetPathSep();
  char *GetDataRoot();
  char *GetTempRoot();
  char *GetCommandTailArgument(const char *tag);
  char *GetFilePath(const char *base, const char *name);
  //
  int Argc;
  char **Argv;
  char *DataRoot;
  char *TempRoot;
};

#endif
