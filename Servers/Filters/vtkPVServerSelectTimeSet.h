/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerSelectTimeSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerSelectTimeSet - Server-side helper for vtkPVSelectTimeSet.
// .SECTION Description

#ifndef __vtkPVServerSelectTimeSet_h
#define __vtkPVServerSelectTimeSet_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkPVServerSelectTimeSetInternals;
class vtkGenericEnSightReader;

class VTK_EXPORT vtkPVServerSelectTimeSet : public vtkPVServerObject
{
public:
  static vtkPVServerSelectTimeSet* New();
  vtkTypeMacro(vtkPVServerSelectTimeSet, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get a list the time sets provided by the given reader.
  const vtkClientServerStream& GetTimeSets(vtkGenericEnSightReader*);

protected:
  vtkPVServerSelectTimeSet();
  ~vtkPVServerSelectTimeSet();

  // Internal implementation details.
  vtkPVServerSelectTimeSetInternals* Internal;
private:
  vtkPVServerSelectTimeSet(const vtkPVServerSelectTimeSet&); // Not implemented
  void operator=(const vtkPVServerSelectTimeSet&); // Not implemented
};

#endif
