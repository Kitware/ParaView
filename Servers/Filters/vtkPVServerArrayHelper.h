/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerArrayHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerArrayHelper - Server-side helper to be used with
// vtkSMInformationHelpers.
// .SECTION Description
// This class is used by vtkSM<Type>ArrayInformationHelper classes. 
// Each information helper creates this object on the server and sets
// the vtkObject and command that this class call on the vtkObject
// to obtain a vtkDataArray. This class converts that vtkDataArray
// and marshalls it into a vtkClientServerStream which can be read
// on the client to populate the property.

#ifndef __vtkPVServerArrayHelper_h
#define __vtkPVServerArrayHelper_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkObject;

class VTK_EXPORT vtkPVServerArrayHelper : public vtkPVServerObject
{
public:
  static vtkPVServerArrayHelper* New();
  vtkTypeMacro(vtkPVServerArrayHelper, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the data array returned by the particular command.
  // \c object is the object from which the vtkDataArray is obtained.
  // \c command is the method called on the object to obtain
  // the vtkDataArray.
  const vtkClientServerStream& GetArray(vtkObject* object, 
    const char* command);
protected:
  vtkPVServerArrayHelper();
  ~vtkPVServerArrayHelper();

  vtkClientServerStream* Result;
  const vtkClientServerStream& EmptyResult();
private:
  vtkPVServerArrayHelper(const vtkPVServerArrayHelper&); // Not implemented.
  void operator=(const vtkPVServerArrayHelper&); // Not implemented.
};


#endif


