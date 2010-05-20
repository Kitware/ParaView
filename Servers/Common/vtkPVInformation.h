/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInformation - Superclass for information objects.
// .SECTION Description
// Subclasses of this class are used to get information from the server.

#ifndef __vtkPVInformation_h
#define __vtkPVInformation_h

#include "vtkObject.h"

class vtkClientServerStream;

class VTK_EXPORT vtkPVInformation : public vtkObject
{
public:
  vtkTypeMacro(vtkPVInformation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*) = 0;
  virtual void CopyFromStream(const vtkClientServerStream*);
  //ETX

  // Description:
  // Set/get whether to gather information only from the root.
  vtkGetMacro(RootOnly, int);
  vtkSetMacro(RootOnly, int);

protected:
  vtkPVInformation();
  ~vtkPVInformation();

  int RootOnly;

  vtkPVInformation(const vtkPVInformation&); // Not implemented
  void operator=(const vtkPVInformation&); // Not implemented
};

#endif
