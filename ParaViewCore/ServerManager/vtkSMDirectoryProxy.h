/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDirectoryProxy
// .SECTION Description
// Is a utility proxy to create/delete/rename directories.

#ifndef __vtkSMDirectoryProxy_h
#define __vtkSMDirectoryProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMDirectoryProxy : public vtkSMProxy
{
public:
  static vtkSMDirectoryProxy* New();
  vtkTypeMacro(vtkSMDirectoryProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create directory.
  bool MakeDirectory(const char* dir, vtkTypeUInt32 processes);

  // Description:
  // Remove a directory.
  bool DeleteDirectory(const char* dir, vtkTypeUInt32 processes);

  // Description:
  // Rename a file or directory.
  bool Rename(const char* oldname, const char* newname, vtkTypeUInt32 processes);

//BTX
protected:
  vtkSMDirectoryProxy();
  ~vtkSMDirectoryProxy();

private:
  vtkSMDirectoryProxy(const vtkSMDirectoryProxy&); // Not implemented
  void operator=(const vtkSMDirectoryProxy&); // Not implemented
//ETX
};

#endif
