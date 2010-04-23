/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringArrayHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVStringArrayHelper - Server-side helper for vtkSMStringArrayHelper.
// .SECTION Description
// This class implements a method for getting the values of a string
// array to the client.

#ifndef __vtkPVStringArrayHelper_h
#define __vtkPVStringArrayHelper_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkPVStringArrayHelperInternals;
class vtkStringArray;

class VTK_EXPORT vtkPVStringArrayHelper : public vtkPVServerObject
{
public:
  static vtkPVStringArrayHelper* New();
  vtkTypeMacro(vtkPVStringArrayHelper, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns a stream containing the elements of the string array.
  const vtkClientServerStream& GetStringList(vtkStringArray*);

protected:
  vtkPVStringArrayHelper();
  ~vtkPVStringArrayHelper();

  // Internal implementation details.
  vtkPVStringArrayHelperInternals* Internal;
private:
  vtkPVStringArrayHelper(const vtkPVStringArrayHelper&); // Not implemented
  void operator=(const vtkPVStringArrayHelper&); // Not implemented
};

#endif
