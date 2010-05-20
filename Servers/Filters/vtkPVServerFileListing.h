/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerFileListing.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerFileListing - Server-side helper for vtkPVFileListing.
// .SECTION Description

#ifndef __vtkPVServerFileListing_h
#define __vtkPVServerFileListing_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkPVServerFileListingInternals;

class VTK_EXPORT vtkPVServerFileListing : public vtkPVServerObject
{
public:
  static vtkPVServerFileListing* New();
  vtkTypeMacro(vtkPVServerFileListing, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get a list of files in the given directory on the server.
  const vtkClientServerStream& GetFileListing(const char* dirname,
                                              int save);

  // Description:
  // Returns a list of "special" files / directories on the server (includes Win32 drives, Win32 favorites, home directory, etc)
  const vtkClientServerStream& GetSpecial();

  // Description:
  // Get the current working directory of the process on the server.
  const char* GetCurrentWorkingDirectory();

  // Description:
  // Check if the given directory exists on the server.
  int FileIsDirectory(const char* dirname);

  // Description:
  // Check if the given file is readable on the server.
  int FileIsReadable(const char* name);
protected:
  vtkPVServerFileListing();
  ~vtkPVServerFileListing();

  // Internal implementation details.
  vtkPVServerFileListingInternals* Internal;

  void List(const char* dirname, int save);
private:
  vtkPVServerFileListing(const vtkPVServerFileListing&); // Not implemented
  void operator=(const vtkPVServerFileListing&); // Not implemented
};

#endif
